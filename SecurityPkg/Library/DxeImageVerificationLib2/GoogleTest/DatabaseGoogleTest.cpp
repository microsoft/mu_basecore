/** @file
  Unit tests for the signature-database helpers in
  DxeImageVerificationLib (Database.c): IsImageHashInAllowList, IsImageHashInRevokeList,
  IsTbsHashInAllowList, IsTbsHashInRevokeList, IsCertInRevokeList,
  LoadSignatureDatabase, LoadDbAndDbx,
  IsChainRevoked, and
  EvaluateSignature. ExtractSignatureData from Support.c is also covered. The
  database helpers are exercised against
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
#include <functional>

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
    IN  CONST VOID   *RevokeList,
    IN  UINTN        RevokeListSize
    );
}

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;

//
// A gmock action for the HashAllByGuid () mock: copy Digest into the output digest buffer and report
// success, standing in for a real hash of the subject buffer.
//
static std::function<EFI_STATUS (CONST EFI_GUID *, CONST VOID *, UINTN, UINT8 *, UINTN *)>
EmitDigest (
  const std::vector<UINT8>  &Digest
  )
{
  return [Digest](CONST EFI_GUID *HashType, CONST VOID *Buffer, UINTN BufferSize, UINT8 *Out, UINTN *DigestSize) -> EFI_STATUS {
           (VOID)HashType;
           (VOID)Buffer;
           (VOID)BufferSize;
           CopyMem (Out, Digest.data (), Digest.size ());
           *DigestSize = Digest.size ();
           return EFI_SUCCESS;
  };
}

//
// A fixed non-zero buffer a subject digest cache is bound to. GetHash () forwards these bytes to the
// mocked HashAllByGuid (), whose action determines the resulting digest, so their contents are
// irrelevant.
//
static UINT8  mSubjectBuffer[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

//
// Bind a subject digest cache to mSubjectBuffer and arrange for its computed SHA-256 digest to be
// Digest. The caller enrolls the same bytes in an allow-list/revoke-list entry to force a
// hash-membership match (or a different Digest to deny one). Release the returned cache with
// FreeDigestCache ().
//
static DIGEST_CACHE
BindDigest (
  MockBaseCryptLib          &BaseCryptLibMock,
  const std::vector<UINT8>  &Digest
  )
{
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mSubjectBuffer;
  Cache.BufferSize = sizeof (mSubjectBuffer);

  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillRepeatedly (Invoke (EmitDigest (Digest)));

  return Cache;
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

// SHA-256 entry size: 16-byte owner GUID + 32-byte digest.
static constexpr UINT32  kSha256EntrySize = sizeof (EFI_GUID) + 32;
static constexpr UINT32  kSha384EntrySize = sizeof (EFI_GUID) + 48;

// V1 EFI_CERT_X509_SHA256 entry: owner GUID + 32-byte TBS hash + EFI_TIME (TimeOfRevocation).
static constexpr UINT32  kSha256TbsV1EntrySize = sizeof (EFI_GUID) + 32 + sizeof (EFI_TIME);

// ---------------------------------------------------------------------------
// Hash-membership search helpers
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
// Default Authenticode signature / image-hash payloads shared by the
// certificate-authorization tests.
//
static const std::vector<UINT8>  kSignatureDataDefault = std::vector<UINT8>(16, 0xA1);
static const std::vector<UINT8>  kImageHashDefault     = std::vector<UINT8>(SHA256_DIGEST_SIZE, 0x55);

// Build a WIN_CERTIFICATE wrapping an Authenticode signature payload.
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

STATIC
EFI_STATUS
EvaluatePkcsSignedDataSignature (
  IN     CONST WIN_CERTIFICATE       *Cert,
  IN OUT DIGEST_CACHE                *Cache,
  IN     CONST SIGNATURE_LISTS       *Lists,
  OUT    IMAGE_SIGNATURE_EVALUATION  *Evaluation
  )
{
  if ((Cert == NULL) || (Cert->dwLength <= sizeof (WIN_CERTIFICATE))) {
    return EvaluateSignature (NULL, 0, Cache, Lists, Evaluation);
  }

  return EvaluateSignature (
           (CONST UINT8 *)Cert + sizeof (WIN_CERTIFICATE),
           Cert->dwLength - sizeof (WIN_CERTIFICATE),
           Cache,
           Lists,
           Evaluation
           );
}

// Tiny throwaway "image" buffer for the digest cache; mocks of
// GetAuthenticodeHash never dereference it.
static UINT8           kFakeImage[16]         = { 0 };
static CONST EFI_GUID  mSha1HashAlgorithmGuid = EFI_HASH_ALGORITHM_SHA1_GUID;

static void
InitImageCache (
  DIGEST_CACHE  &Cache
  )
{
  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = kFakeImage;
  Cache.BufferSize = sizeof (kFakeImage);
}

// ---------------------------------------------------------------------------
// IsImageHashInAllowList (image-hash lists)
// ---------------------------------------------------------------------------

TEST (IsImageHashInAllowListTest, NullDatabaseWithNonZeroSize_NotFound) {
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, NULL, 1));
  // Validate that Cache remains consistent
  EXPECT_EQ (Cache.Buffer, (const VOID *)(UINTN)1);
  EXPECT_EQ (Cache.BufferSize, (UINTN)1);
}

TEST (IsImageHashInAllowListTest, NullDatabaseWithZeroSize_NotFound) {
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, NULL, 0));
  // Validate that Cache remains consistent
  EXPECT_EQ (Cache.Buffer, (const VOID *)(UINTN)1);
  EXPECT_EQ (Cache.BufferSize, (UINTN)1);
}

TEST (IsImageHashInAllowListTest, NullCache_NotFound) {
  UINT8  Dummy = 0;

  EXPECT_FALSE (IsImageHashInAllowList (NULL, &Dummy, 1));
}

TEST (IsImageHashInAllowListTest, UnboundCache_NotFound) {
  UINT8         Dummy = 0;
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  EXPECT_FALSE (IsImageHashInAllowList (&Cache, &Dummy, 1));
}

TEST (IsImageHashInAllowListTest, ZeroSizeCache_NotFound) {
  UINT8         Dummy = 0;
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 0;

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, &Dummy, 1));
}

TEST (IsImageHashInAllowListTest, HashComputationFailure_NotAuthorized) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mSubjectBuffer;
  Cache.BufferSize = sizeof (mSubjectBuffer);

  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  // A hash failure means this list cannot authorize; a best-effort allow-list search reports absent.
  EXPECT_FALSE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
}

TEST (IsImageHashInAllowListTest, ExactMatch_Found) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);
  std::vector<UINT8>  Target (kSha256DigestSize, 0xAA);

  SetEntryPayload (Db, Off, 1, Target);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, Target);

  EXPECT_TRUE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
  FreeDigestCache (&Cache);
}

TEST (IsImageHashInAllowListTest, V2ImageHashExactMatch_Found) {
  // A V2 (EFI_SIGNATURE_V2_DATA) image-hash list stores the digest with no SignatureOwner prefix,
  // so the payload must be read from the entry start rather than sizeof (EFI_GUID) bytes in.
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertV2Sha256Guid, 0, kSha256V2EntrySize, 2);
  std::vector<UINT8>  Target (kSha256DigestSize, 0xAA);

  SetV2EntryPayload (Db, Off, 1, Target);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, Target);

  EXPECT_TRUE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
  FreeDigestCache (&Cache);
}

TEST (IsImageHashInAllowListTest, NoMatchingEntry_NotFound) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  std::vector<UINT8>  Stored (kSha256DigestSize, 0xAA);

  SetEntryPayload (Db, Off, 0, Stored);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0xBB);
  DIGEST_CACHE        Cache = BindDigest (BaseCryptLibMock, Digest);

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
  FreeDigestCache (&Cache);
}

TEST (IsImageHashInAllowListTest, UnknownSignatureTypeList_Skipped) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Guid, 0, sizeof (EFI_GUID) + 16, 1);

  // A full-certificate list is not an image-hash list, so the digest is never computed and the list
  // is skipped entirely.
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mSubjectBuffer;
  Cache.BufferSize = sizeof (mSubjectBuffer);

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
}

TEST (IsImageHashInAllowListTest, MismatchedSignatureSize_Skipped) {
  // A list whose SignatureType is a supported image-hash GUID but whose per-entry size does not match
  // that algorithm's entry layout is skipped entirely - even when the leading bytes hold the exact
  // search digest - because the entry size must match exactly.
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha384EntrySize, 1);
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xCC);

  // Plant the exact search digest in the entry's leading bytes; the oversized entry must still be
  // rejected on size.
  SetEntryPayload (Db, Off, 0, Digest);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, Digest);

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
  FreeDigestCache (&Cache);
}

TEST (IsImageHashInAllowListTest, MatchInSecondList_Found) {
  // First list is a full-certificate list (skipped), second list contains the target digest.
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Guid, 0, sizeof (EFI_GUID) + 16, 1);
  size_t  SecondOff = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);

  std::vector<UINT8>  Target (kSha256DigestSize, 0x77);

  SetEntryPayload (Db, SecondOff, 1, Target);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, Target);

  EXPECT_TRUE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
  FreeDigestCache (&Cache);
}

TEST (IsImageHashInAllowListTest, NonZeroSignatureHeaderSize_EntryMathCorrect) {
  // SignatureHeaderSize is non-zero: the per-list header occupies
  // additional bytes between EFI_SIGNATURE_LIST and the first entry.
  // A naive cursor that forgets to skip it would either miss the
  // payload entirely or read the header bytes as a fake entry.
  MockBaseCryptLib    BaseCryptLibMock;
  constexpr UINT32    kHeader = 8;
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, kHeader, kSha256EntrySize, 2);

  // Fill the per-list header with a recognizable pattern so a math
  // bug that read it as an entry would compare against this, not the
  // real digest. The search target intentionally differs from it.
  std::memset (Db.data () + Off + sizeof (EFI_SIGNATURE_LIST), 0xEE, kHeader);

  std::vector<UINT8>  Target (kSha256DigestSize, 0x55);

  SetEntryPayload (Db, Off, 1, Target);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, Target);

  EXPECT_TRUE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
  FreeDigestCache (&Cache);
}

TEST (IsImageHashInAllowListTest, ZeroEntryList_NotFound) {
  // A well-formed list with zero entries must be skipped without a
  // false positive (EntryCount == 0 means the inner loop never runs).
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 0);

  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mSubjectBuffer;
  Cache.BufferSize = sizeof (mSubjectBuffer);

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
}

//
// A malformed allow-list (the sole list overruns the buffer) truncates to an empty
// prefix; a best-effort allow-list search simply finds no authority.
//
TEST (IsImageHashInAllowListTest, MalformedDb_BestEffortNotFound) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  ((EFI_SIGNATURE_LIST *)Db.data ())->SignatureListSize = (UINT32)(Db.size () + 1);

  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mSubjectBuffer;
  Cache.BufferSize = sizeof (mSubjectBuffer);

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
}

//
// A well-formed authorizing list followed by a malformed trailing fragment:
// the valid prefix still authorizes (best-effort allow-list search).
//
TEST (IsImageHashInAllowListTest, MalformedTail_ValidPrefixAuthorizes) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  std::vector<UINT8>  Target (kSha256DigestSize, 0x5A);

  SetEntryPayload (Db, Off, 0, Target);

  // Append a stray fragment too small to be a list header.
  Db.resize (Db.size () + sizeof (EFI_SIGNATURE_LIST) - 1, 0);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, Target);

  EXPECT_TRUE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
  FreeDigestCache (&Cache);
}

TEST (IsImageHashInAllowListTest, ZeroSizeNonNullDatabase_EmptyDatabaseNotFound) {
  UINT8         Dummy = 0;
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, &Dummy, 0));
}

//
// An allow-list whose SignatureType is a supported image-hash GUID (so GetHash
// succeeds against the bound cache) but whose SignatureHeaderSize is
// inflated so SigListIterInit yields an empty range for it. The list must be
// skipped and no authority returned.
//
TEST (IsImageHashInAllowListTest, MalformedListHeader_Skipped) {
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mSubjectBuffer;
  Cache.BufferSize = sizeof (mSubjectBuffer);

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

  EXPECT_FALSE (IsImageHashInAllowList (&Cache, Db.data (), Db.size ()));
}

// ---------------------------------------------------------------------------
// IsTbsHashInAllowList (X.509 TBS-cert-hash lists)
// ---------------------------------------------------------------------------

TEST (IsTbsHashInAllowListTest, V1TbsHashMatch_Found) {
  // A V1 EFI_CERT_X509_SHA256 entry is owner GUID + 32-byte TBS hash + EFI_TIME; only the leading
  // 32-byte hash is compared.
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256TbsV1EntrySize, 1);
  std::vector<UINT8>  TbsHash (kSha256DigestSize, 0x5A);

  SetEntryPayload (Db, Off, 0, TbsHash);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, TbsHash);

  EXPECT_TRUE (IsTbsHashInAllowList (&Cache, Db.data (), Db.size ()));
  FreeDigestCache (&Cache);
}

TEST (IsTbsHashInAllowListTest, V2TbsHashMatch_Found) {
  // A V2 EFI_CERT_V2_X509_SHA256 entry stores the TBS hash with neither the owner GUID nor a v1
  // TimeOfRevocation trailer.
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertV2X509Sha256Guid, 0, kSha256V2EntrySize, 1);
  std::vector<UINT8>  TbsHash (kSha256DigestSize, 0x5A);

  SetV2EntryPayload (Db, Off, 0, TbsHash);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, TbsHash);

  EXPECT_TRUE (IsTbsHashInAllowList (&Cache, Db.data (), Db.size ()));
  FreeDigestCache (&Cache);
}

TEST (IsTbsHashInAllowListTest, ImageHashList_Ignored) {
  // IsTbsHashInAllowList matches only X.509 TBS-cert-hash lists; an image-hash list is not its
  // subject and is skipped without computing the digest.
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mSubjectBuffer;
  Cache.BufferSize = sizeof (mSubjectBuffer);

  EXPECT_FALSE (IsTbsHashInAllowList (&Cache, Db.data (), Db.size ()));
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
// LoadDbAndDbx (db + dbx)
// ---------------------------------------------------------------------------

class LoadDbAndDbxTest : public ::testing::Test {
protected:
  MockUefiLib UefiLibMock;
};

//
// Lambda factory: a GetVariable2 action that allocates a copy of the
// supplied payload and returns EFI_SUCCESS. Used to feed synthetic
// db/dbx buffers into LoadDbAndDbx through the mock.
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

TEST_F (LoadDbAndDbxTest, NullLists_ReturnsInvalidParameter) {
  EXPECT_EQ (
    LoadDbAndDbx (NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (LoadDbAndDbxTest, BothVariablesMissing_Success) {
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_NOT_FOUND))   // db
    .WillOnce (Return (EFI_NOT_FOUND));  // dbx

  // Pre-set to bogus values: must be cleared.
  SIGNATURE_LISTS  Lists = {
    (VOID *)(UINTN)0xDEADBEEF, 0xAA,
    (VOID *)(UINTN)0xCAFEF00D, 0xBB
  };

  EXPECT_EQ (
    LoadDbAndDbx (&Lists),
    EFI_SUCCESS
    );
  EXPECT_EQ (Lists.AllowList, (VOID *)NULL);
  EXPECT_EQ (Lists.AllowListSize, 0u);
  EXPECT_EQ (Lists.RevokeList, (VOID *)NULL);
  EXPECT_EQ (Lists.RevokeListSize, 0u);
}

TEST_F (LoadDbAndDbxTest, OnlyDbPresent_DbAllocated) {
  std::vector<UINT8>  DbBuf;

  AppendSignatureList (DbBuf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (DbBuf.data (), DbBuf.size ()))  // db
    .WillOnce (Return (EFI_NOT_FOUND));                               // dbx

  SIGNATURE_LISTS  Lists = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadDbAndDbx (&Lists),
    EFI_SUCCESS
    );
  ASSERT_NE (Lists.AllowList, (VOID *)NULL);
  EXPECT_EQ (Lists.AllowListSize, DbBuf.size ());
  EXPECT_EQ (Lists.RevokeList, (VOID *)NULL);
  EXPECT_EQ (Lists.RevokeListSize, 0u);
  FreePool (Lists.AllowList);
}

TEST_F (LoadDbAndDbxTest, OnlyDbxPresent_DbxAllocated) {
  std::vector<UINT8>  DbxBuf;

  AppendSignatureList (DbxBuf, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_NOT_FOUND))                                  // db
    .WillOnce (ReturnVariablePayload (DbxBuf.data (), DbxBuf.size ())); // dbx

  SIGNATURE_LISTS  Lists = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadDbAndDbx (&Lists),
    EFI_SUCCESS
    );
  EXPECT_EQ (Lists.AllowList, (VOID *)NULL);
  EXPECT_EQ (Lists.AllowListSize, 0u);
  ASSERT_NE (Lists.RevokeList, (VOID *)NULL);
  EXPECT_EQ (Lists.RevokeListSize, DbxBuf.size ());
  FreePool (Lists.RevokeList);
}

TEST_F (LoadDbAndDbxTest, BothPresent_BuffersAllocated) {
  std::vector<UINT8>  DbBuf;
  std::vector<UINT8>  DbxBuf;

  AppendSignatureList (DbBuf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  AppendSignatureList (DbxBuf, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (DbBuf.data (), DbBuf.size ()))    // db
    .WillOnce (ReturnVariablePayload (DbxBuf.data (), DbxBuf.size ())); // dbx

  SIGNATURE_LISTS  Lists = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadDbAndDbx (&Lists),
    EFI_SUCCESS
    );
  ASSERT_NE (Lists.AllowList, (VOID *)NULL);
  EXPECT_EQ (Lists.AllowListSize, DbBuf.size ());
  ASSERT_NE (Lists.RevokeList, (VOID *)NULL);
  EXPECT_EQ (Lists.RevokeListSize, DbxBuf.size ());
  FreePool (Lists.AllowList);
  FreePool (Lists.RevokeList);
}

TEST_F (LoadDbAndDbxTest, DbLoadFails_ErrorPropagatedNothingAllocated) {
  // The db lookup fails with a non-NOT_FOUND status; dbx must not even
  // be attempted, and both out-pointers must be NULL.
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  SIGNATURE_LISTS  Lists = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadDbAndDbx (&Lists),
    EFI_DEVICE_ERROR
    );
  EXPECT_EQ (Lists.AllowList, (VOID *)NULL);
  EXPECT_EQ (Lists.AllowListSize, 0u);
  EXPECT_EQ (Lists.RevokeList, (VOID *)NULL);
  EXPECT_EQ (Lists.RevokeListSize, 0u);
}

TEST_F (LoadDbAndDbxTest, DbxLoadFails_DbFreedAndErrorPropagated) {
  std::vector<UINT8>  DbBuf;

  AppendSignatureList (DbBuf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  // db succeeds (allocation must be cleaned up internally); dbx fails.
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (DbBuf.data (), DbBuf.size ()))  // db
    .WillOnce (Return (EFI_DEVICE_ERROR));                            // dbx

  SIGNATURE_LISTS  Lists = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadDbAndDbx (&Lists),
    EFI_DEVICE_ERROR
    );
  EXPECT_EQ (Lists.AllowList, (VOID *)NULL);   // freed and nulled
  EXPECT_EQ (Lists.AllowListSize, 0u);
  EXPECT_EQ (Lists.RevokeList, (VOID *)NULL);
  EXPECT_EQ (Lists.RevokeListSize, 0u);
}

TEST_F (LoadDbAndDbxTest, DbxLoadFailsWithAllocatedBuffer_DbxFreedAndNulled) {
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

  SIGNATURE_LISTS  Lists = { NULL, 0, NULL, 0 };

  EXPECT_EQ (
    LoadDbAndDbx (&Lists),
    EFI_DEVICE_ERROR
    );
  EXPECT_EQ (Lists.AllowList, (VOID *)NULL);
  EXPECT_EQ (Lists.AllowListSize, 0u);
  EXPECT_EQ (Lists.RevokeList, (VOID *)NULL);
  EXPECT_EQ (Lists.RevokeListSize, 0u);
}

// ---------------------------------------------------------------------------
// IsImageHashInRevokeList (image-hash lists)
// ---------------------------------------------------------------------------

//
// An unusable subject cache (NULL buffer) is treated as present (fail closed).
//
TEST (IsImageHashInRevokeListTest, UnusableCacheNullBuffer_ReturnsTrue) {
  std::vector<UINT8>  Dbx (32, 0);
  DIGEST_CACHE        Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = NULL;
  Cache.BufferSize = 4;

  EXPECT_TRUE (IsImageHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An unusable subject cache (zero size) is treated as present (fail closed).
//
TEST (IsImageHashInRevokeListTest, UnusableCacheZeroSize_ReturnsTrue) {
  UINT8               SubjectByte = 0x30;
  std::vector<UINT8>  Dbx (32, 0);
  DIGEST_CACHE        Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = &SubjectByte;
  Cache.BufferSize = 0;

  EXPECT_TRUE (IsImageHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
}

//
// An image subject whose digest is enrolled in the revoke-list is revoked.
//
TEST (IsImageHashInRevokeListTest, ImageHashMatch_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xC3);

  SetEntryPayload (Dbx, Off, 0, Digest);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, Digest);

  EXPECT_TRUE (IsImageHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
  FreeDigestCache (&Cache);
}

//
// A V2 (EFI_SIGNATURE_V2_DATA) image-hash revoke-list entry honors the ownerless layout the same as the
// allow-list.
//
TEST (IsImageHashInRevokeListTest, V2ImageHashExactMatch_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertV2Sha256Guid, 0, kSha256V2EntrySize, 1);
  std::vector<UINT8>  Target (kSha256DigestSize, 0xAA);

  SetV2EntryPayload (Dbx, Off, 0, Target);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, Target);

  EXPECT_TRUE (IsImageHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
  FreeDigestCache (&Cache);
}

//
// An image subject whose digest is not in the revoke-list is not revoked.
//
TEST (IsImageHashInRevokeListTest, ImageHashNoMatch_ReturnsFalse) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(kSha256DigestSize, 0xC3));

  std::vector<UINT8>  Digest (kSha256DigestSize, 0xD4);
  DIGEST_CACHE        Cache = BindDigest (BaseCryptLibMock, Digest);

  EXPECT_FALSE (IsImageHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
  FreeDigestCache (&Cache);
}

//
// A supported image-hash list whose digest cannot be computed fails closed (the entry might have
// matched).
//
TEST (IsImageHashInRevokeListTest, HashComputationFailure_FailsClosed) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mSubjectBuffer;
  Cache.BufferSize = sizeof (mSubjectBuffer);

  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  EXPECT_TRUE (IsImageHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
}

//
// A valid non-matching list followed by a malformed trailing fragment fails
// closed: even though the subject is not in the valid prefix, the dropped tail
// might have matched it.
//
TEST (IsImageHashInRevokeListTest, MalformedTail_FailsClosed) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(kSha256DigestSize, 0x01));

  // Append a stray fragment too small to be a list header.
  Dbx.resize (Dbx.size () + sizeof (EFI_SIGNATURE_LIST) - 1, 0);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0x99);   // not the enrolled entry
  DIGEST_CACHE        Cache = BindDigest (BaseCryptLibMock, Digest);

  EXPECT_TRUE (IsImageHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
  FreeDigestCache (&Cache);
}

// ---------------------------------------------------------------------------
// IsTbsHashInRevokeList (X.509 TBS-cert-hash lists)
// ---------------------------------------------------------------------------

//
// An EFI_CERT_X509_SHA256 revoke-list whose entry holds the subject's TBS hash marks it revoked.
//
TEST (IsTbsHashInRevokeListTest, TbsHashMatch_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256TbsV1EntrySize, 1);
  std::vector<UINT8>  TbsHash (kSha256DigestSize, 0xE1);

  SetEntryPayload (Dbx, Off, 0, TbsHash);

  DIGEST_CACHE  Cache = BindDigest (BaseCryptLibMock, TbsHash);

  EXPECT_TRUE (IsTbsHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
  FreeDigestCache (&Cache);
}

//
// An EFI_CERT_X509_SHA256 revoke-list whose entry digest differs from the subject's TBS hash is not a
// match.
//
TEST (IsTbsHashInRevokeListTest, TbsHashDiffers_ReturnsFalse) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;
  size_t              Off = AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256TbsV1EntrySize, 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(kSha256DigestSize, 0xAA));

  std::vector<UINT8>  TbsHash (kSha256DigestSize, 0x77);
  DIGEST_CACHE        Cache = BindDigest (BaseCryptLibMock, TbsHash);

  EXPECT_FALSE (IsTbsHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
  FreeDigestCache (&Cache);
}

//
// If the subject's TBS hash cannot be computed for a cert-hash list, the search fails closed.
//
TEST (IsTbsHashInRevokeListTest, TbsHashComputeFails_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256TbsV1EntrySize, 1);

  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mSubjectBuffer;
  Cache.BufferSize = sizeof (mSubjectBuffer);

  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  EXPECT_TRUE (IsTbsHashInRevokeList (&Cache, Dbx.data (), Dbx.size ()));
}

// ---------------------------------------------------------------------------
// IsCertInRevokeList (full X.509 certificate lists)
// ---------------------------------------------------------------------------

//
// An unusable certificate is treated as present (fail closed).
//
TEST (IsCertInRevokeListTest, UnusableCert_ReturnsTrue) {
  std::vector<UINT8>  Dbx (32, 0);

  EXPECT_TRUE (IsCertInRevokeList (NULL, 0, Dbx.data (), Dbx.size ()));
}

//
// A NULL revoke-list means nothing is revoked.
//
TEST (IsCertInRevokeListTest, NullRevokeList_ReturnsFalse) {
  std::vector<UINT8>  Cert = { 0x30, 0x82 };

  EXPECT_FALSE (IsCertInRevokeList (Cert.data (), Cert.size (), NULL, 0));
}

//
// An empty revoke-list means nothing is revoked.
//
TEST (IsCertInRevokeListTest, EmptyRevokeList_ReturnsFalse) {
  std::vector<UINT8>  Cert = { 0x30, 0x82 };
  std::vector<UINT8>  Dbx (32, 0);

  EXPECT_FALSE (IsCertInRevokeList (Cert.data (), Cert.size (), Dbx.data (), 0));
}

//
// A revoke-list too small to contain a signature-list header must fail closed.
//
TEST (IsCertInRevokeListTest, MalformedRevokeList_ReturnsTrue) {
  std::vector<UINT8>  Cert = { 0x30, 0x82 };
  std::vector<UINT8>  Dbx (4, 0);

  EXPECT_TRUE (IsCertInRevokeList (Cert.data (), Cert.size (), Dbx.data (), Dbx.size ()));
}

//
// An EFI_CERT_X509 list whose entry payload matches the certificate byte-for-byte (same length)
// marks the certificate as revoked.
//
TEST (IsCertInRevokeListTest, X509ExactMatch_ReturnsTrue) {
  std::vector<UINT8>  Cert (16, 0x11);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, Off, 0, Cert);

  EXPECT_TRUE (IsCertInRevokeList (Cert.data (), Cert.size (), Dbx.data (), Dbx.size ()));
}

//
// A V2 (EFI_SIGNATURE_V2_DATA) full-certificate revoke-list stores the DER certificate with no owner
// prefix.
//
TEST (IsCertInRevokeListTest, V2X509ExactMatch_ReturnsTrue) {
  std::vector<UINT8>  Cert (24, 0xC7);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertV2X509Guid, 0, (UINT32)Cert.size (), 1);

  SetV2EntryPayload (Dbx, Off, 0, Cert);

  EXPECT_TRUE (IsCertInRevokeList (Cert.data (), Cert.size (), Dbx.data (), Dbx.size ()));
}

//
// An EFI_CERT_X509 list whose entry payload differs from the certificate is not a match.
//
TEST (IsCertInRevokeListTest, X509BytesDiffer_ReturnsFalse) {
  std::vector<UINT8>  Cert (16, 0x11);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(16, 0x22));

  EXPECT_FALSE (IsCertInRevokeList (Cert.data (), Cert.size (), Dbx.data (), Dbx.size ()));
}

//
// An EFI_CERT_X509 list whose entry payload size differs from the certificate size is skipped (not
// a match).
//
TEST (IsCertInRevokeListTest, X509PayloadSizeMismatch_ReturnsFalse) {
  std::vector<UINT8>  Cert (16, 0x11);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 32), 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(32, 0x11));

  EXPECT_FALSE (IsCertInRevokeList (Cert.data (), Cert.size (), Dbx.data (), Dbx.size ()));
}

//
// A list whose type is not EFI_CERT_X509 (here an image-hash list) is not a full-certificate list
// and is skipped.
//
TEST (IsCertInRevokeListTest, NonX509List_ReturnsFalse) {
  std::vector<UINT8>  Cert (8, 0x30);
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_FALSE (IsCertInRevokeList (Cert.data (), Cert.size (), Dbx.data (), Dbx.size ()));
}

// ---------------------------------------------------------------------------
// IsChainRevoked
// ---------------------------------------------------------------------------

//
// Mock X509GetTBSCert () to hand back the whole certificate as its own TBSCertificate. IsChainRevoked
// pre-extracts the TBSCertificate of every chain certificate; the exact bytes are irrelevant to
// these tests, where chain revocation is decided by the exact-DER IsCertInRevokeList path (or a
// parse failure) rather than a TBS-cert-hash match.
//
static void
ExpectTbsExtractionPassthrough (
  MockBaseCryptLib  &BaseCryptLibMock
  )
{
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillRepeatedly (
       Invoke (
         [] (CONST UINT8 *Cert, UINTN CertSize, UINT8 **TBSCert, UINTN *TBSCertSize) -> BOOLEAN {
    *TBSCert     = (UINT8 *)Cert;
    *TBSCertSize = CertSize;
    return TRUE;
  }
         )
       );
}

TEST (IsChainRevokedTest, NullCertChain_ReturnsTrue) {
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

TEST (IsChainRevokedTest, ZeroCertChainSize_ReturnsTrue) {
  std::vector<UINT8>  Dbx (32, 0);

  EXPECT_TRUE (
    IsChainRevoked (
      kSignatureDataDefault.data (),
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
// With valid inputs but no revoke-list, nothing is revoked and the chain is
// never built.
//
TEST (IsChainRevokedTest, NullRevokeList_ReturnsFalse) {
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

TEST (IsChainRevokedTest, EmptyRevokeList_ReturnsFalse) {
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
// A chain cert that is listed in the revoke-list (exact DER match) revokes the chain.
//
TEST (IsChainRevokedTest, ChainCertInRevokeList_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  ChainCert (16, 0x11);
  std::vector<UINT8>  Dbx;

  static std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(16, 0x11) });

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, Off, 0, ChainCert);

  ExpectTbsExtractionPassthrough (BaseCryptLibMock);

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
// A chain whose certs are not in the revoke-list is not revoked.
//
TEST (IsChainRevokedTest, ChainCertNotInRevokeList_ReturnsFalse) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;

  static std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(16, 0x11) });

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(16, 0x22));

  ExpectTbsExtractionPassthrough (BaseCryptLibMock);

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
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Dbx;

  // CertNumber = 2, but only one 4-byte cert is present.
  static std::vector<UINT8>  BadStack = {
    0x02, 0x04, 0x00, 0x00, 0x00, 0xAA, 0xBB, 0xCC, 0xDD
  };

  // X509 list whose payload size (16) will not match the 4-byte cert, so
  // the first cert is skipped and the walk reaches the truncation.
  AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  ExpectTbsExtractionPassthrough (BaseCryptLibMock);

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
// ExtractSignatureData (Support.c)
// ---------------------------------------------------------------------------

TEST (ExtractSignatureDataTest, NullCert_ReturnsInvalidParameter) {
  const UINT8  *SignatureData    = NULL;
  UINTN        SignatureDataSize = 0;

  EXPECT_EQ (
    ExtractSignatureData (NULL, &SignatureData, &SignatureDataSize),
    EFI_INVALID_PARAMETER
    );
}

TEST (ExtractSignatureDataTest, NullOutputs_ReturnsInvalidParameter) {
  WIN_CERTIFICATE  Cert              = { sizeof (WIN_CERTIFICATE) + 1, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA };
  const UINT8      *SignatureData    = NULL;
  UINTN            SignatureDataSize = 0;

  EXPECT_EQ (
    ExtractSignatureData (&Cert, NULL, &SignatureDataSize),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    ExtractSignatureData (&Cert, &SignatureData, NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST (ExtractSignatureDataTest, PkcsSignedData_ExtractsPayload) {
  // Build a WIN_CERTIFICATE followed by 4 bytes of payload.
  const UINT8         Payload[] = { 0xAA, 0xBB, 0xCC, 0xDD };
  std::vector<UINT8>  Buffer (sizeof (WIN_CERTIFICATE) + sizeof (Payload), 0);
  WIN_CERTIFICATE     *Cert = (WIN_CERTIFICATE *)Buffer.data ();

  Cert->dwLength         = (UINT32)Buffer.size ();
  Cert->wRevision        = 0x0200;
  Cert->wCertificateType = WIN_CERT_TYPE_PKCS_SIGNED_DATA;
  std::memcpy (Buffer.data () + sizeof (WIN_CERTIFICATE), Payload, sizeof (Payload));

  const UINT8  *SignatureData    = NULL;
  UINTN        SignatureDataSize = 0;

  EXPECT_EQ (
    ExtractSignatureData (Cert, &SignatureData, &SignatureDataSize),
    EFI_SUCCESS
    );
  ASSERT_EQ (SignatureDataSize, sizeof (Payload));
  EXPECT_EQ (0, std::memcmp (SignatureData, Payload, sizeof (Payload)));
}

TEST (ExtractSignatureDataTest, PkcsSignedData_HeaderOnly_ReturnsCorrupted) {
  WIN_CERTIFICATE  Cert              = { sizeof (WIN_CERTIFICATE), 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA };
  const UINT8      *SignatureData    = NULL;
  UINTN            SignatureDataSize = 0;

  EXPECT_EQ (
    ExtractSignatureData (&Cert, &SignatureData, &SignatureDataSize),
    EFI_VOLUME_CORRUPTED
    );
}

TEST (ExtractSignatureDataTest, EfiGuidAuthenticodeSignature_ExtractsPayload) {
  const UINT8                Payload[]  = { 0x11, 0x22, 0x33 };
  const size_t               HeaderSize = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  std::vector<UINT8>         Buffer (HeaderSize + sizeof (Payload), 0);
  WIN_CERTIFICATE_UEFI_GUID  *UefiCert = (WIN_CERTIFICATE_UEFI_GUID *)Buffer.data ();

  UefiCert->Hdr.dwLength         = (UINT32)Buffer.size ();
  UefiCert->Hdr.wRevision        = 0x0200;
  UefiCert->Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyMem (&UefiCert->CertType, &gEfiCertPkcs7Guid, sizeof (EFI_GUID));
  std::memcpy (Buffer.data () + HeaderSize, Payload, sizeof (Payload));

  const UINT8  *SignatureData    = NULL;
  UINTN        SignatureDataSize = 0;

  EXPECT_EQ (
    ExtractSignatureData (&UefiCert->Hdr, &SignatureData, &SignatureDataSize),
    EFI_SUCCESS
    );
  ASSERT_EQ (SignatureDataSize, sizeof (Payload));
  EXPECT_EQ (0, std::memcmp (SignatureData, Payload, sizeof (Payload)));
}

TEST (ExtractSignatureDataTest, EfiGuidOtherSignatureType_ReturnsUnsupported) {
  const size_t               HeaderSize = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  std::vector<UINT8>         Buffer (HeaderSize + 4, 0);
  WIN_CERTIFICATE_UEFI_GUID  *UefiCert = (WIN_CERTIFICATE_UEFI_GUID *)Buffer.data ();
  const EFI_GUID             OtherGuid = { 0x12345678, 0x1234, 0x1234, { 1, 2, 3, 4, 5, 6, 7, 8 }
  };

  UefiCert->Hdr.dwLength         = (UINT32)Buffer.size ();
  UefiCert->Hdr.wRevision        = 0x0200;
  UefiCert->Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyMem (&UefiCert->CertType, &OtherGuid, sizeof (EFI_GUID));

  const UINT8  *SignatureData    = NULL;
  UINTN        SignatureDataSize = 0;

  EXPECT_EQ (
    ExtractSignatureData (&UefiCert->Hdr, &SignatureData, &SignatureDataSize),
    EFI_UNSUPPORTED
    );
}

TEST (ExtractSignatureDataTest, EfiGuid_HeaderOnly_ReturnsCorrupted) {
  WIN_CERTIFICATE_UEFI_GUID  UefiCert;

  ZeroMem (&UefiCert, sizeof (UefiCert));
  UefiCert.Hdr.dwLength         = (UINT32)OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  UefiCert.Hdr.wRevision        = 0x0200;
  UefiCert.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;

  const UINT8  *SignatureData    = NULL;
  UINTN        SignatureDataSize = 0;

  EXPECT_EQ (
    ExtractSignatureData (&UefiCert.Hdr, &SignatureData, &SignatureDataSize),
    EFI_VOLUME_CORRUPTED
    );
}

TEST (ExtractSignatureDataTest, UnknownCertType_ReturnsUnsupported) {
  WIN_CERTIFICATE  Cert              = { sizeof (WIN_CERTIFICATE) + 8, 0x0200, WIN_CERT_TYPE_EFI_PKCS115 };
  const UINT8      *SignatureData    = NULL;
  UINTN            SignatureDataSize = 0;

  EXPECT_EQ (
    ExtractSignatureData (&Cert, &SignatureData, &SignatureDataSize),
    EFI_UNSUPPORTED
    );
}

// ---------------------------------------------------------------------------
// EvaluateSignature -- end-to-end Authenticode signature + database scenarios
// ---------------------------------------------------------------------------

//
// Install the mock expectations shared by every allow-list walk scenario: the image
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
    CopyMem (Out, &gEfiHashAlgorithmSha256Guid, sizeof (EFI_GUID));
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillRepeatedly (Invoke (EmitDigest (std::vector<UINT8>(kSha256DigestSize, 0x55))));
}

//
// A NULL SignatureData pointer is rejected up front with EFI_INVALID_PARAMETER;
// no crypto is consulted and no verdict is produced.
//
TEST (EvaluateSignatureTest, NullSignatureData_ReturnsInvalidParameter) {
  DIGEST_CACHE                Cache;
  SIGNATURE_LISTS             Lists = { NULL, 0, NULL, 0 };
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_EQ (
    EvaluateSignature (NULL, kSignatureDataDefault.size (), &Cache, &Lists, &Eval),
    EFI_INVALID_PARAMETER
    );
}

//
// A NULL digest cache is rejected up front with EFI_INVALID_PARAMETER.
//
TEST (EvaluateSignatureTest, NullCache_ReturnsInvalidParameter) {
  SIGNATURE_LISTS             Lists = { NULL, 0, NULL, 0 };
  IMAGE_SIGNATURE_EVALUATION  Eval;

  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_EQ (
    EvaluateSignature (
      kSignatureDataDefault.data (),
      kSignatureDataDefault.size (),
      NULL,
      &Lists,
      &Eval
      ),
    EFI_INVALID_PARAMETER
    );
}

//
// A NULL databases pointer is rejected up front with EFI_INVALID_PARAMETER.
//
TEST (EvaluateSignatureTest, NullLists_ReturnsInvalidParameter) {
  DIGEST_CACHE                Cache;
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_EQ (
    EvaluateSignature (
      kSignatureDataDefault.data (),
      kSignatureDataDefault.size (),
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
TEST (EvaluateSignatureTest, NullEvaluation_ReturnsInvalidParameter) {
  DIGEST_CACHE     Cache;
  SIGNATURE_LISTS  Lists = { NULL, 0, NULL, 0 };

  InitImageCache (Cache);

  EXPECT_EQ (
    EvaluateSignature (
      kSignatureDataDefault.data (),
      kSignatureDataDefault.size (),
      &Cache,
      &Lists,
      NULL
      ),
    EFI_INVALID_PARAMETER
    );
}

//
// An empty SignatureData buffer is rejected up front with EFI_INVALID_PARAMETER.
//
TEST (EvaluateSignatureTest, EmptySignatureData_ReturnsInvalidParameter) {
  DIGEST_CACHE                Cache;
  SIGNATURE_LISTS             Lists = { NULL, 0, NULL, 0 };
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_EQ (
    EvaluateSignature (kSignatureDataDefault.data (), 0, &Cache, &Lists, &Eval),
    EFI_INVALID_PARAMETER
    );
}

//
// GetAuthenticodeHashAlgorithm fails, so the image-hash algorithm cannot be
// determined and the certificate is unusable. The image hash is never computed
// and no anchor is verified.
//
TEST (EvaluateSignatureTest, HashAlgorithmFails_Unusable) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_LISTS             Lists   = { NULL, 0, NULL, 0 };
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (Return (EFI_UNSUPPORTED));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureUnusable);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

TEST (EvaluateSignatureTest, Sha1HashAlgorithm_Unusable) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_LISTS             Lists   = { NULL, 0, NULL, 0 };
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *Out) -> EFI_STATUS {
    CopyMem (Out, &mSha1HashAlgorithmGuid, sizeof (EFI_GUID));
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureUnusable);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// The image hash cannot be computed: HashAllByGuid fails, GetHash
// surfaces the error, and EvaluateSignature propagates it.
// This is the only non-INVALID_PARAMETER error path; no verdict is asserted.
//
TEST (EvaluateSignatureTest, ImageHashFails_ReturnsError) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_LISTS             Lists   = { NULL, 0, NULL, 0 };
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *Out) -> EFI_STATUS {
    CopyMem (Out, &gEfiHashAlgorithmSha256Guid, sizeof (EFI_GUID));
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_DEVICE_ERROR
    );
}

//
// Signer extraction is no longer a separate prerequisite. With an empty allow-list,
// evaluation completes without invoking a verifier.
//
TEST (EvaluateSignatureTest, SignerExtractionNotRequired_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_LISTS             Lists   = { NULL, 0, NULL, 0 };
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// The evaluator never calls the legacy signer-extraction API.
//
TEST (EvaluateSignatureTest, LegacySignerApiNotCalled_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_LISTS             Lists   = { NULL, 0, NULL, 0 };
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (_)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// The prelude succeeds and a signer is available, but the allow-list is empty. No
// anchor matches, so the verdict is ImageSignatureNotAuthorized and AuthenticodeVerifyEx is
// never called.
//
TEST (EvaluateSignatureTest, EmptyAllowList_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_LISTS             Lists   = { NULL, 0, NULL, 0 };
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// A single EFI_CERT_X509 allow-list trust anchor verifies the image and there is no
// revoke-list (so no chain is built). The signature is allowed and the authority points
// at the matching allow-list entry.
//
TEST (EvaluateSignatureTest, X509VerifiesNoRevokeList_Allowed) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureAllowed);
  EXPECT_NE (Eval.Authority.Data, nullptr);
  EXPECT_EQ (Eval.Authority.Size, (UINTN)(sizeof (EFI_GUID) + 16));
}

//
// A V2 (EFI_SIGNATURE_V2_DATA) full-certificate allow-list anchor carries no owner GUID; the whole entry
// is the DER certificate handed to AuthenticodeVerifyEx. The recorded authority normalizes it to a
// V1 EFI_SIGNATURE_DATA by prepending a zeroed owner GUID, so its size is the certificate plus a
// SignatureOwner.
//
TEST (EvaluateSignatureTest, V2X509VerifiesNoRevokeList_Allowed) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertV2X509Guid, 0, 16, 1);

  SetV2EntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureAllowed);
  EXPECT_NE (Eval.Authority.Data, nullptr);
  // The V2 anchor carries no owner GUID, so the authority prepends a zeroed one to the certificate.
  EXPECT_EQ (Eval.Authority.Size, (UINTN)(sizeof (EFI_GUID) + 16));
  FreeImageAuthority (&Eval.Authority);
}

//
// A V2 (EFI_SIGNATURE_V2_DATA) TBS-cert-hash allow-list entry holds only the hash (no owner GUID, no v1
// TimeOfRevocation). The full hash region is passed to GetTrustAnchorX509FromAuthData; this asserts
// the size passed reflects the ownerless entry.
//
TEST (EvaluateSignatureTest, V2X509HashListResolvesAnchor_Allowed) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

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

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureAllowed);
  EXPECT_NE (Eval.Authority.Data, nullptr);
}

//
// A single EFI_CERT_X509 trust anchor is present but AuthenticodeVerifyEx rejects
// the image. No anchor authorizes it, so the verdict is ImageSignatureNotAuthorized.
//
TEST (EvaluateSignatureTest, X509DoesNotVerify_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SECURITY_VIOLATION));

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// An EFI_CERT_X509 anchor verifies the image, but a certificate in the signer's
// chain is enrolled in the revoke-list. The anchor is verified-but-revoked and, with no
// other anchor, the verdict is ImageSignatureRevoked and the authority names the
// revoke-list entry.
//
TEST (EvaluateSignatureTest, X509VerifiesChainRevoked_Revoked) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  // Revoke-list: one X509 list holding the (20-byte) chain cert exactly.
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 20), 1);

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(20, 0xAB));

  static std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(20, 0xAB) });

  ExpectSignedImagePrelude (BaseCryptLibMock);
  ExpectTbsExtractionPassthrough (BaseCryptLibMock);
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

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureRevoked);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
  EXPECT_EQ (Eval.Authority.Size, (UINTN)0);
}

//
// An EFI_CERT_X509 anchor verifies the image and none of the signer's chain
// certs are in the revoke-list. The image is approved.
//
TEST (EvaluateSignatureTest, X509VerifiesChainClean_Allowed) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  // The revoke-list holds an unrelated cert, so the chain is clean.
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(16, 0x22));

  static std::vector<UINT8>  Stack = MakeCertStack ({ std::vector<UINT8>(16, 0x33) });

  ExpectSignedImagePrelude (BaseCryptLibMock);
  ExpectTbsExtractionPassthrough (BaseCryptLibMock);
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

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureAllowed);
  EXPECT_NE (Eval.Authority.Data, nullptr);
}

//
// The allow-list holds an EFI_CERT_X509_SHA256 (TBS-cert-hash) list. A trust anchor is
// recovered from the signature data, it authenticates the image, and there is no
// revoke-list. The signature is allowed; the resolver's cache handle is released via
// FreeTrustAnchorX509Cache.
//
TEST (EvaluateSignatureTest, X509HashListResolvesAnchor_Allowed) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

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

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureAllowed);
  EXPECT_NE (Eval.Authority.Data, nullptr);
}

//
// The allow-list holds an EFI_CERT_X509_SHA256 list with two entries, but neither recovers
// a trust anchor from the auth data (EFI_NOT_FOUND). Both entries are walked
// and no anchor authorizes: ImageSignatureNotAuthorized.
//
TEST (EvaluateSignatureTest, X509HashListAllNotFound_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256TbsV1EntrySize, 2);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .Times (2)
    .WillRepeatedly (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// The allow-list holds an EFI_CERT_X509_SHA256 list; the trust-anchor lookup fails with a
// hard error (not EFI_NOT_FOUND). The entry is skipped and no anchor
// authorizes: ImageSignatureNotAuthorized.
//
TEST (EvaluateSignatureTest, X509HashListHardError_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256TbsV1EntrySize, 1);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// Two EFI_CERT_X509 anchors: the first verifies but its chain is revoked by
// the revoke-list; the second verifies with a clean chain. The image is approved via the
// second anchor.
//
TEST (EvaluateSignatureTest, TwoAnchorsFirstRevokedSecondClean_Allowed) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  // Allow-list: one X509 list with two anchors (payload 0x11 and 0x22).
  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 2);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));
  SetEntryPayload (Db, DbOff, 1, std::vector<UINT8>(16, 0x22));

  // Revoke-list: holds the revoked chain cert (24 bytes 0xDD).
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 24), 1);

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(24, 0xDD));

  static std::vector<UINT8>  RevokedStack = MakeCertStack ({ std::vector<UINT8>(24, 0xDD) });
  static std::vector<UINT8>  CleanStack   = MakeCertStack ({ std::vector<UINT8>(24, 0xEE) });

  ExpectSignedImagePrelude (BaseCryptLibMock);
  ExpectTbsExtractionPassthrough (BaseCryptLibMock);
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

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureAllowed);
  EXPECT_NE (Eval.Authority.Data, nullptr);
}

//
// The allow-list contains only a non-X.509 (image-hash) list. It is skipped and, with no
// trust anchors, the verdict is ImageSignatureNotAuthorized.
//
TEST (EvaluateSignatureTest, AllowListHasOnlyNonX509List_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// The allow-list contains an EFI_CERT_X509 list whose SignatureSize equals sizeof(EFI_GUID)
// (no cert payload). The list is skipped and the verdict is ImageSignatureNotAuthorized.
//
TEST (EvaluateSignatureTest, AllowListX509ListNoCertPayload_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)sizeof (EFI_GUID), 1);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// The allow-list is malformed (smaller than one EFI_SIGNATURE_LIST header) so
// DatabaseIterInit truncates it to an empty range. The verdict is
// ImageSignatureNotAuthorized.
//
TEST (EvaluateSignatureTest, MalformedAllowList_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db (4, 0);

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _)).Times (0);

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}

//
// Happy path: the prelude succeeds, the single allow-list trust anchor verifies via
// AuthenticodeVerifyEx, and the empty revoke-list makes the chain check pass. The image
// is approved with a non-NULL authority whose SignatureType is the authorizing
// allow-list type.
//
TEST (EvaluateSignatureTest, AuthorizedByAllowList_Allowed) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  ExpectSignedImagePrelude (BaseCryptLibMock);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureAllowed);
  EXPECT_NE (Eval.Authority.Data, nullptr);
  EXPECT_EQ (Eval.Authority.Size, (UINTN)(sizeof (EFI_GUID) + 16));
  EXPECT_TRUE (CompareGuid (&Eval.Authority.SignatureType, &gEfiCertX509Guid));
}

//
// An allow-list anchor verifies the image, but a certificate in its verified chain is
// enrolled in the revoke-list, so the chain is revoked. With no other anchor the
// verdict is ImageSignatureRevoked and no authority is recorded (Data is NULL,
// Size is 0, and SignatureType stays zeroed).
//
TEST (EvaluateSignatureTest, RevokedByRevokeList_Revoked) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

  InitImageCache (Cache);
  ZeroMem (&Eval, sizeof (Eval));

  // Allow-list: one X509 trust anchor that verifies the image.
  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  // Revoke-list: one X509 list holding the (20-byte) chain cert exactly.
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (Dbx, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 20), 1);

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(20, 0xAB));

  static std::vector<UINT8>  RevokedChain = MakeCertStack ({ std::vector<UINT8>(20, 0xAB) });

  ExpectSignedImagePrelude (BaseCryptLibMock);
  ExpectTbsExtractionPassthrough (BaseCryptLibMock);
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

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureRevoked);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
  EXPECT_EQ (Eval.Authority.Size, (UINTN)0);
}

//
// The prelude succeeds, the revoke-list is empty, but no allow-list anchor
// verifies the signature. The verdict is ImageSignatureNotAuthorized.
//
TEST (EvaluateSignatureTest, NotRevokedNotAuthorized_NotAuthorized) {
  MockBaseCryptLib            BaseCryptLibMock;
  DIGEST_CACHE                Cache;
  std::vector<UINT8>          CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  IMAGE_SIGNATURE_EVALUATION  Eval;

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

  SIGNATURE_LISTS  Lists = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    EvaluatePkcsSignedDataSignature (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Lists,
      &Eval
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Eval.Verdict, ImageSignatureNotAuthorized);
  EXPECT_EQ (Eval.Authority.Data, nullptr);
}
