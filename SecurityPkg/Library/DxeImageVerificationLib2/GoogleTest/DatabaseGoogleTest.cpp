/** @file
  Unit tests for the signature-database helpers in
  DxeImageVerificationLib (Database.c): IsInDb, IsInDbx,
  LoadSignatureDatabase, LoadSignatureDatabases,
  IsChainRevoked,
  ExtractAuthData, and
  EvaluateImageCertificate. The database helpers are exercised against
  synthetic in-memory EFI_SIGNATURE_LIST buffers built by helpers in
  this file; the variable loaders against a mocked GetVariable2; and the
  certificate helpers against a mocked BaseCryptLib.
  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockUefiLib.h>
#include <GoogleTest/Library/MockBaseCryptLib.h>

#include <vector>
#include <cstring>

extern "C" {
  #include <Uefi.h>
  #include <Guid/ImageAuthentication.h>
  #include <Guid/WinCertificate.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include "../Database.h"
  #include "../Support.h"

  EFI_STATUS
  LoadSignatureDatabase (
    IN  CONST CHAR16  *DatabaseName,
    OUT VOID          **Buffer,
    OUT UINTN         *BufferSize
    );

  BOOLEAN
  IsChainRevoked (
    IN  CONST UINT8  *CertChain,
    IN  UINTN        CertChainSize,
    IN  CONST VOID   *Dbx,
    IN  UINTN        DbxSize
    );

  EFI_STATUS
  ExtractAuthData (
    IN  CONST WIN_CERTIFICATE  *Cert,
    OUT CONST UINT8            **AuthData,
    OUT UINTN                  *AuthDataSize
    );
}

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;

static bool
GetImageHashIndexForTest (
  const EFI_GUID  *Guid,
  UINTN           *Index
  )
{
  UINTN  I;

  if ((Guid == nullptr) || (Index == nullptr)) {
    return false;
  }

  for (I = 0; I < ARRAY_SIZE (mHashAlgorithms); I++) {
    if (CompareGuid (Guid, mHashAlgorithms[I].ImageHashGuid)) {
      *Index = I;
      return true;
    }
  }

  return false;
}

// ---------------------------------------------------------------------------
// Helpers for constructing signature-list buffers.
// ---------------------------------------------------------------------------

//
// Append one EFI_SIGNATURE_LIST containing SignatureCount entries of
// EntrySize bytes each (entry payloads are zero-initialized) to Buffer.
// Returns the offset of the new list within Buffer.
//
static size_t
AppendSignatureList (
  std::vector<UINT8>  &Buffer,
  const EFI_GUID      &SignatureType,
  UINT32              SignatureHeaderSize,
  UINT32              EntrySize,
  UINT32              SignatureCount
  )
{
  const size_t  PayloadBytes = (size_t)EntrySize * (size_t)SignatureCount;
  const size_t  ListBytes    = sizeof (EFI_SIGNATURE_LIST) + SignatureHeaderSize + PayloadBytes;
  const size_t  Offset       = Buffer.size ();

  Buffer.resize (Offset + ListBytes, 0);

  EFI_SIGNATURE_LIST  *List = (EFI_SIGNATURE_LIST *)(Buffer.data () + Offset);

  CopyMem (&List->SignatureType, &SignatureType, sizeof (EFI_GUID));
  List->SignatureListSize   = (UINT32)ListBytes;
  List->SignatureHeaderSize = SignatureHeaderSize;
  List->SignatureSize       = EntrySize;

  return Offset;
}

//
// Walk callback that simply counts invocations and records the
// per-list SignatureType GUIDs in visit order.
//
// SHA-256 entry size: 16-byte owner GUID + 32-byte digest.
static constexpr UINT32  kSha256EntrySize = sizeof (EFI_GUID) + 32;
static constexpr UINT32  kSha384EntrySize = sizeof (EFI_GUID) + 48;

// V1 EFI_CERT_X509_SHA256 entry: owner GUID + 32-byte TBS hash + EFI_TIME (TimeOfRevocation).
static constexpr UINT32  kSha256TbsV1EntrySize = sizeof (EFI_GUID) + 32 + sizeof (EFI_TIME);

// ---------------------------------------------------------------------------
// IsInDb / IsInDbx helpers
// ---------------------------------------------------------------------------

//
// Write Bytes into the entry payload (the part after the owner GUID) of
// signature index EntryIndex inside the EFI_SIGNATURE_LIST that begins
// at ListOffset within Buffer.
//
static void
SetEntryPayload (
  std::vector<UINT8>        &Buffer,
  size_t                    ListOffset,
  UINTN                     EntryIndex,
  const std::vector<UINT8>  &Bytes
  )
{
  EFI_SIGNATURE_LIST  *List      = (EFI_SIGNATURE_LIST *)(Buffer.data () + ListOffset);
  const size_t        FirstEntry = ListOffset + sizeof (EFI_SIGNATURE_LIST) + List->SignatureHeaderSize;
  const size_t        EntryStart = FirstEntry + (size_t)EntryIndex * (size_t)List->SignatureSize;
  const size_t        PayloadOff = EntryStart + sizeof (EFI_GUID);

  ASSERT_LE (PayloadOff + Bytes.size (), Buffer.size ());
  std::memcpy (Buffer.data () + PayloadOff, Bytes.data (), Bytes.size ());
}

// SHA-256 digest payload size (no owner GUID).
static constexpr UINTN  kSha256DigestSize = 32;
static constexpr UINTN  kSha384DigestSize = 48;

// V2 (EFI_SIGNATURE_V2_DATA) entry size: the payload only, with no SignatureOwner prefix.
static constexpr UINT32  kSha256V2EntrySize = 32;

//
// Write Bytes into the entry payload of a V2 (EFI_SIGNATURE_V2_DATA) signature list, whose entries
// omit the SignatureOwner, so the payload begins at the entry start rather than sizeof (EFI_GUID)
// bytes in.
//
static void
SetV2EntryPayload (
  std::vector<UINT8>        &Buffer,
  size_t                    ListOffset,
  UINTN                     EntryIndex,
  const std::vector<UINT8>  &Bytes
  )
{
  EFI_SIGNATURE_LIST  *List      = (EFI_SIGNATURE_LIST *)(Buffer.data () + ListOffset);
  const size_t        FirstEntry = ListOffset + sizeof (EFI_SIGNATURE_LIST) + List->SignatureHeaderSize;
  const size_t        EntryStart = FirstEntry + (size_t)EntryIndex * (size_t)List->SignatureSize;

  ASSERT_LE (EntryStart + Bytes.size (), Buffer.size ());
  std::memcpy (Buffer.data () + EntryStart, Bytes.data (), Bytes.size ());
}

static DIGEST_CACHE
MakeBoundCache (
  const EFI_GUID            *HashType,
  const std::vector<UINT8>  &Digest
  )
{
  DIGEST_CACHE  Cache;
  UINTN         Index;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_TRUE (GetImageHashIndexForTest (HashType, &Index));
  EXPECT_LE (Digest.size (), (size_t)MAX_DIGEST_SIZE);

  std::memcpy (Cache.Entries[Index].Bytes, Digest.data (), Digest.size ());
  Cache.Entries[Index].BufferSize = Digest.size ();
  return Cache;
}

//
// Bind an X509 (certificate) digest cache to a DER certificate. The cache borrows Cert's storage,
// so Cert must outlive the returned cache.
//
static DIGEST_CACHE
MakeCertCache (
  const std::vector<UINT8>  &Cert
  )
{
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = DigestCacheTypeX509;
  Cache.Buffer     = Cert.data ();
  Cache.BufferSize = Cert.size ();
  return Cache;
}

//
// Build a buffer in EFI_CERT_STACK format:
//   UINT8 CertNumber; { UINT32 CertLen (LE); UINT8 CertData[CertLen]; } * N
//
static std::vector<UINT8>
MakeCertStack (
  const std::vector<std::vector<UINT8> >  &Certs
  )
{
  std::vector<UINT8>  Buf;

  Buf.push_back ((UINT8)Certs.size ());
  for (const auto &Cert : Certs) {
    UINT32  Len = (UINT32)Cert.size ();
    Buf.push_back ((UINT8)(Len & 0xFF));
    Buf.push_back ((UINT8)((Len >>  8) & 0xFF));
    Buf.push_back ((UINT8)((Len >> 16) & 0xFF));
    Buf.push_back ((UINT8)((Len >> 24) & 0xFF));
    Buf.insert (Buf.end (), Cert.begin (), Cert.end ());
  }

  return Buf;
}

//
// Default PKCS#7 auth-data / image-hash payloads shared by the
// certificate-authorization tests.
//
static const std::vector<UINT8>  kAuthDataDefault  = std::vector<UINT8>(16, 0xA1);
static const std::vector<UINT8>  kImageHashDefault = std::vector<UINT8>(SHA256_DIGEST_SIZE, 0x55);

// Build a PKCS_SIGNED_DATA WIN_CERTIFICATE wrapping `Payload`.
static std::vector<UINT8>
MakePkcsSignedDataCert (
  const std::vector<UINT8>  &Payload
  )
{
  std::vector<UINT8>  Buffer (sizeof (WIN_CERTIFICATE) + Payload.size (), 0);
  WIN_CERTIFICATE     *Cert = (WIN_CERTIFICATE *)Buffer.data ();

  Cert->dwLength         = (UINT32)Buffer.size ();
  Cert->wRevision        = 0x0200;
  Cert->wCertificateType = WIN_CERT_TYPE_PKCS_SIGNED_DATA;
  std::memcpy (Buffer.data () + sizeof (WIN_CERTIFICATE), Payload.data (), Payload.size ());
  return Buffer;
}

// Tiny throwaway "image" buffer for the digest cache; mocks of
// GetAuthenticodeHash never dereference it.
static UINT8  kFakeImage[16] = { 0 };

static void
InitImageCache (
  DIGEST_CACHE  &Cache
  )
{
  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = DigestCacheTypeImage;
  Cache.Buffer     = kFakeImage;
  Cache.BufferSize = sizeof (kFakeImage);
}

TEST (IsInDbTest, NullDatabaseWithNonZeroSize_NotFound) {
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_FALSE (IsInDb (&Cache, NULL, 1));
  // Validate that Cache remains consistent
  EXPECT_EQ (Cache.Buffer, (const VOID *)(UINTN)1);
  EXPECT_EQ (Cache.BufferSize, (UINTN)1);
}

TEST (IsInDbTest, NullDatabaseWithZeroSize_NotFound) {
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_FALSE (IsInDb (&Cache, NULL, 0));
  // Validate that Cache remains consistent
  EXPECT_EQ (Cache.Buffer, (const VOID *)(UINTN)1);
  EXPECT_EQ (Cache.BufferSize, (UINTN)1);
}

TEST (IsInDbTest, NullCache_NotFound) {
  UINT8  Dummy = 0;

  EXPECT_FALSE (IsInDb (NULL, &Dummy, 1));
}

TEST (IsInDbTest, UnboundCache_NotFound) {
  UINT8         Dummy = 0;
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  EXPECT_FALSE (IsInDb (&Cache, &Dummy, 1));
}

TEST (IsInDbTest, ZeroSizeCache_NotFound) {
  UINT8         Dummy = 0;
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 0;

  EXPECT_FALSE (IsInDb (&Cache, &Dummy, 1));
}

TEST (IsInDbTest, HashComputationFailure_NotAuthorized) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  // A hash failure means this list cannot authorize; a best-effort search reports "not found".
  EXPECT_FALSE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, ExactMatch_Found) {
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);
  std::vector<UINT8>  Target (kSha256DigestSize, 0xAA);

  SetEntryPayload (Db, Off, 1, Target);

  DIGEST_CACHE  Cache = MakeBoundCache (&gEfiCertSha256Guid, Target);

  EXPECT_TRUE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, V2ImageHashExactMatch_Found) {
  // A V2 (EFI_SIGNATURE_V2_DATA) image-hash list stores the digest with no SignatureOwner prefix,
  // so the payload must be read from the entry start rather than sizeof (EFI_GUID) bytes in.
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertV2Sha256Guid, 0, kSha256V2EntrySize, 2);
  std::vector<UINT8>  Target (kSha256DigestSize, 0xAA);

  SetV2EntryPayload (Db, Off, 1, Target);

  DIGEST_CACHE  Cache = MakeBoundCache (&gEfiCertSha256Guid, Target);

  EXPECT_TRUE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, V2X509CertExactMatch_Found) {
  // A V2 full-certificate list stores the DER certificate with no owner prefix.
  std::vector<UINT8>  Cert (24, 0xC7);
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertV2X509Guid, 0, (UINT32)Cert.size (), 1);

  SetV2EntryPayload (Db, Off, 0, Cert);

  DIGEST_CACHE  Cache = MakeCertCache (Cert);

  EXPECT_TRUE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, V2X509CertHashMatch_Found) {
  // A V2 X.509 TBS-cert-hash list stores the hash with neither the owner GUID nor a v1
  // TimeOfRevocation trailer.
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Cert (24, 0xC7);
  std::vector<UINT8>  TbsHash (kSha256DigestSize, 0x5A);
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertV2X509Sha256Guid, 0, kSha256V2EntrySize, 1);

  SetV2EntryPayload (Db, Off, 0, TbsHash);

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (
             IN  VOID            *CertBuffer,
             IN  UINTN           CertSize,
             IN  CONST EFI_GUID  *HashType,
             OUT UINT8           *OutDigest,
             OUT UINTN           *OutDigestSize
         ) -> EFI_STATUS {
    (VOID)CertBuffer;
    (VOID)CertSize;
    (VOID)HashType;
    SetMem (OutDigest, SHA256_DIGEST_SIZE, 0x5A);
    *OutDigestSize = SHA256_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  DIGEST_CACHE  Cache = MakeCertCache (Cert);

  EXPECT_TRUE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbxTest, V2ImageHashExactMatch_Revoked) {
  // Deny-list parsing honors the V2 ownerless layout the same as the allow-list.
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertV2Sha256Guid, 0, kSha256V2EntrySize, 1);
  std::vector<UINT8>  Target (kSha256DigestSize, 0xAA);

  SetV2EntryPayload (Dbx, Off, 0, Target);

  DIGEST_CACHE  Cache = MakeBoundCache (&gEfiCertSha256Guid, Target);

  EXPECT_TRUE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

TEST (IsInDbTest, NoMatchingEntry_NotFound) {
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  std::vector<UINT8>  Stored (kSha256DigestSize, 0xAA);

  SetEntryPayload (Db, Off, 0, Stored);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0xBB);
  DIGEST_CACHE        Cache = MakeBoundCache (&gEfiCertSha256Guid, Digest);

  EXPECT_FALSE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, UnknownSignatureTypeList_Skipped) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Guid, 0, sizeof (EFI_GUID) + 16, 1);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0xAA);
  DIGEST_CACHE        Cache = MakeBoundCache (&gEfiCertSha256Guid, Digest);

  EXPECT_FALSE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, MismatchedSignatureSize_Skipped) {
  // List type matches but per-entry size doesn't, so the list describes
  // a different algorithm and must be skipped.
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha384EntrySize, 1);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0xCC);
  DIGEST_CACHE        Cache = MakeBoundCache (&gEfiCertSha256Guid, Digest);

  EXPECT_FALSE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, MatchInSecondList_Found) {
  // First list is the wrong algorithm, second list contains the target.
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Guid, 0, sizeof (EFI_GUID) + 16, 1);
  size_t  SecondOff = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);

  std::vector<UINT8>  Target (kSha256DigestSize, 0x77);

  SetEntryPayload (Db, SecondOff, 1, Target);

  DIGEST_CACHE  Cache = MakeBoundCache (&gEfiCertSha256Guid, Target);

  EXPECT_TRUE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, NonZeroSignatureHeaderSize_EntryMathCorrect) {
  // SignatureHeaderSize is non-zero: the per-list header occupies
  // additional bytes between EFI_SIGNATURE_LIST and the first entry.
  // A naive cursor that forgets to skip it would either miss the
  // payload entirely or read the header bytes as a fake entry.
  constexpr UINT32    kHeader = 8;
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, kHeader, kSha256EntrySize, 2);

  // Fill the per-list header with a recognizable pattern so a math
  // bug that read it as an entry would compare against this, not the
  // real digest. The search target intentionally differs from it.
  std::memset (Db.data () + Off + sizeof (EFI_SIGNATURE_LIST), 0xEE, kHeader);

  std::vector<UINT8>  Target (kSha256DigestSize, 0x55);

  SetEntryPayload (Db, Off, 1, Target);

  DIGEST_CACHE  Cache = MakeBoundCache (&gEfiCertSha256Guid, Target);

  EXPECT_TRUE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, ZeroEntryList_NotFound) {
  // A well-formed list with zero entries must be skipped without a
  // false positive (EntryCount == 0 means the inner loop never runs).
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 0);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0x00);
  DIGEST_CACHE        Cache = MakeBoundCache (&gEfiCertSha256Guid, Digest);

  EXPECT_FALSE (IsInDb (&Cache, Db.data (), Db.size ()));
}

//
// A malformed db (the sole list overruns the buffer) truncates to an empty
// prefix; a best-effort allow-list search simply finds no authority.
//
TEST (IsInDbTest, MalformedDb_BestEffortNotFound) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  ((EFI_SIGNATURE_LIST *)Db.data ())->SignatureListSize = (UINT32)(Db.size () + 1);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0x00);
  DIGEST_CACHE        Cache = MakeBoundCache (&gEfiCertSha256Guid, Digest);

  EXPECT_FALSE (IsInDb (&Cache, Db.data (), Db.size ()));
}

//
// A well-formed authorizing list followed by a malformed trailing fragment:
// the valid prefix still authorizes (best-effort allow-list search).
//
TEST (IsInDbTest, MalformedTail_ValidPrefixAuthorizes) {
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  std::vector<UINT8>  Target (kSha256DigestSize, 0x5A);

  SetEntryPayload (Db, Off, 0, Target);

  // Append a stray fragment too small to be a list header.
  Db.resize (Db.size () + sizeof (EFI_SIGNATURE_LIST) - 1, 0);

  DIGEST_CACHE  Cache = MakeBoundCache (&gEfiCertSha256Guid, Target);

  EXPECT_TRUE (IsInDb (&Cache, Db.data (), Db.size ()));
}

TEST (IsInDbTest, ZeroSizeNonNullDatabase_EmptyDatabaseNotFound) {
  UINT8         Dummy = 0;
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_FALSE (IsInDb (&Cache, &Dummy, 0));
}

//
// A db list whose SignatureType is a supported image-hash GUID (so GetHash
// succeeds against the bound cache) but whose SignatureHeaderSize is
// inflated so SigListIterInit yields an empty range for it. The list must be
// skipped and no authority returned.
//
TEST (IsInDbTest, MalformedListHeader_Skipped) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xAB);
  DIGEST_CACHE        Cache = MakeBoundCache (&gEfiCertSha256Guid, Digest);

  // One image-hash list sized for a single SHA-256 entry, but with an
  // inflated SignatureHeaderSize that overflows the list payload area.
  const UINT32        EntrySize = kSha256EntrySize;
  const UINT32        ListSize  = (UINT32)(sizeof (EFI_SIGNATURE_LIST) + EntrySize);
  std::vector<UINT8>  Db (ListSize, 0);

  EFI_SIGNATURE_LIST  *List = (EFI_SIGNATURE_LIST *)Db.data ();

  CopyMem (&List->SignatureType, &gEfiCertSha256Guid, sizeof (EFI_GUID));
  List->SignatureListSize   = ListSize;
  List->SignatureHeaderSize = ListSize;        // > ListSize - sizeof (EFI_SIGNATURE_LIST)
  List->SignatureSize       = EntrySize;

  EXPECT_FALSE (IsInDb (&Cache, Db.data (), Db.size ()));
}

// ---------------------------------------------------------------------------
// LoadSignatureDatabase (uses MockUefiLib::GetVariable2)
// ---------------------------------------------------------------------------

class LoadSignatureDatabaseTest : public ::testing::Test {
protected:
  MockUefiLib UefiLibMock;
};

TEST_F (LoadSignatureDatabaseTest, NullDatabaseName_ReturnsInvalidParameter) {
  VOID   *Buffer    = NULL;
  UINTN  BufferSize = 0;

  EXPECT_EQ (
    LoadSignatureDatabase (NULL, &Buffer, &BufferSize),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (LoadSignatureDatabaseTest, NullBuffer_ReturnsInvalidParameter) {
  UINTN  BufferSize = 0;

  EXPECT_EQ (
    LoadSignatureDatabase ((const CHAR16 *)u"db", NULL, &BufferSize),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (LoadSignatureDatabaseTest, NullSize_ReturnsInvalidParameter) {
  VOID  *Buffer = NULL;

  EXPECT_EQ (
    LoadSignatureDatabase ((const CHAR16 *)u"db", &Buffer, NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (LoadSignatureDatabaseTest, VariableMissing_SuccessWithNullBuffer) {
  // EFI_NOT_FOUND is normalized to EFI_SUCCESS with *Buffer == NULL.
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_NOT_FOUND));

  VOID   *Buffer    = (VOID *)(UINTN)0xDEADBEEF; // pre-set: must be cleared
  UINTN  BufferSize = 0xAA;

  EXPECT_EQ (
    LoadSignatureDatabase ((const CHAR16 *)u"db", &Buffer, &BufferSize),
    EFI_SUCCESS
    );
  EXPECT_EQ (Buffer, (VOID *)NULL);
  EXPECT_EQ (BufferSize, 0u);
}

TEST_F (LoadSignatureDatabaseTest, VariablePresent_BufferAndSizePopulated) {
  static const UINT8  kPayload[] = { 0xAA, 0xBB, 0xCC, 0xDD };

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (
       Invoke (
         [] (
             IN CONST CHAR16    *Name,
             IN CONST EFI_GUID  *Guid,
             OUT      VOID      **Value,
             OUT      UINTN     *BufferSize
         ) -> EFI_STATUS {
    (VOID)Name;
    (VOID)Guid;
    *Value      = AllocateCopyPool (sizeof (kPayload), kPayload);
    *BufferSize = sizeof (kPayload);
    return EFI_SUCCESS;
  }
         )
       );

  VOID   *Buffer    = NULL;
  UINTN  BufferSize = 0;

  EXPECT_EQ (
    LoadSignatureDatabase ((const CHAR16 *)u"db", &Buffer, &BufferSize),
    EFI_SUCCESS
    );
  ASSERT_NE (Buffer, (VOID *)NULL);
  EXPECT_EQ (BufferSize, sizeof (kPayload));
  EXPECT_EQ (CompareMem (Buffer, kPayload, sizeof (kPayload)), 0);

  FreePool (Buffer);
}

TEST_F (LoadSignatureDatabaseTest, GetVariableUnexpectedError_PropagatedVerbatim) {
  // Errors other than EFI_NOT_FOUND must be reported unchanged.
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  VOID   *Buffer    = NULL;
  UINTN  BufferSize = 0;

  EXPECT_EQ (
    LoadSignatureDatabase ((const CHAR16 *)u"db", &Buffer, &BufferSize),
    EFI_DEVICE_ERROR
    );
}

// ---------------------------------------------------------------------------
// LoadSignatureDatabases (db + dbx)
// ---------------------------------------------------------------------------

class LoadSignatureDatabasesTest : public ::testing::Test {
protected:
  MockUefiLib UefiLibMock;
};

//
// Lambda factory: a GetVariable2 action that allocates a copy of the
// supplied payload and returns EFI_SUCCESS. Used to feed synthetic
// db/dbx buffers into LoadSignatureDatabases through the mock.
//
static auto
ReturnVariablePayload (
  const UINT8  *Payload,
  size_t       PayloadSize
  )
{
  return Invoke (
           [Payload, PayloadSize] (
                                   IN CONST CHAR16    *Name,
                                   IN CONST EFI_GUID  *Guid,
                                   OUT      VOID      **Value,
                                   OUT      UINTN     *BufferSize
           ) -> EFI_STATUS {
    (VOID)Name;
    (VOID)Guid;
    *Value      = AllocateCopyPool (PayloadSize, Payload);
    *BufferSize = PayloadSize;
    return EFI_SUCCESS;
  }
           );
}

TEST_F (LoadSignatureDatabasesTest, NullDatabases_ReturnsInvalidParameter) {
  EXPECT_EQ (
    LoadSignatureDatabases (NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (LoadSignatureDatabasesTest, BothVariablesMissing_Success) {
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_NOT_FOUND))   // db
    .WillOnce (Return (EFI_NOT_FOUND));  // dbx

  // Pre-set to bogus values: must be cleared.
  SIGNATURE_DATABASES  Databases = {
    (VOID *)(UINTN)0xDEADBEEF, 0xAA,
    (VOID *)(UINTN)0xCAFEF00D, 0xBB
  };

  EXPECT_EQ (
    LoadSignatureDatabases (&Databases),
    EFI_SUCCESS
    );
  EXPECT_EQ (Databases.Db, (VOID *)NULL);
  EXPECT_EQ (Databases.DbSize, 0u);
  EXPECT_EQ (Databases.Dbx, (VOID *)NULL);
  EXPECT_EQ (Databases.DbxSize, 0u);
}

TEST_F (LoadSignatureDatabasesTest, OnlyDbPresent_DbAllocated) {
  std::vector<UINT8>  DbBuf;

  AppendSignatureList (DbBuf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (DbBuf.data (), DbBuf.size ()))  // db
    .WillOnce (Return (EFI_NOT_FOUND));                               // dbx

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadSignatureDatabases (&Databases),
    EFI_SUCCESS
    );
  ASSERT_NE (Databases.Db, (VOID *)NULL);
  EXPECT_EQ (Databases.DbSize, DbBuf.size ());
  EXPECT_EQ (Databases.Dbx, (VOID *)NULL);
  EXPECT_EQ (Databases.DbxSize, 0u);
  FreePool (Databases.Db);
}

TEST_F (LoadSignatureDatabasesTest, OnlyDbxPresent_DbxAllocated) {
  std::vector<UINT8>  DbxBuf;

  AppendSignatureList (DbxBuf, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_NOT_FOUND))                                  // db
    .WillOnce (ReturnVariablePayload (DbxBuf.data (), DbxBuf.size ())); // dbx

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadSignatureDatabases (&Databases),
    EFI_SUCCESS
    );
  EXPECT_EQ (Databases.Db, (VOID *)NULL);
  EXPECT_EQ (Databases.DbSize, 0u);
  ASSERT_NE (Databases.Dbx, (VOID *)NULL);
  EXPECT_EQ (Databases.DbxSize, DbxBuf.size ());
  FreePool (Databases.Dbx);
}

TEST_F (LoadSignatureDatabasesTest, BothPresent_BuffersAllocated) {
  std::vector<UINT8>  DbBuf;
  std::vector<UINT8>  DbxBuf;

  AppendSignatureList (DbBuf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  AppendSignatureList (DbxBuf, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (DbBuf.data (), DbBuf.size ()))    // db
    .WillOnce (ReturnVariablePayload (DbxBuf.data (), DbxBuf.size ())); // dbx

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadSignatureDatabases (&Databases),
    EFI_SUCCESS
    );
  ASSERT_NE (Databases.Db, (VOID *)NULL);
  EXPECT_EQ (Databases.DbSize, DbBuf.size ());
  ASSERT_NE (Databases.Dbx, (VOID *)NULL);
  EXPECT_EQ (Databases.DbxSize, DbxBuf.size ());
  FreePool (Databases.Db);
  FreePool (Databases.Dbx);
}

TEST_F (LoadSignatureDatabasesTest, DbLoadFails_ErrorPropagatedNothingAllocated) {
  // The db lookup fails with a non-NOT_FOUND status; dbx must not even
  // be attempted, and both out-pointers must be NULL.
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadSignatureDatabases (&Databases),
    EFI_DEVICE_ERROR
    );
  EXPECT_EQ (Databases.Db, (VOID *)NULL);
  EXPECT_EQ (Databases.DbSize, 0u);
  EXPECT_EQ (Databases.Dbx, (VOID *)NULL);
  EXPECT_EQ (Databases.DbxSize, 0u);
}

TEST_F (LoadSignatureDatabasesTest, DbxLoadFails_DbFreedAndErrorPropagated) {
  std::vector<UINT8>  DbBuf;

  AppendSignatureList (DbBuf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  // db succeeds (allocation must be cleaned up internally); dbx fails.
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (DbBuf.data (), DbBuf.size ()))  // db
    .WillOnce (Return (EFI_DEVICE_ERROR));                            // dbx

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadSignatureDatabases (&Databases),
    EFI_DEVICE_ERROR
    );
  EXPECT_EQ (Databases.Db, (VOID *)NULL);   // freed and nulled
  EXPECT_EQ (Databases.DbSize, 0u);
  EXPECT_EQ (Databases.Dbx, (VOID *)NULL);
  EXPECT_EQ (Databases.DbxSize, 0u);
}

TEST_F (LoadSignatureDatabasesTest, DbxLoadFailsWithAllocatedBuffer_DbxFreedAndNulled) {
  std::vector<UINT8>  DbBuf;

  AppendSignatureList (DbBuf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (DbBuf.data (), DbBuf.size ()))
    .WillOnce (
       Invoke (
         [] (
             IN CONST CHAR16    *Name,
             IN CONST EFI_GUID  *Guid,
             OUT      VOID      **Value,
             OUT      UINTN     *BufferSize
         ) -> EFI_STATUS {
    (VOID)Name;
    (VOID)Guid;
    *Value      = AllocatePool (8);
    *BufferSize = 8;
    return EFI_DEVICE_ERROR;
  }
         )
       );

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadSignatureDatabases (&Databases),
    EFI_DEVICE_ERROR
    );
  EXPECT_EQ (Databases.Db, (VOID *)NULL);
  EXPECT_EQ (Databases.DbSize, 0u);
  EXPECT_EQ (Databases.Dbx, (VOID *)NULL);
  EXPECT_EQ (Databases.DbxSize, 0u);
}

// ---------------------------------------------------------------------------
// IsInDbx
// ---------------------------------------------------------------------------

//
// An unusable subject cache (NULL buffer) is treated as revoked (fail closed).
//
TEST (IsInDbxTest, UnusableCacheNullBuffer_ReturnsTrue) {
  std::vector<UINT8>  Dbx (32, 0);
  DIGEST_CACHE        Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = DigestCacheTypeX509;
  Cache.Buffer     = NULL;
  Cache.BufferSize = 4;

  EXPECT_TRUE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An unusable subject cache (zero size) is treated as revoked (fail closed).
//
TEST (IsInDbxTest, UnusableCacheZeroSize_ReturnsTrue) {
  UINT8               CertByte = 0x30;
  std::vector<UINT8>  Dbx (32, 0);
  DIGEST_CACHE        Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = DigestCacheTypeX509;
  Cache.Buffer     = &CertByte;
  Cache.BufferSize = 0;

  EXPECT_TRUE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// A NULL dbx means nothing is revoked.
//
TEST (IsInDbxTest, NullDbx_ReturnsFalse) {
  std::vector<UINT8>  Cert  = { 0x30, 0x82 };
  DIGEST_CACHE        Cache = MakeCertCache (Cert);

  EXPECT_FALSE (IsInDbx (&Cache, NULL, 0));
}

//
// An empty dbx means nothing is revoked.
//
TEST (IsInDbxTest, EmptyDbx_ReturnsFalse) {
  std::vector<UINT8>  Cert = { 0x30, 0x82 };
  std::vector<UINT8>  Dbx (32, 0);
  DIGEST_CACHE        Cache = MakeCertCache (Cert);

  EXPECT_FALSE (IsInDbx (&Cache, Dbx.data (), 0));
}

//
// A dbx too small to contain a signature-list header must fail closed.
//
TEST (IsInDbxTest, MalformedDbx_ReturnsTrue) {
  std::vector<UINT8>  Cert = { 0x30, 0x82 };
  std::vector<UINT8>  Dbx (4, 0);
  DIGEST_CACHE        Cache = MakeCertCache (Cert);

  EXPECT_TRUE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An EFI_CERT_X509 list whose entry payload matches the certificate
// byte-for-byte (same length) marks the certificate as revoked.
//
TEST (IsInDbxTest, X509ExactMatch_ReturnsTrue) {
  std::vector<UINT8>  Cert (16, 0x11);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, Off, 0, Cert);

  DIGEST_CACHE  Cache = MakeCertCache (Cert);

  EXPECT_TRUE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An EFI_CERT_X509 list whose entry payload differs from the
// certificate is not a match.
//
TEST (IsInDbxTest, X509BytesDiffer_ReturnsFalse) {
  std::vector<UINT8>  Cert (16, 0x11);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(16, 0x22));

  DIGEST_CACHE  Cache = MakeCertCache (Cert);

  EXPECT_FALSE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An EFI_CERT_X509 list whose entry payload size differs from the
// certificate size is skipped (not a match).
//
TEST (IsInDbxTest, X509PayloadSizeMismatch_ReturnsFalse) {
  std::vector<UINT8>  Cert (16, 0x11);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 32), 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(32, 0x11));

  DIGEST_CACHE  Cache = MakeCertCache (Cert);

  EXPECT_FALSE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An EFI_CERT_X509_SHA256 list whose entry holds the certificate's TBS
// hash marks it revoked. The TBS hash is produced by the mocked
// X509GetTbsCertHash.
//
TEST (IsInDbxTest, TbsHashMatch_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Cert (8, 0x30);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(kSha256DigestSize, 0xE1));

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Digest, UINTN *DigestSize) -> EFI_STATUS {
    std::memset (Digest, 0xE1, SHA256_DIGEST_SIZE);
    *DigestSize = SHA256_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  DIGEST_CACHE  Cache = MakeCertCache (Cert);

  EXPECT_TRUE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An EFI_CERT_X509_SHA256 list whose entry digest differs from the
// certificate's TBS hash is not a match.
//
TEST (IsInDbxTest, TbsHashDiffers_ReturnsFalse) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Cert (8, 0x30);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(kSha256DigestSize, 0xAA));

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Digest, UINTN *DigestSize) -> EFI_STATUS {
    std::memset (Digest, 0x77, SHA256_DIGEST_SIZE);
    *DigestSize = SHA256_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  DIGEST_CACHE  Cache = MakeCertCache (Cert);

  EXPECT_FALSE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// A list whose type is neither EFI_CERT_X509 nor a supported
// X509-cert-hash type is skipped; X509GetTbsCertHash is never called.
//
TEST (IsInDbxTest, UnsupportedListType_ReturnsFalse) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Cert (8, 0x30);
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _)).Times (0);

  DIGEST_CACHE  Cache = MakeCertCache (Cert);

  EXPECT_FALSE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// If computing the certificate's TBS hash fails for a cert-hash list,
// the search fails closed (returns TRUE).
//
TEST (IsInDbxTest, TbsHashComputeFails_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Cert (8, 0x30);
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  DIGEST_CACHE  Cache = MakeCertCache (Cert);

  EXPECT_TRUE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An image subject whose digest is enrolled in the dbx is revoked.
//
TEST (IsInDbxTest, ImageHashMatch_ReturnsTrue) {
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xC3);

  SetEntryPayload (Dbx, Off, 0, Digest);

  DIGEST_CACHE  Cache = MakeBoundCache (&gEfiCertSha256Guid, Digest);

  EXPECT_TRUE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An image subject whose digest is not in the dbx is not revoked.
//
TEST (IsInDbxTest, ImageHashNoMatch_ReturnsFalse) {
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(kSha256DigestSize, 0xC3));

  std::vector<UINT8>  Digest (kSha256DigestSize, 0xD4);
  DIGEST_CACHE        Cache = MakeBoundCache (&gEfiCertSha256Guid, Digest);

  EXPECT_FALSE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

//
// A valid non-matching list followed by a malformed trailing fragment fails
// closed: even though the subject is not in the valid prefix, the dropped tail
// might have matched it.
//
TEST (IsInDbxTest, MalformedTail_FailsClosed) {
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(kSha256DigestSize, 0x01));

  // Append a stray fragment too small to be a list header.
  Dbx.resize (Dbx.size () + sizeof (EFI_SIGNATURE_LIST) - 1, 0);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0x99);   // not the enrolled entry
  DIGEST_CACHE        Cache = MakeBoundCache (&gEfiCertSha256Guid, Digest);

  EXPECT_TRUE (IsInDbx (&Cache, Dbx.data (), Dbx.size ()));
}

// ---------------------------------------------------------------------------
// IsChainRevoked
// ---------------------------------------------------------------------------

TEST (IsChainRevokedTest, NullAuthData_ReturnsTrue) {
  std::vector<UINT8>  Dbx (32, 0);

  EXPECT_TRUE (
    IsChainRevoked (
      NULL,
      16,
      Dbx.data (),
      Dbx.size ()
      )
    );
}

TEST (IsChainRevokedTest, ZeroAuthDataSize_ReturnsTrue) {
  std::vector<UINT8>  Dbx (32, 0);

  EXPECT_TRUE (
    IsChainRevoked (
      kAuthDataDefault.data (),
      0,
      Dbx.data (),
      Dbx.size ()
      )
    );
}

TEST (IsChainRevokedTest, NullSignerCert_ReturnsTrue) {
  std::vector<UINT8>  Dbx (32, 0);

  EXPECT_TRUE (
    IsChainRevoked (
      NULL,
      8,
      Dbx.data (),
      Dbx.size ()
      )
    );
}

TEST (IsChainRevokedTest, NullAnchor_ReturnsTrue) {
  std::vector<UINT8>  Dbx (32, 0);

  EXPECT_TRUE (
    IsChainRevoked (
      NULL,
      8,
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// With valid inputs but no dbx, nothing is revoked and the chain is
// never built.
//
TEST (IsChainRevokedTest, NullDbx_ReturnsFalse) {
  std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(8, 0x22) });

  EXPECT_FALSE (
    IsChainRevoked (
      Stack.data (),
      Stack.size (),
      NULL,
      0
      )
    );
}

TEST (IsChainRevokedTest, EmptyDbx_ReturnsFalse) {
  std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(8, 0x22) });
  std::vector<UINT8>  Dbx;

  EXPECT_FALSE (
    IsChainRevoked (
      Stack.data (),
      Stack.size (),
      Dbx.data (),
      0
      )
    );
}

//
// If the certificate chain cannot be built, the helper fails closed.
//
TEST (IsChainRevokedTest, ChainBuildError_ReturnsTrue) {
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  EXPECT_TRUE (
    IsChainRevoked (
      NULL,
      0,
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// A well-formed but empty (zero-length) chain fails closed.
//
TEST (IsChainRevokedTest, EmptyChain_ReturnsTrue) {
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  EXPECT_TRUE (
    IsChainRevoked (
      NULL,
      0,
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// A chain cert that is listed in dbx (exact DER match) revokes the chain.
//
TEST (IsChainRevokedTest, ChainCertInDbx_ReturnsTrue) {
  std::vector<UINT8>  ChainCert (16, 0x11);
  std::vector<UINT8>  Dbx;

  static std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(16, 0x11) });

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, Off, 0, ChainCert);

  EXPECT_TRUE (
    IsChainRevoked (
      Stack.data (),
      Stack.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// A chain whose certs are not in dbx is not revoked.
//
TEST (IsChainRevokedTest, ChainCertNotInDbx_ReturnsFalse) {
  std::vector<UINT8>  Dbx;

  static std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(16, 0x11) });

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(16, 0x22));

  EXPECT_FALSE (
    IsChainRevoked (
      Stack.data (),
      Stack.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// A malformed chain (declares more certs than the buffer holds) fails
// closed.
//
TEST (IsChainRevokedTest, MalformedChain_ReturnsTrue) {
  std::vector<UINT8>  Dbx;

  // CertNumber = 2, but only one 4-byte cert is present.
  static std::vector<UINT8>  BadStack = {
    0x02, 0x04, 0x00, 0x00, 0x00, 0xAA, 0xBB, 0xCC, 0xDD
  };

  // X509 list whose payload size (16) will not match the 4-byte cert, so
  // the first cert is skipped and the walk reaches the truncation.
  AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  EXPECT_TRUE (
    IsChainRevoked (
      BadStack.data (),
      BadStack.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

// ---------------------------------------------------------------------------
// ExtractAuthData
// ---------------------------------------------------------------------------

TEST (ExtractAuthDataTest, NullCert_ReturnsInvalidParameter) {
  const UINT8  *AuthData    = NULL;
  UINTN        AuthDataSize = 0;

  EXPECT_EQ (
    ExtractAuthData (NULL, &AuthData, &AuthDataSize),
    EFI_INVALID_PARAMETER
    );
}

TEST (ExtractAuthDataTest, NullOutputs_ReturnsInvalidParameter) {
  WIN_CERTIFICATE  Cert         = { sizeof (WIN_CERTIFICATE) + 1, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA };
  const UINT8      *AuthData    = NULL;
  UINTN            AuthDataSize = 0;

  EXPECT_EQ (
    ExtractAuthData (&Cert, NULL, &AuthDataSize),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    ExtractAuthData (&Cert, &AuthData, NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST (ExtractAuthDataTest, PkcsSignedData_ExtractsPayload) {
  // Build a WIN_CERTIFICATE followed by 4 bytes of payload.
  const UINT8         Payload[] = { 0xAA, 0xBB, 0xCC, 0xDD };
  std::vector<UINT8>  Buffer (sizeof (WIN_CERTIFICATE) + sizeof (Payload), 0);
  WIN_CERTIFICATE     *Cert = (WIN_CERTIFICATE *)Buffer.data ();

  Cert->dwLength         = (UINT32)Buffer.size ();
  Cert->wRevision        = 0x0200;
  Cert->wCertificateType = WIN_CERT_TYPE_PKCS_SIGNED_DATA;
  std::memcpy (Buffer.data () + sizeof (WIN_CERTIFICATE), Payload, sizeof (Payload));

  const UINT8  *AuthData    = NULL;
  UINTN        AuthDataSize = 0;

  EXPECT_EQ (
    ExtractAuthData (Cert, &AuthData, &AuthDataSize),
    EFI_SUCCESS
    );
  ASSERT_EQ (AuthDataSize, sizeof (Payload));
  EXPECT_EQ (0, std::memcmp (AuthData, Payload, sizeof (Payload)));
}

TEST (ExtractAuthDataTest, PkcsSignedData_HeaderOnly_ReturnsCorrupted) {
  WIN_CERTIFICATE  Cert         = { sizeof (WIN_CERTIFICATE), 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA };
  const UINT8      *AuthData    = NULL;
  UINTN            AuthDataSize = 0;

  EXPECT_EQ (
    ExtractAuthData (&Cert, &AuthData, &AuthDataSize),
    EFI_VOLUME_CORRUPTED
    );
}

TEST (ExtractAuthDataTest, EfiGuidPkcs7_ExtractsPayload) {
  const UINT8                Payload[]  = { 0x11, 0x22, 0x33 };
  const size_t               HeaderSize = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  std::vector<UINT8>         Buffer (HeaderSize + sizeof (Payload), 0);
  WIN_CERTIFICATE_UEFI_GUID  *UefiCert = (WIN_CERTIFICATE_UEFI_GUID *)Buffer.data ();

  UefiCert->Hdr.dwLength         = (UINT32)Buffer.size ();
  UefiCert->Hdr.wRevision        = 0x0200;
  UefiCert->Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyMem (&UefiCert->CertType, &gEfiCertPkcs7Guid, sizeof (EFI_GUID));
  std::memcpy (Buffer.data () + HeaderSize, Payload, sizeof (Payload));

  const UINT8  *AuthData    = NULL;
  UINTN        AuthDataSize = 0;

  EXPECT_EQ (
    ExtractAuthData (&UefiCert->Hdr, &AuthData, &AuthDataSize),
    EFI_SUCCESS
    );
  ASSERT_EQ (AuthDataSize, sizeof (Payload));
  EXPECT_EQ (0, std::memcmp (AuthData, Payload, sizeof (Payload)));
}

TEST (ExtractAuthDataTest, EfiGuidNonPkcs7_ReturnsUnsupported) {
  const size_t               HeaderSize = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  std::vector<UINT8>         Buffer (HeaderSize + 4, 0);
  WIN_CERTIFICATE_UEFI_GUID  *UefiCert = (WIN_CERTIFICATE_UEFI_GUID *)Buffer.data ();
  const EFI_GUID             OtherGuid = { 0x12345678, 0x1234, 0x1234, { 1, 2, 3, 4, 5, 6, 7, 8 }
  };

  UefiCert->Hdr.dwLength         = (UINT32)Buffer.size ();
  UefiCert->Hdr.wRevision        = 0x0200;
  UefiCert->Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyMem (&UefiCert->CertType, &OtherGuid, sizeof (EFI_GUID));

  const UINT8  *AuthData    = NULL;
  UINTN        AuthDataSize = 0;

  EXPECT_EQ (
    ExtractAuthData (&UefiCert->Hdr, &AuthData, &AuthDataSize),
    EFI_UNSUPPORTED
    );
}

TEST (ExtractAuthDataTest, EfiGuid_HeaderOnly_ReturnsCorrupted) {
  WIN_CERTIFICATE_UEFI_GUID  UefiCert;

  ZeroMem (&UefiCert, sizeof (UefiCert));
  UefiCert.Hdr.dwLength         = (UINT32)OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  UefiCert.Hdr.wRevision        = 0x0200;
  UefiCert.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;

  const UINT8  *AuthData    = NULL;
  UINTN        AuthDataSize = 0;

  EXPECT_EQ (
    ExtractAuthData (&UefiCert.Hdr, &AuthData, &AuthDataSize),
    EFI_VOLUME_CORRUPTED
    );
}

TEST (ExtractAuthDataTest, UnknownCertType_ReturnsUnsupported) {
  WIN_CERTIFICATE  Cert         = { sizeof (WIN_CERTIFICATE) + 8, 0x0200, WIN_CERT_TYPE_EFI_PKCS115 };
  const UINT8      *AuthData    = NULL;
  UINTN            AuthDataSize = 0;

  EXPECT_EQ (
    ExtractAuthData (&Cert, &AuthData, &AuthDataSize),
    EFI_UNSUPPORTED
    );
}

// ---------------------------------------------------------------------------
// EvaluateImageCertificate -- end-to-end PKCS#7 + database scenarios
// ---------------------------------------------------------------------------

//
// Install the mock expectations shared by every db-walk scenario: the image
// hash algorithm resolves to SHA-256 and the image hash is produced.
//
static void
ExpectSignedImagePrelude (
  MockBaseCryptLib  &BaseCryptLibMock
  )
{
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *Out) -> EFI_STATUS {
    CopyMem (Out, &gEfiCertSha256Guid, sizeof (EFI_GUID));
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Out, UINTN *OutSize) -> EFI_STATUS {
    SetMem (Out, kSha256DigestSize, 0x55);
    *OutSize = kSha256DigestSize;
    return EFI_SUCCESS;
  }
         )
       );
}

//
// A NULL certificate pointer is rejected up front with EFI_INVALID_PARAMETER;
// no crypto is consulted and no verdict is produced.
//
TEST (EvaluateImageCertificateTest, NullCert_ReturnsInvalidParameter) {
  DIGEST_CACHE           Cache;
  SIGNATURE_DATABASES    Databases = { NULL, 0, NULL, 0 };
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_EQ (EvaluateImageCertificate (NULL, &Cache, &Databases, &Eval), EFI_INVALID_PARAMETER);
}

//
// A NULL digest cache is rejected up front with EFI_INVALID_PARAMETER.
//
TEST (EvaluateImageCertificateTest, NullCache_ReturnsInvalidParameter) {
  std::vector<UINT8>     CertBuf   = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_DATABASES    Databases = { NULL, 0, NULL, 0 };
  IMAGE_CERT_EVALUATION  Eval;

  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      NULL,
      &Databases,
      &Eval
      ),
    EFI_INVALID_PARAMETER
    );
}

//
// A NULL databases pointer is rejected up front with EFI_INVALID_PARAMETER.
//
TEST (EvaluateImageCertificateTest, NullDatabases_ReturnsInvalidParameter) {
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  DIGEST_CACHE           Cache;
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      NULL,
      &Eval
      ),
    EFI_INVALID_PARAMETER
    );
}

//
// A NULL evaluation output pointer is rejected up front with
// EFI_INVALID_PARAMETER.
//
TEST (EvaluateImageCertificateTest, NullEvaluation_ReturnsInvalidParameter) {
  std::vector<UINT8>   CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  DIGEST_CACHE         Cache;
  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };

  InitImageCache (Cache);

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      NULL
      ),
    EFI_INVALID_PARAMETER
    );
}

//
// An unknown WIN_CERTIFICATE type cannot be parsed into a PKCS#7 payload, so
// the certificate is unusable. No crypto is consulted and no anchor is
// inspected.
//
TEST (EvaluateImageCertificateTest, UnsupportedCertType_Unusable) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  WIN_CERTIFICATE        Cert      = { sizeof (WIN_CERTIFICATE) + 8, 0x0200, WIN_CERT_TYPE_EFI_PKCS115 };
  SIGNATURE_DATABASES    Databases = { NULL, 0, NULL, 0 };
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (EvaluateImageCertificate (&Cert, &Cache, &Databases, &Eval), EFI_SUCCESS);
  EXPECT_EQ (Eval.Verdict, ImageCertUnusable);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// GetAuthenticodeHashAlgorithm fails, so the image-hash algorithm cannot be
// determined and the certificate is unusable. The image hash is never computed
// and no anchor is verified.
//
TEST (EvaluateImageCertificateTest, HashAlgorithmFails_Unusable) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf   = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_DATABASES    Databases = { NULL, 0, NULL, 0 };
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (Return (EFI_UNSUPPORTED));
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertUnusable);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// The image hash cannot be computed: GetAuthenticodeHash fails, GetHash
// surfaces EFI_SECURITY_VIOLATION, and EvaluateImageCertificate propagates it.
// This is the only non-INVALID_PARAMETER error path; no verdict is asserted.
//
TEST (EvaluateImageCertificateTest, ImageHashFails_ReturnsError) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf   = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_DATABASES    Databases = { NULL, 0, NULL, 0 };
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *Out) -> EFI_STATUS {
    CopyMem (Out, &gEfiCertSha256Guid, sizeof (EFI_GUID));
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _))
    .WillOnce (Return (EFI_SECURITY_VIOLATION));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SECURITY_VIOLATION
    );
}

//
// Signer extraction is no longer a separate prerequisite. With an empty db,
// evaluation completes without invoking a verifier.
//
TEST (EvaluateImageCertificateTest, SignerExtractionNotRequired_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf   = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_DATABASES    Databases = { NULL, 0, NULL, 0 };
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *Out) -> EFI_STATUS {
    CopyMem (Out, &gEfiCertSha256Guid, sizeof (EFI_GUID));
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Out, UINTN *OutSize) -> EFI_STATUS {
    SetMem (Out, kSha256DigestSize, 0x55);
    *OutSize = kSha256DigestSize;
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// The evaluator never calls the legacy signer-extraction API.
//
TEST (EvaluateImageCertificateTest, LegacySignerApiNotCalled_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf   = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_DATABASES    Databases = { NULL, 0, NULL, 0 };
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *Out) -> EFI_STATUS {
    CopyMem (Out, &gEfiCertSha256Guid, sizeof (EFI_GUID));
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Out, UINTN *OutSize) -> EFI_STATUS {
    SetMem (Out, kSha256DigestSize, 0x55);
    *OutSize = kSha256DigestSize;
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (_)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// The prelude succeeds and a signer is available, but the db is empty. No
// anchor matches, so the verdict is ImageCertNotInDb and AuthenticodeVerifyEx is
// never called.
//
TEST (EvaluateImageCertificateTest, EmptyDb_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf   = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_DATABASES    Databases = { NULL, 0, NULL, 0 };
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// A single EFI_CERT_X509 db trust anchor verifies the image and there is no
// dbx (so no chain is built). The image is approved and the authority points
// at the matching db entry.
//
TEST (EvaluateImageCertificateTest, X509VerifiesNoDbx_Approved) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertApproved);
  EXPECT_NE (Eval.Authority.Data, nullptr);
  EXPECT_EQ (Eval.Authority.Size, (UINTN)(sizeof (EFI_GUID) + 16));
}

//
// A V2 (EFI_SIGNATURE_V2_DATA) full-certificate db anchor carries no owner GUID; the whole entry
// is the DER certificate handed to AuthenticodeVerifyEx. The recorded authority normalizes it to a
// V1 EFI_SIGNATURE_DATA by prepending a zeroed owner GUID, so its size is the certificate plus a
// SignatureOwner.
//
TEST (EvaluateImageCertificateTest, V2X509VerifiesNoDbx_Approved) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertV2X509Guid, 0, 16, 1);

  SetV2EntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertApproved);
  EXPECT_NE (Eval.Authority.Data, nullptr);
  // The V2 anchor carries no owner GUID, so the authority prepends a zeroed one to the certificate.
  EXPECT_EQ (Eval.Authority.Size, (UINTN)(sizeof (EFI_GUID) + 16));
  FreeImageAuthority (&Eval.Authority);
}

//
// A V2 (EFI_SIGNATURE_V2_DATA) TBS-cert-hash db entry holds only the hash (no owner GUID, no v1
// TimeOfRevocation). The full hash region is passed to GetTrustAnchorX509FromAuthData; this asserts
// the size passed reflects the ownerless entry.
//
TEST (EvaluateImageCertificateTest, V2X509HashListResolvesAnchor_Approved) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertV2X509Sha256Guid, 0, kSha256V2EntrySize, 1);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID **CacheHandle, CONST UINT8 *, UINTN TbsHashSize, CONST UINT8 *, UINTN, UINT8 **TrustAnchor, UINTN *TrustAnchorSize) -> EFI_STATUS {
    // The V2 entry supplies exactly the hash payload (SignatureSize with no owner subtracted).
    EXPECT_EQ (TbsHashSize, (UINTN)kSha256V2EntrySize);
    static const UINT8  CertBytes[] = { 0x30, 0x82, 0x01, 0x02 };
    *TrustAnchor                    = (UINT8 *)AllocateCopyPool (sizeof (CertBytes), CertBytes);
    *TrustAnchorSize                = sizeof (CertBytes);
    if (CacheHandle != NULL) {
      *CacheHandle = (VOID *)(UINTN)1;
    }

    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));
  EXPECT_CALL (BaseCryptLibMock, FreeTrustAnchorX509Cache (_)).Times (1);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertApproved);
  EXPECT_NE (Eval.Authority.Data, nullptr);
}

//
// A single EFI_CERT_X509 trust anchor is present but AuthenticodeVerifyEx rejects
// the image. No anchor authorizes it, so the verdict is ImageCertNotInDb.
//
TEST (EvaluateImageCertificateTest, X509DoesNotVerify_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SECURITY_VIOLATION));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// An EFI_CERT_X509 anchor verifies the image, but a certificate in the signer's
// chain is enrolled in dbx. The anchor is verified-but-revoked and, with no
// other anchor, the verdict is ImageCertRevokedByDbx and the authority names the
// revoking dbx entry.
//
TEST (EvaluateImageCertificateTest, X509VerifiesChainRevoked_RevokedByDbx) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  // dbx: one X509 list holding the (20-byte) chain cert exactly.
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 20), 1);

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(20, 0xAB));

  static std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(20, 0xAB) });

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, UINT8 **OutChain, UINTN *OutChainSize) -> EFI_STATUS {
    *OutChain     = (UINT8 *)AllocateCopyPool (Stack.size (), Stack.data ());
    *OutChainSize = Stack.size ();
    return EFI_SUCCESS;
  }
         )
       );

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertRevokedByDbx);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
  EXPECT_EQ (Eval.Authority.Size, (UINTN)0);
}

//
// An EFI_CERT_X509 anchor verifies the image and none of the signer's chain
// certs are in dbx. The image is approved.
//
TEST (EvaluateImageCertificateTest, X509VerifiesChainClean_Approved) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  // dbx holds an unrelated cert, so the chain is clean.
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(16, 0x22));

  static std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(16, 0x33) });

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, UINT8 **OutChain, UINTN *OutChainSize) -> EFI_STATUS {
    *OutChain     = (UINT8 *)AllocateCopyPool (Stack.size (), Stack.data ());
    *OutChainSize = Stack.size ();
    return EFI_SUCCESS;
  }
         )
       );

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertApproved);
  EXPECT_NE (Eval.Authority.Data, nullptr);
}

//
// db holds an EFI_CERT_X509_SHA256 (TBS-cert-hash) list. A trust anchor is
// recovered from the auth data, it authenticates the image, and there is no
// dbx. The image is approved; the resolver's cache handle is released via
// FreeTrustAnchorX509Cache.
//
TEST (EvaluateImageCertificateTest, X509HashListResolvesAnchor_Approved) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256TbsV1EntrySize, 1);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID **CacheHandle, CONST UINT8 *, UINTN TbsHashSize, CONST UINT8 *, UINTN, UINT8 **TrustAnchor, UINTN *TrustAnchorSize) -> EFI_STATUS {
    // The V1 entry appends an EFI_TIME after the hash; only the 32-byte SHA-256 hash is passed.
    EXPECT_EQ (TbsHashSize, (UINTN)kSha256DigestSize);
    static const UINT8  CertBytes[] = { 0x30, 0x82, 0x01, 0x02 };
    *TrustAnchor                    = (UINT8 *)AllocateCopyPool (sizeof (CertBytes), CertBytes);
    *TrustAnchorSize                = sizeof (CertBytes);
    if (CacheHandle != NULL) {
      *CacheHandle = (VOID *)(UINTN)1;
    }

    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));
  EXPECT_CALL (BaseCryptLibMock, FreeTrustAnchorX509Cache (_)).Times (1);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertApproved);
  EXPECT_NE (Eval.Authority.Data, nullptr);
}

//
// db holds an EFI_CERT_X509_SHA256 list with two entries, but neither recovers
// a trust anchor from the auth data (EFI_NOT_FOUND). Both entries are walked
// and no anchor authorizes: ImageCertNotInDb.
//
TEST (EvaluateImageCertificateTest, X509HashListAllNotFound_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256TbsV1EntrySize, 2);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .Times (2)
    .WillRepeatedly (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// db holds an EFI_CERT_X509_SHA256 list; the trust-anchor lookup fails with a
// hard error (not EFI_NOT_FOUND). The entry is skipped and no anchor
// authorizes: ImageCertNotInDb.
//
TEST (EvaluateImageCertificateTest, X509HashListHardError_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256TbsV1EntrySize, 1);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// Two EFI_CERT_X509 anchors: the first verifies but its chain is revoked by
// dbx; the second verifies with a clean chain. The image is approved via the
// second anchor.
//
TEST (EvaluateImageCertificateTest, TwoAnchorsFirstRevokedSecondClean_Approved) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  // db: one X509 list with two anchors (payload 0x11 and 0x22).
  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 2);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));
  SetEntryPayload (Db, DbOff, 1, std::vector<UINT8>(16, 0x22));

  // dbx: holds the revoked chain cert (24 bytes 0xDD).
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 24), 1);

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(24, 0xDD));

  static std::vector<UINT8>  RevokedStack = MakeCertStack ({ std::vector<UINT8>(24, 0xDD) });
  static std::vector<UINT8>  CleanStack   = MakeCertStack ({ std::vector<UINT8>(24, 0xEE) });

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, UINT8 **OutChain, UINTN *OutChainSize) -> EFI_STATUS {
    *OutChain     = (UINT8 *)AllocateCopyPool (RevokedStack.size (), RevokedStack.data ());
    *OutChainSize = RevokedStack.size ();
    return EFI_SUCCESS;
  }
         )
       )
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, UINT8 **OutChain, UINTN *OutChainSize) -> EFI_STATUS {
    *OutChain     = (UINT8 *)AllocateCopyPool (CleanStack.size (), CleanStack.data ());
    *OutChainSize = CleanStack.size ();
    return EFI_SUCCESS;
  }
         )
       );

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertApproved);
  EXPECT_NE (Eval.Authority.Data, nullptr);
}

//
// db contains only a non-X.509 (image-hash) list. It is skipped and, with no
// trust anchors, the verdict is ImageCertNotInDb.
//
TEST (EvaluateImageCertificateTest, DbHasOnlyNonX509List_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// db contains an EFI_CERT_X509 list whose SignatureSize equals sizeof(EFI_GUID)
// (no cert payload). The list is skipped and the verdict is ImageCertNotInDb.
//
TEST (EvaluateImageCertificateTest, DbX509ListNoCertPayload_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)sizeof (EFI_GUID), 1);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// db is malformed (smaller than one EFI_SIGNATURE_LIST header) so
// DatabaseIterInit truncates it to an empty range. The verdict is
// ImageCertNotInDb.
//
TEST (EvaluateImageCertificateTest, MalformedDb_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db (4, 0);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// Happy path: the prelude succeeds, the single db trust anchor verifies via
// AuthenticodeVerifyEx, and the empty dbx makes the chain check pass. The image
// is approved with a non-NULL authority whose SignatureType is the authorizing
// db list's type.
//
TEST (EvaluateImageCertificateTest, AuthorizedByDb_Approved) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertApproved);
  EXPECT_NE (Eval.Authority.Data, nullptr);
  EXPECT_EQ (Eval.Authority.Size, (UINTN)(sizeof (EFI_GUID) + 16));
  EXPECT_TRUE (CompareGuid (&Eval.Authority.SignatureType, &gEfiCertX509Guid));
}

//
// A db anchor verifies the image, but a certificate in its verified chain is
// enrolled in the dbx, so the chain is revoked. With no other anchor the
// verdict is ImageCertRevokedByDbx and no authority is recorded (Data is NULL,
// Size is 0, and SignatureType stays zeroed).
//
TEST (EvaluateImageCertificateTest, RevokedByDbx_RevokedByDbx) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  // db: one X509 trust anchor that verifies the image.
  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  // dbx: one X509 list holding the (20-byte) chain cert exactly.
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 20), 1);

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(20, 0xAB));

  static std::vector<UINT8>  RevokedChain = MakeCertStack ({ std::vector<UINT8>(20, 0xAB) });

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, UINT8 **OutChain, UINTN *OutChainSize) -> EFI_STATUS {
    *OutChain     = (UINT8 *)AllocateCopyPool (RevokedChain.size (), RevokedChain.data ());
    *OutChainSize = RevokedChain.size ();
    return EFI_SUCCESS;
  }
         )
       );

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertRevokedByDbx);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
  EXPECT_EQ (Eval.Authority.Size, (UINTN)0);
}

//
// The prelude succeeds, the dbx is empty (nothing revoked), but no db anchor
// verifies the signature. The verdict is ImageCertNotInDb.
//
TEST (EvaluateImageCertificateTest, NotRevokedNotAuthorized_NotInDb) {
  MockBaseCryptLib       BaseCryptLibMock;
  DIGEST_CACHE           Cache;
  std::vector<UINT8>     CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_CERT_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 2);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));
  SetEntryPayload (Db, DbOff, 1, std::vector<UINT8>(16, 0x22));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .Times (2)
    .WillRepeatedly (Return (EFI_SECURITY_VIOLATION));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluateImageCertificate (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageCertNotInDb);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}
