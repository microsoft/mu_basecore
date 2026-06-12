/** @file
  Unit tests for the signature-database helpers in
  DxeImageVerificationLib (Database.c): GetImageDigestAuthority,
  LoadSignatureDatabase, LoadSignatureDatabases, IsCertRevoked,
  IsCertAuthorized, GetWinCertificatePkcs7AuthData, and
  GetImageCertAuthority. The database helpers are exercised against
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
  IsCertRevoked (
    IN  CONST UINT8  *AuthData,
    IN  UINTN        AuthDataSize,
    IN  CONST UINT8  *ImageHash,
    IN  UINTN        ImageHashSize,
    IN  CONST VOID   *Dbx,
    IN  UINTN        DbxSize
    );

  BOOLEAN
  IsCertAuthorized (
    IN  CONST UINT8                *AuthData,
    IN  UINTN                      AuthDataSize,
    IN  CONST UINT8                *ImageHash,
    IN  UINTN                      ImageHashSize,
    IN  CONST SIGNATURE_DATABASES  *Databases,
    OUT IMAGE_AUTHORITY            *Authority
    );

  EFI_STATUS
  GetWinCertificatePkcs7AuthData (
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

// ---------------------------------------------------------------------------
// GetImageDigestAuthority
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

TEST (GetImageDigestAuthorityTest, NullDatabaseWithNonZeroSize_ReturnsSuccess) {
  DIGEST_CACHE     Cache;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_EQ (GetImageDigestAuthority (NULL, 1, &Cache, &Authority), EFI_SUCCESS);
  EXPECT_EQ (Authority.Data, nullptr);
  // Validate that Cache remains consistent
  EXPECT_EQ (Cache.Buffer, (const VOID *)(UINTN)1);
  EXPECT_EQ (Cache.BufferSize, (UINTN)1);
}

TEST (GetImageDigestAuthorityTest, NullDatabaseWithZeroSize_EmptyDatabaseNotFound) {
  DIGEST_CACHE     Cache;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_EQ (GetImageDigestAuthority (NULL, 0, &Cache, &Authority), EFI_SUCCESS);
  EXPECT_EQ (Authority.Data, nullptr);
  // Validate that Cache remains consistent
  EXPECT_EQ (Cache.Buffer, (const VOID *)(UINTN)1);
  EXPECT_EQ (Cache.BufferSize, (UINTN)1);
}

TEST (GetImageDigestAuthorityTest, NullCache_ReturnsInvalidParameter) {
  UINT8            Dummy     = 0;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (GetImageDigestAuthority (&Dummy, 1, NULL, &Authority), EFI_INVALID_PARAMETER);
}

TEST (GetImageDigestAuthorityTest, NullAuthority_ReturnsInvalidParameter) {
  UINT8         Dummy = 0;
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_EQ (GetImageDigestAuthority (&Dummy, 1, &Cache, NULL), EFI_INVALID_PARAMETER);
}

TEST (GetImageDigestAuthorityTest, CacheWithoutImageBinding_ReturnsInvalidParameter) {
  UINT8            Dummy = 0;
  DIGEST_CACHE     Cache;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  ZeroMem (&Cache, sizeof (Cache));
  EXPECT_EQ (GetImageDigestAuthority (&Dummy, 1, &Cache, &Authority), EFI_INVALID_PARAMETER);
}

TEST (GetImageDigestAuthorityTest, CacheWithZeroFileSize_ReturnsInvalidParameter) {
  UINT8            Dummy = 0;
  DIGEST_CACHE     Cache;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 0;

  EXPECT_EQ (GetImageDigestAuthority (&Dummy, 1, &Cache, &Authority), EFI_INVALID_PARAMETER);
}

TEST (GetImageDigestAuthorityTest, HashComputationFailure_ReturnsSecurityViolation) {
  MockBaseCryptLib    BaseCryptLibMock;
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  DIGEST_CACHE     Cache;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_SECURITY_VIOLATION);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (GetImageDigestAuthorityTest, ExactMatch_Found) {
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);
  std::vector<UINT8>  Target (kSha256DigestSize, 0xAA);

  SetEntryPayload (Db, Off, 1, Target);

  DIGEST_CACHE     Cache     = MakeBoundCache (&gEfiCertSha256Guid, Target);
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_SUCCESS);
  EXPECT_NE (Authority.Data, nullptr);
}

TEST (GetImageDigestAuthorityTest, NoMatchingEntry_NotFound) {
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  std::vector<UINT8>  Stored (kSha256DigestSize, 0xAA);

  SetEntryPayload (Db, Off, 0, Stored);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0xBB);
  DIGEST_CACHE        Cache     = MakeBoundCache (&gEfiCertSha256Guid, Digest);
  IMAGE_AUTHORITY     Authority = { NULL, 0 };

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_SUCCESS);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (GetImageDigestAuthorityTest, UnknownSignatureTypeList_Skipped) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Guid, 0, sizeof (EFI_GUID) + 16, 1);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0xAA);
  DIGEST_CACHE        Cache     = MakeBoundCache (&gEfiCertSha256Guid, Digest);
  IMAGE_AUTHORITY     Authority = { NULL, 0 };

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_SUCCESS);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (GetImageDigestAuthorityTest, MismatchedSignatureSize_Skipped) {
  // List type matches but per-entry size doesn't, so the list describes
  // a different algorithm and must be skipped.
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha384EntrySize, 1);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0xCC);
  DIGEST_CACHE        Cache     = MakeBoundCache (&gEfiCertSha256Guid, Digest);
  IMAGE_AUTHORITY     Authority = { NULL, 0 };

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_SUCCESS);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (GetImageDigestAuthorityTest, MatchInSecondList_Found) {
  // First list is the wrong algorithm, second list contains the target.
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Guid, 0, sizeof (EFI_GUID) + 16, 1);
  size_t  SecondOff = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);

  std::vector<UINT8>  Target (kSha256DigestSize, 0x77);

  SetEntryPayload (Db, SecondOff, 1, Target);

  DIGEST_CACHE     Cache     = MakeBoundCache (&gEfiCertSha256Guid, Target);
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_SUCCESS);
  EXPECT_NE (Authority.Data, nullptr);
}

TEST (GetImageDigestAuthorityTest, NonZeroSignatureHeaderSize_EntryMathCorrect) {
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

  DIGEST_CACHE     Cache     = MakeBoundCache (&gEfiCertSha256Guid, Target);
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_SUCCESS);
  EXPECT_NE (Authority.Data, nullptr);
}

TEST (GetImageDigestAuthorityTest, ZeroEntryList_NotFound) {
  // A well-formed list with zero entries must be skipped without a
  // false positive (EntryCount == 0 means the inner loop never runs).
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 0);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0x00);
  DIGEST_CACHE        Cache     = MakeBoundCache (&gEfiCertSha256Guid, Digest);
  IMAGE_AUTHORITY     Authority = { NULL, 0 };

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_SUCCESS);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (GetImageDigestAuthorityTest, MalformedDb_ReturnsCorrupted) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  ((EFI_SIGNATURE_LIST *)Db.data ())->SignatureListSize = (UINT32)(Db.size () + 1);

  std::vector<UINT8>  Digest (kSha256DigestSize, 0x00);
  DIGEST_CACHE        Cache     = MakeBoundCache (&gEfiCertSha256Guid, Digest);
  IMAGE_AUTHORITY     Authority = { NULL, 0 };

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_VOLUME_CORRUPTED);
}

TEST (GetImageDigestAuthorityTest, ZeroSizeNonNullDatabase_EmptyDatabaseNotFound) {
  UINT8            Dummy = 0;
  DIGEST_CACHE     Cache;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (const VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_EQ (GetImageDigestAuthority (&Dummy, 0, &Cache, &Authority), EFI_SUCCESS);
  EXPECT_EQ (Authority.Data, nullptr);
}

//
// A db list whose SignatureType is a supported image-hash GUID (so GetHash
// succeeds against the bound cache) but whose SignatureHeaderSize is
// inflated so SigListIterInit rejects it. The list must be skipped and no
// authority returned.
//
TEST (GetImageDigestAuthorityTest, MalformedListHeader_Skipped) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xAB);
  DIGEST_CACHE        Cache     = MakeBoundCache (&gEfiCertSha256Guid, Digest);
  IMAGE_AUTHORITY     Authority = { NULL, 0 };

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

  EXPECT_EQ (GetImageDigestAuthority (Db.data (), Db.size (), &Cache, &Authority), EFI_SUCCESS);
  EXPECT_EQ (Authority.Data, nullptr);
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
// IsTBSCertHashInDbx -- additional list-handling coverage
// ---------------------------------------------------------------------------

//
// Dbx is too small to even contain one EFI_SIGNATURE_LIST header.
// DatabaseIterInit must reject it and the helper must fail closed
// (return TRUE).
//
TEST (IsTBSCertHashInDbxTest, MalformedDbx_ReturnsTrue) {
  UINT8  TBSCert[] = { 0xDE, 0xAD };
  // Less than sizeof(EFI_SIGNATURE_LIST) -> DatabaseIterInit returns corrupted.
  std::vector<UINT8>  Dbx (4, 0);

  EXPECT_TRUE (IsTBSCertHashInDbx (TBSCert, sizeof (TBSCert), Dbx.data (), Dbx.size ()));
}

//
// Dbx contains exactly one list whose SignatureType is not any of the
// supported gEfiCertX509ShaXXXGuid values. The helper must skip it
// and return FALSE.
//
TEST (IsTBSCertHashInDbxTest, UnsupportedShaList_ReturnsFalse) {
  MockBaseCryptLib    BaseCryptLibMock;
  UINT8               TBSCert[] = { 0xDE };
  std::vector<UINT8>  Dbx;

  // gEfiCertSha256Guid is an image-hash list type, not a cert-hash list type.
  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  // X509GetTbsCertHash should not be invoked for an unsupported list type.
  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _)).Times (0);

  EXPECT_FALSE (IsTBSCertHashInDbx (TBSCert, sizeof (TBSCert), Dbx.data (), Dbx.size ()));
}

//
// Dbx is an X509-SHA384 list with no matching entry. The helper must
// take the SHA-384 branch and return FALSE.
//
TEST (IsTBSCertHashInDbxTest, Sha384List_NoMatch_ReturnsFalse) {
  MockBaseCryptLib    BaseCryptLibMock;
  UINT8               TBSCert[] = { 0xDE };
  const UINT32        EntrySize = (UINT32)(sizeof (EFI_GUID) + SHA384_DIGEST_SIZE);
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertX509Sha384Guid, 0, EntrySize, 1);

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Digest, UINTN *DigestSize) -> EFI_STATUS {
    std::memset (Digest, 0x11, SHA384_DIGEST_SIZE);
    *DigestSize = SHA384_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_FALSE (IsTBSCertHashInDbx (TBSCert, sizeof (TBSCert), Dbx.data (), Dbx.size ()));
}

//
// Dbx is an X509-SHA512 list that contains the matching cert hash.
// The helper must take the SHA-512 branch and return TRUE.
//
TEST (IsTBSCertHashInDbxTest, Sha512List_Match_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  UINT8               TBSCert[] = { 0xDE };
  const UINT32        EntrySize = (UINT32)(sizeof (EFI_GUID) + SHA512_DIGEST_SIZE);
  std::vector<UINT8>  Dbx;

  size_t  Off = AppendSignatureList (Dbx, gEfiCertX509Sha512Guid, 0, EntrySize, 1);

  SetEntryPayload (Dbx, Off, 0, std::vector<UINT8>(SHA512_DIGEST_SIZE, 0x99));

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Digest, UINTN *DigestSize) -> EFI_STATUS {
    std::memset (Digest, 0x99, SHA512_DIGEST_SIZE);
    *DigestSize = SHA512_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_TRUE (IsTBSCertHashInDbx (TBSCert, sizeof (TBSCert), Dbx.data (), Dbx.size ()));
}

//
// The hash routine itself fails (X509GetTbsCertHash returns an error).
// The helper must fail closed and return TRUE.
//
TEST (IsTBSCertHashInDbxTest, HashFails_FailsClosed_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  UINT8               TBSCert[] = { 0xDE };
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  EXPECT_TRUE (IsTBSCertHashInDbx (TBSCert, sizeof (TBSCert), Dbx.data (), Dbx.size ()));
}

//
// Dbx X509-SHA256 list whose SignatureSize is too small to contain a
// 32-byte digest. The helper must fail closed and return TRUE.
//
TEST (IsTBSCertHashInDbxTest, SignatureSizeTooSmall_FailsClosed_ReturnsTrue) {
  MockBaseCryptLib  BaseCryptLibMock;
  UINT8             TBSCert[] = { 0xDE };
  // EntrySize = GUID + 16 bytes -- smaller than required for a SHA-256 digest.
  const UINT32        EntrySize = (UINT32)(sizeof (EFI_GUID) + 16);
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, EntrySize, 1);

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Digest, UINTN *DigestSize) -> EFI_STATUS {
    std::memset (Digest, 0x00, SHA256_DIGEST_SIZE);
    *DigestSize = SHA256_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_TRUE (IsTBSCertHashInDbx (TBSCert, sizeof (TBSCert), Dbx.data (), Dbx.size ()));
}

//
// Dbx X509-SHA256 list that passes the size check but whose
// SignatureHeaderSize is inconsistent with SignatureListSize, causing
// SigListIterInit to fail. The helper must fail closed and return
// TRUE.
//
TEST (IsTBSCertHashInDbxTest, SigListIterInitFails_FailsClosed_ReturnsTrue) {
  MockBaseCryptLib    BaseCryptLibMock;
  UINT8               TBSCert[] = { 0xDE };
  std::vector<UINT8>  Dbx;

  // Build a list with a SignatureHeaderSize larger than the list itself
  // allows. We size the list to contain a single SHA-256 cert-hash entry
  // (so the size check at the top of IsX509HashInList passes), but
  // the inflated SignatureHeaderSize makes SigListIterInit reject it.
  const UINT32  EntrySize = kSha256EntrySize;
  const UINT32  ListSize  = (UINT32)(sizeof (EFI_SIGNATURE_LIST) + EntrySize);

  Dbx.resize (ListSize, 0);

  EFI_SIGNATURE_LIST  *List = (EFI_SIGNATURE_LIST *)Dbx.data ();

  CopyMem (&List->SignatureType, &gEfiCertX509Sha256Guid, sizeof (EFI_GUID));
  List->SignatureListSize = ListSize;
  // SignatureHeaderSize > SignatureListSize - sizeof (EFI_SIGNATURE_LIST).
  List->SignatureHeaderSize = ListSize;
  List->SignatureSize       = EntrySize;

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Digest, UINTN *DigestSize) -> EFI_STATUS {
    std::memset (Digest, 0x00, SHA256_DIGEST_SIZE);
    *DigestSize = SHA256_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_TRUE (IsTBSCertHashInDbx (TBSCert, sizeof (TBSCert), Dbx.data (), Dbx.size ()));
}

//
// Dbx contains two X509-SHA256 lists for the same certificate. The
// TBS digest should be computed once and reused for the second list.
//
TEST (IsTBSCertHashInDbxTest, RepeatedSha256Lists_UsesCachedDigest_ReturnsFalse) {
  MockBaseCryptLib    BaseCryptLibMock;
  UINT8               TBSCert[] = { 0xDE };
  std::vector<UINT8>  Dbx;

  size_t  Off0 = AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);
  size_t  Off1 = AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);

  SetEntryPayload (Dbx, Off0, 0, std::vector<UINT8>(SHA256_DIGEST_SIZE, 0x11));
  SetEntryPayload (Dbx, Off1, 0, std::vector<UINT8>(SHA256_DIGEST_SIZE, 0x22));

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .Times (1)
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Digest, UINTN *DigestSize) -> EFI_STATUS {
    std::memset (Digest, 0xAA, SHA256_DIGEST_SIZE);
    *DigestSize = SHA256_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_FALSE (IsTBSCertHashInDbx (TBSCert, sizeof (TBSCert), Dbx.data (), Dbx.size ()));
}

static const std::vector<UINT8>  kAuthDataDefault  = std::vector<UINT8>(16, 0xA1);
static const std::vector<UINT8>  kImageHashDefault = std::vector<UINT8>(SHA256_DIGEST_SIZE, 0x55);

// ---------------------------------------------------------------------------
// IsCertRevoked -- parameter validation
// ---------------------------------------------------------------------------

TEST (IsCertRevokedTest, NullAuthData_ReturnsTrue) {
  std::vector<UINT8>  Dbx (16, 0);

  EXPECT_TRUE (
    IsCertRevoked (
      NULL,
      0,
      kImageHashDefault.data (),
      kImageHashDefault.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

TEST (IsCertRevokedTest, ZeroAuthDataSize_ReturnsTrue) {
  std::vector<UINT8>  Dbx (16, 0);

  EXPECT_TRUE (
    IsCertRevoked (
      kAuthDataDefault.data (),
      0,
      kImageHashDefault.data (),
      kImageHashDefault.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

TEST (IsCertRevokedTest, NullImageHash_ReturnsTrue) {
  std::vector<UINT8>  Dbx (16, 0);

  EXPECT_TRUE (
    IsCertRevoked (
      kAuthDataDefault.data (),
      kAuthDataDefault.size (),
      NULL,
      0,
      Dbx.data (),
      Dbx.size ()
      )
    );
}

TEST (IsCertRevokedTest, ZeroImageHashSize_ReturnsTrue) {
  std::vector<UINT8>  Dbx (16, 0);

  EXPECT_TRUE (
    IsCertRevoked (
      kAuthDataDefault.data (),
      kAuthDataDefault.size (),
      kImageHashDefault.data (),
      0,
      Dbx.data (),
      Dbx.size ()
      )
    );
}

TEST (IsCertRevokedTest, NullDbx_ReturnsFalse) {
  EXPECT_FALSE (
    IsCertRevoked (
      kAuthDataDefault.data (),
      kAuthDataDefault.size (),
      kImageHashDefault.data (),
      kImageHashDefault.size (),
      NULL,
      0
      )
    );
}

TEST (IsCertRevokedTest, EmptyDbx_ReturnsFalse) {
  std::vector<UINT8>  Dbx;

  EXPECT_FALSE (
    IsCertRevoked (
      kAuthDataDefault.data (),
      kAuthDataDefault.size (),
      kImageHashDefault.data (),
      kImageHashDefault.size (),
      Dbx.data (),
      0
      )
    );
}

// ---------------------------------------------------------------------------
// IsCertAuthorized -- parameter validation
// ---------------------------------------------------------------------------

TEST (IsCertAuthorizedTest, NullAuthData_ReturnsFalse) {
  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };
  IMAGE_AUTHORITY      Authority = { NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      NULL,
      0,
      kImageHashDefault.data (),
      kImageHashDefault.size (),
      &Databases,
      &Authority
      )
    );
}

TEST (IsCertAuthorizedTest, ZeroAuthDataSize_ReturnsFalse) {
  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };
  IMAGE_AUTHORITY      Authority = { NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      kAuthDataDefault.data (),
      0,
      kImageHashDefault.data (),
      kImageHashDefault.size (),
      &Databases,
      &Authority
      )
    );
}

TEST (IsCertAuthorizedTest, NullImageHash_ReturnsFalse) {
  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };
  IMAGE_AUTHORITY      Authority = { NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      kAuthDataDefault.data (),
      kAuthDataDefault.size (),
      NULL,
      0,
      &Databases,
      &Authority
      )
    );
}

TEST (IsCertAuthorizedTest, NullDatabases_ReturnsFalse) {
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      kAuthDataDefault.data (),
      kAuthDataDefault.size (),
      kImageHashDefault.data (),
      kImageHashDefault.size (),
      NULL,
      &Authority
      )
    );
}

TEST (IsCertAuthorizedTest, NullAuthority_ReturnsFalse) {
  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      kAuthDataDefault.data (),
      kAuthDataDefault.size (),
      kImageHashDefault.data (),
      kImageHashDefault.size (),
      &Databases,
      NULL
      )
    );
}

TEST (IsCertAuthorizedTest, ZeroImageHashSize_ReturnsFalse) {
  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };
  IMAGE_AUTHORITY      Authority = { NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      kAuthDataDefault.data (),
      kAuthDataDefault.size (),
      kImageHashDefault.data (),
      0,
      &Databases,
      &Authority
      )
    );
}

// ---------------------------------------------------------------------------
// GetWinCertificatePkcs7AuthData
// ---------------------------------------------------------------------------

TEST (GetWinCertificatePkcs7AuthDataTest, NullCert_ReturnsInvalidParameter) {
  const UINT8  *AuthData    = NULL;
  UINTN        AuthDataSize = 0;

  EXPECT_EQ (
    GetWinCertificatePkcs7AuthData (NULL, &AuthData, &AuthDataSize),
    EFI_INVALID_PARAMETER
    );
}

TEST (GetWinCertificatePkcs7AuthDataTest, NullOutputs_ReturnsInvalidParameter) {
  WIN_CERTIFICATE  Cert         = { sizeof (WIN_CERTIFICATE) + 1, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA };
  const UINT8      *AuthData    = NULL;
  UINTN            AuthDataSize = 0;

  EXPECT_EQ (
    GetWinCertificatePkcs7AuthData (&Cert, NULL, &AuthDataSize),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    GetWinCertificatePkcs7AuthData (&Cert, &AuthData, NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST (GetWinCertificatePkcs7AuthDataTest, PkcsSignedData_ExtractsPayload) {
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
    GetWinCertificatePkcs7AuthData (Cert, &AuthData, &AuthDataSize),
    EFI_SUCCESS
    );
  ASSERT_EQ (AuthDataSize, sizeof (Payload));
  EXPECT_EQ (0, std::memcmp (AuthData, Payload, sizeof (Payload)));
}

TEST (GetWinCertificatePkcs7AuthDataTest, PkcsSignedData_HeaderOnly_ReturnsCorrupted) {
  WIN_CERTIFICATE  Cert         = { sizeof (WIN_CERTIFICATE), 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA };
  const UINT8      *AuthData    = NULL;
  UINTN            AuthDataSize = 0;

  EXPECT_EQ (
    GetWinCertificatePkcs7AuthData (&Cert, &AuthData, &AuthDataSize),
    EFI_VOLUME_CORRUPTED
    );
}

TEST (GetWinCertificatePkcs7AuthDataTest, EfiGuidPkcs7_ExtractsPayload) {
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
    GetWinCertificatePkcs7AuthData (&UefiCert->Hdr, &AuthData, &AuthDataSize),
    EFI_SUCCESS
    );
  ASSERT_EQ (AuthDataSize, sizeof (Payload));
  EXPECT_EQ (0, std::memcmp (AuthData, Payload, sizeof (Payload)));
}

TEST (GetWinCertificatePkcs7AuthDataTest, EfiGuidNonPkcs7_ReturnsUnsupported) {
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
    GetWinCertificatePkcs7AuthData (&UefiCert->Hdr, &AuthData, &AuthDataSize),
    EFI_UNSUPPORTED
    );
}

TEST (GetWinCertificatePkcs7AuthDataTest, EfiGuid_HeaderOnly_ReturnsCorrupted) {
  WIN_CERTIFICATE_UEFI_GUID  UefiCert;

  ZeroMem (&UefiCert, sizeof (UefiCert));
  UefiCert.Hdr.dwLength         = (UINT32)OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  UefiCert.Hdr.wRevision        = 0x0200;
  UefiCert.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;

  const UINT8  *AuthData    = NULL;
  UINTN        AuthDataSize = 0;

  EXPECT_EQ (
    GetWinCertificatePkcs7AuthData (&UefiCert.Hdr, &AuthData, &AuthDataSize),
    EFI_VOLUME_CORRUPTED
    );
}

TEST (GetWinCertificatePkcs7AuthDataTest, UnknownCertType_ReturnsUnsupported) {
  WIN_CERTIFICATE  Cert         = { sizeof (WIN_CERTIFICATE) + 8, 0x0200, WIN_CERT_TYPE_EFI_PKCS115 };
  const UINT8      *AuthData    = NULL;
  UINTN            AuthDataSize = 0;

  EXPECT_EQ (
    GetWinCertificatePkcs7AuthData (&Cert, &AuthData, &AuthDataSize),
    EFI_UNSUPPORTED
    );
}

// ---------------------------------------------------------------------------
// IsCertAuthorized -- end-to-end PKCS#7 + dbx scenarios
// ---------------------------------------------------------------------------

//
// One PKCS#7 signature whose only valid trust anchor in db has its
// hash listed in dbx. The cert must not be authorized.
//
TEST (IsCertAuthorizedTest, SignatureCertInDbAndDbx_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  // db: one X509 list with one cert (payload byte == 0x11).
  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (
                                Db,
                                gEfiCertX509Guid,
                                0,
                                sizeof (EFI_GUID) + 16,
                                1
                                );

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  // dbx: one X509-SHA256 list with the digest the mocked X509GetTbsCertHash
  // will produce for the cert's TBS bytes (0xE1 * 32).
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (
                                 Dbx,
                                 gEfiCertX509Sha256Guid,
                                 0,
                                 kSha256EntrySize,
                                 1
                                 );

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(kSha256DigestSize, 0xE1));

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _))
    .WillOnce (Return (TRUE));
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutTbs, UINTN *OutTbsSize) -> BOOLEAN {
    static UINT8  TbsBytes[] = { 0x11 };
    *OutTbs                  = TbsBytes;
    *OutTbsSize              = sizeof (TbsBytes);
    return TRUE;
  }
         )
       );
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

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

//
// Happy path: a single PKCS#7 signature, db contains an X509 trust
// anchor that AuthenticodeVerify accepts, and no dbx. IsCertAuthorized
// must return TRUE. X509GetTBSCert is still called for the verifying
// trust anchor, but dbx hashing is skipped because dbx is empty.
//
TEST (IsCertAuthorizedTest, SingleSignatureVerifies_NoDbx_ReturnsTrue) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (
                                Db,
                                gEfiCertX509Guid,
                                0,
                                sizeof (EFI_GUID) + 16,
                                1
                                );

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _))
    .WillOnce (Return (TRUE));
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutTbs, UINTN *OutTbsSize) -> BOOLEAN {
    static UINT8  TbsBytes[] = { 0x11 };
    *OutTbs                  = TbsBytes;
    *OutTbsSize              = sizeof (TbsBytes);
    return TRUE;
  }
         )
       );

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_TRUE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

//
// A PKCS#7 signature exists but no db cert verifies it. IsCertAuthorized
// must return FALSE.
//
TEST (IsCertAuthorizedTest, SignatureDoesNotVerifyAnyDbCert_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (
                                Db,
                                gEfiCertX509Guid,
                                0,
                                sizeof (EFI_GUID) + 16,
                                2
                                );

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));
  SetEntryPayload (Db, DbOff, 1, std::vector<UINT8>(16, 0x22));

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _))
    .Times (2)
    .WillRepeatedly (Return (FALSE));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

//
// db is absent (NULL/0) but the cert carries a valid-looking PKCS#7
// signature. With no trust anchors at all IsCertAuthorized must return
// FALSE, and no crypto must be invoked.
//
TEST (IsCertAuthorizedTest, NoDbButValidSignature_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  // No db => the db walk visits nothing => AuthenticodeVerify is never called.
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

// ---------------------------------------------------------------------------
// IsCertAuthorized -- additional signature-list handling coverage
// ---------------------------------------------------------------------------

//
// db contains only a non-X.509 list (image-hash list). The db walk
// inspects it but skips it; with no trust anchors available the cert
// must not be authorized.
//
TEST (IsCertAuthorizedTest, DbHasOnlyNonX509List_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  // db: one image-hash list (gEfiCertSha256Guid) with a non-matching digest.
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  // No X.509 list => AuthenticodeVerify is never called.
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

//
// db contains an X.509 list whose SignatureSize equals sizeof(EFI_GUID),
// i.e. no cert payload at all. The list must be skipped and the cert
// must not be authorized.
//
TEST (IsCertAuthorizedTest, DbX509ListNoCertPayload_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  // X.509 list with SignatureSize = sizeof(EFI_GUID) (no cert payload).
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)sizeof (EFI_GUID), 1);

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

//
// db contains an X.509 list whose SignatureHeaderSize is inflated past
// SignatureListSize - sizeof(EFI_SIGNATURE_LIST). DatabaseIterInit
// accepts the list (it only validates SignatureListSize), but
// SigListIterInit inside IsPkcs7AuthDataVerifiedByX509 rejects it.
// The list must be skipped and the cert must not be authorized.
//
TEST (IsCertAuthorizedTest, DbX509ListMalformedHeader_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  // Build a single X.509 list with SignatureSize > sizeof(EFI_GUID) (so
  // the early-return check in IsPkcs7AuthDataVerifiedByX509 is passed)
  // but a SignatureHeaderSize that overflows the list payload area.
  const UINT32        EntrySize = (UINT32)(sizeof (EFI_GUID) + 16);
  const UINT32        ListSize  = (UINT32)(sizeof (EFI_SIGNATURE_LIST) + EntrySize);
  std::vector<UINT8>  Db (ListSize, 0);

  EFI_SIGNATURE_LIST  *List = (EFI_SIGNATURE_LIST *)Db.data ();

  CopyMem (&List->SignatureType, &gEfiCertX509Guid, sizeof (EFI_GUID));
  List->SignatureListSize   = ListSize;
  List->SignatureHeaderSize = ListSize;          // > ListSize - sizeof(EFI_SIGNATURE_LIST)
  List->SignatureSize       = EntrySize;

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

//
// db is malformed (less than one EFI_SIGNATURE_LIST header). The
// per-signature DatabaseIterInit call fails and IsCertAuthorized must
// return FALSE.
//
TEST (IsCertAuthorizedTest, MalformedDb_RejectsImage) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  // Less than sizeof(EFI_SIGNATURE_LIST) -> DatabaseIterInit fails.
  std::vector<UINT8>  Db (4, 0);

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

//
// db contains an X.509 (full-cert) list whose entry AuthenticodeVerify
// accepts, but X509GetTBSCert fails to extract the TBSCertificate. The
// helper must fail closed and the image must not be authorized.
//
TEST (IsCertAuthorizedTest, DbX509AuthenticodeVerifiesButTbsCertFails_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (Db, gEfiCertX509Guid, 0, (UINT32)(sizeof (EFI_GUID) + 16), 1);

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _))
    .WillOnce (Return (TRUE));
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (Return (FALSE));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
  EXPECT_EQ (Authority.Data, nullptr);
}

// ---------------------------------------------------------------------------
// IsCertAuthorized -- X.509 TBS-cert-hash (trust anchor) list handling
// ---------------------------------------------------------------------------

//
// db contains an EFI_CERT_X509_SHA256 (TBS-cert-hash) list. A trust
// anchor is recovered from the PKCS#7 auth data via
// GetTrustAnchorX509FromAuthData, and its TBS hash is not present in an
// (empty) dbx. The image must be authorized and Authority must point at
// the matching db entry. The non-NULL cache handle produced by the
// lookup must be released via FreeTrustAnchorX509Cache.
//
TEST (IsCertAuthorizedTest, X509HashListTrustAnchorNotRevoked_ReturnsTrue) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);

  // The returned trust anchor is freed by the library, so it must be
  // pool-allocated. A non-NULL cache handle is also produced.
  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID **CacheHandle, CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, UINT8 **TrustAnchor, UINTN *TrustAnchorSize) -> EFI_STATUS {
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

  EXPECT_CALL (BaseCryptLibMock, FreeTrustAnchorX509Cache (_)).Times (1);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_TRUE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
  EXPECT_NE (Authority.Data, nullptr);
}

//
// db contains an EFI_CERT_X509_SHA256 list with two entries, but neither
// entry recovers a trust anchor from the auth data (EFI_NOT_FOUND). The
// helper must walk both entries and the image must not be authorized.
//
TEST (IsCertAuthorizedTest, X509HashListNoTrustAnchor_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 2);

  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .Times (2)
    .WillRepeatedly (Return (EFI_NOT_FOUND));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
  EXPECT_EQ (Authority.Data, nullptr);
}

//
// db contains an EFI_CERT_X509_SHA256 list; the trust-anchor lookup
// fails with a hard error (not EFI_NOT_FOUND). The helper must fail
// closed and the image must not be authorized.
//
TEST (IsCertAuthorizedTest, X509HashListTrustAnchorLookupError_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
  EXPECT_EQ (Authority.Data, nullptr);
}

//
// db contains an EFI_CERT_X509_SHA256 list; a trust anchor is recovered
// from the auth data, but its TBS hash is enrolled in dbx. The anchor
// must be skipped and, with no other entries, the image is not
// authorized.
//
TEST (IsCertAuthorizedTest, X509HashListTrustAnchorRevoked_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);

  // dbx: one X509-SHA256 list holding the digest the mocked
  // X509GetTbsCertHash produces for the recovered anchor (0xC3 * 32).
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (Dbx, gEfiCertX509Sha256Guid, 0, kSha256EntrySize, 1);

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(kSha256DigestSize, 0xC3));

  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID **, CONST UINT8 *, UINTN, CONST UINT8 *, UINTN, UINT8 **TrustAnchor, UINTN *TrustAnchorSize) -> EFI_STATUS {
    static const UINT8  CertBytes[] = { 0x30, 0x82, 0x01, 0x02 };
    *TrustAnchor                    = (UINT8 *)AllocateCopyPool (sizeof (CertBytes), CertBytes);
    *TrustAnchorSize                = sizeof (CertBytes);
    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID *, UINTN, CONST EFI_GUID *, UINT8 *Digest, UINTN *DigestSize) -> EFI_STATUS {
    std::memset (Digest, 0xC3, SHA256_DIGEST_SIZE);
    *DigestSize = SHA256_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), Dbx.data (), Dbx.size () };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
  EXPECT_EQ (Authority.Data, nullptr);
}

//
// db contains an EFI_CERT_X509_SHA256 list whose SignatureSize is only
// sizeof(EFI_GUID) -- no TBS-hash payload. The list must be skipped (no
// trust-anchor lookup attempted) and the image must not be authorized.
//
TEST (IsCertAuthorizedTest, X509HashListNoCertPayload_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertX509Sha256Guid, 0, (UINT32)sizeof (EFI_GUID), 1);

  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

//
// db contains an EFI_CERT_X509_SHA256 list whose SignatureHeaderSize is
// inflated past the list payload, so SigListIterInit rejects it. The list
// must be skipped and the image must not be authorized.
//
TEST (IsCertAuthorizedTest, X509HashListMalformedHeader_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  const UINT32        EntrySize = kSha256EntrySize;
  const UINT32        ListSize  = (UINT32)(sizeof (EFI_SIGNATURE_LIST) + EntrySize);
  std::vector<UINT8>  Db (ListSize, 0);

  EFI_SIGNATURE_LIST  *List = (EFI_SIGNATURE_LIST *)Db.data ();

  CopyMem (&List->SignatureType, &gEfiCertX509Sha256Guid, sizeof (EFI_GUID));
  List->SignatureListSize   = ListSize;
  List->SignatureHeaderSize = ListSize;        // > ListSize - sizeof (EFI_SIGNATURE_LIST)
  List->SignatureSize       = EntrySize;

  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _)).Times (0);

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_FALSE (
    IsCertAuthorized (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      &Databases,
      &Authority
      )
    );
}

// ---------------------------------------------------------------------------
// IsCertRevoked -- end-to-end PKCS#7 + dbx scenarios
// ---------------------------------------------------------------------------

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
// dbx contains an X.509 list whose entry AuthenticodeVerify accepts as
// a trust anchor for the cert's signature. Step 1 short-circuits and
// the cert is revoked. Pkcs7GetSigners must not be reached.
//
TEST (IsCertRevokedTest, DbxX509VerifiesSignature_ReturnsTrue) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (
                                 Dbx,
                                 gEfiCertX509Guid,
                                 0,
                                 sizeof (EFI_GUID) + 16,
                                 1
                                 );

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(16, 0x11));

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _))
    .WillOnce (Return (TRUE));
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _)).Times (0);

  EXPECT_TRUE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// dbx X.509 list does not verify the signature, and Pkcs7GetSigners
// reports no recoverable signer chain. The cert is not revoked.
//
TEST (IsCertRevokedTest, DbxX509DoesNotVerify_PkcsSignersFails_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (
                                 Dbx,
                                 gEfiCertX509Guid,
                                 0,
                                 sizeof (EFI_GUID) + 16,
                                 1
                                 );

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(16, 0x22));

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _))
    .WillOnce (Return (FALSE));
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (Return (FALSE));

  EXPECT_FALSE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// dbx has only hash-typed lists (no X.509 entries). Step 1 calls
// AuthenticodeVerify zero times; step 2 attempts the signer chain.
//
TEST (IsCertRevokedTest, DbxOnlyHashLists_PkcsSignersFails_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (Return (FALSE));

  EXPECT_FALSE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// dbx X.509 list whose SignatureSize equals sizeof(EFI_GUID) carries
// no cert payload; step 1 must skip the list rather than treating
// it as a verifying anchor.
//
TEST (IsCertRevokedTest, DbxX509ListNoCertPayload_StepOneSkipped_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertX509Guid, 0, sizeof (EFI_GUID), 1);

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (Return (FALSE));

  EXPECT_FALSE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// dbx contains an X509-SHA256 list whose digest matches the TBS hash
// of the signer reported by Pkcs7GetSigners. Step 1 finds no verifying
// anchor; step 2 hashes the signer's TBS and IsTBSCertHashInDbx fires.
//
TEST (IsCertRevokedTest, SignerTBSHashInDbx_ReturnsTrue) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (
                                 Dbx,
                                 gEfiCertX509Sha256Guid,
                                 0,
                                 kSha256EntrySize,
                                 1
                                 );

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(kSha256DigestSize, 0xE1));

  static std::vector<UINT8>  Stack =
    MakeCertStack (
      { std::vector<UINT8>{ 0x11, 0x22, 0x33 }
      }
      );

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutStack, UINTN *OutStackLen, UINT8 **OutTrusted, UINTN *OutTrustedLen) -> BOOLEAN {
    *OutStack      = Stack.data ();
    *OutStackLen   = Stack.size ();
    *OutTrusted    = NULL;
    *OutTrustedLen = 0;
    return TRUE;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutTbs, UINTN *OutTbsSize) -> BOOLEAN {
    static UINT8  Tbs[] = { 0x11 };
    *OutTbs             = Tbs;
    *OutTbsSize         = sizeof (Tbs);
    return TRUE;
  }
         )
       );
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
  EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (_)).Times (1);

  EXPECT_TRUE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// Pkcs7GetSigners reports a single signer whose TBS hash is not in
// dbx. The cert is not revoked.
//
TEST (IsCertRevokedTest, SignerTBSHashNotInDbx_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (
                                 Dbx,
                                 gEfiCertX509Sha256Guid,
                                 0,
                                 kSha256EntrySize,
                                 1
                                 );

  // Plant a digest that the mocked X509GetTbsCertHash will never produce.
  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(kSha256DigestSize, 0xAA));

  static std::vector<UINT8>  Stack =
    MakeCertStack (
      { std::vector<UINT8>{ 0x11, 0x22, 0x33 }
      }
      );

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutStack, UINTN *OutStackLen, UINT8 **OutTrusted, UINTN *OutTrustedLen) -> BOOLEAN {
    *OutStack      = Stack.data ();
    *OutStackLen   = Stack.size ();
    *OutTrusted    = NULL;
    *OutTrustedLen = 0;
    return TRUE;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutTbs, UINTN *OutTbsSize) -> BOOLEAN {
    static UINT8  Tbs[] = { 0x11 };
    *OutTbs             = Tbs;
    *OutTbsSize         = sizeof (Tbs);
    return TRUE;
  }
         )
       );
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
  EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (_)).Times (1);

  EXPECT_FALSE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// Pkcs7GetSigners reports a non-NULL stack with CertNumber == 0.
// Step 2 has nothing to walk and the cert is not revoked.
//
TEST (IsCertRevokedTest, PkcsSignersEmptyStack_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  static std::vector<UINT8>  EmptyStack = MakeCertStack ({ });

  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutStack, UINTN *OutStackLen, UINT8 **OutTrusted, UINTN *OutTrustedLen) -> BOOLEAN {
    *OutStack      = EmptyStack.data ();
    *OutStackLen   = EmptyStack.size ();
    *OutTrusted    = NULL;
    *OutTrustedLen = 0;
    return TRUE;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (_)).Times (1);

  EXPECT_FALSE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// X509GetTBSCert fails on the signer; treat the chain as poisoned.
//
TEST (IsCertRevokedTest, X509GetTBSCertFails_ReturnsTrue) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  static std::vector<UINT8>  Stack =
    MakeCertStack (
      { std::vector<UINT8>{ 0x11, 0x22, 0x33 }
      }
      );

  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutStack, UINTN *OutStackLen, UINT8 **OutTrusted, UINTN *OutTrustedLen) -> BOOLEAN {
    *OutStack      = Stack.data ();
    *OutStackLen   = Stack.size ();
    *OutTrusted    = NULL;
    *OutTrustedLen = 0;
    return TRUE;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (Return (FALSE));
  EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (_)).Times (1);

  EXPECT_TRUE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// Pkcs7GetSigners returns a stack whose CertNumber claims one cert
// but whose length field would run past the end of the buffer. Step 2
// detects the malformed payload and treats the chain as poisoned.
//
TEST (IsCertRevokedTest, MalformedSignerStack_ReturnsTrue) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  // CertNumber=1, length=0x10, but only 4 payload bytes follow.
  static std::vector<UINT8>  BadStack = {
    0x01, 0x10, 0x00, 0x00, 0x00, 0xAA, 0xBB, 0xCC, 0xDD
  };

  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutStack, UINTN *OutStackLen, UINT8 **OutTrusted, UINTN *OutTrustedLen) -> BOOLEAN {
    *OutStack      = BadStack.data ();
    *OutStackLen   = BadStack.size ();
    *OutTrusted    = NULL;
    *OutTrustedLen = 0;
    return TRUE;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (_)).Times (1);

  EXPECT_TRUE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// Pkcs7GetSigners returns a stack that claims a signer but truncates the
// per-cert length prefix (fewer than 4 bytes remain). IsCertRevoked must
// fail closed and report the cert as revoked.
//
TEST (IsCertRevokedTest, MalformedSignerStackLengthPrefix_ReturnsTrue) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  // CertNumber=1, but only 2 bytes follow -- fewer than the 4-byte length
  // prefix the parser expects.
  static std::vector<UINT8>  BadStack = { 0x01, 0xAA, 0xBB };

  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutStack, UINTN *OutStackLen, UINT8 **OutTrusted, UINTN *OutTrustedLen) -> BOOLEAN {
    *OutStack      = BadStack.data ();
    *OutStackLen   = BadStack.size ();
    *OutTrusted    = NULL;
    *OutTrustedLen = 0;
    return TRUE;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (_)).Times (1);

  EXPECT_TRUE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// Both CertStack and TrustedCert are returned by Pkcs7GetSigners.
// Both must be freed exactly once via Pkcs7FreeSigners.
//
TEST (IsCertRevokedTest, BothPkcsBuffersFreed_ReturnsFalse) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  static std::vector<UINT8>  Stack            = MakeCertStack ({ });
  static UINT8               TrustedCertBuf[] = { 0xDE, 0xAD, 0xBE, 0xEF };

  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutStack, UINTN *OutStackLen, UINT8 **OutTrusted, UINTN *OutTrustedLen) -> BOOLEAN {
    *OutStack      = Stack.data ();
    *OutStackLen   = Stack.size ();
    *OutTrusted    = TrustedCertBuf;
    *OutTrustedLen = sizeof (TrustedCertBuf);
    return TRUE;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (_)).Times (2);

  EXPECT_FALSE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

//
// dbx is too small to contain even one EFI_SIGNATURE_LIST header,
// so DatabaseIterInit fails. Step 1 fails closed.
//
TEST (IsCertRevokedTest, MalformedDbx_ReturnsTrue) {
  MockBaseCryptLib  BaseCryptLibMock;

  std::vector<UINT8>  AuthData (16, 0xA1);
  std::vector<UINT8>  ImageHash (SHA256_DIGEST_SIZE, 0x55);

  // Less than sizeof(EFI_SIGNATURE_LIST) -> DatabaseIterInit fails.
  std::vector<UINT8>  Dbx (4, 0);

  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _)).Times (0);

  EXPECT_TRUE (
    IsCertRevoked (
      AuthData.data (),
      AuthData.size (),
      ImageHash.data (),
      ImageHash.size (),
      Dbx.data (),
      Dbx.size ()
      )
    );
}

// ---------------------------------------------------------------------------
// GetImageCertAuthority -- prelude (GetWinCertificatePkcs7AuthData,
// GetAuthenticodeHashAlgorithm, GetHash) plus IsCertRevoked / IsCertAuthorized
// dispatch.
// ---------------------------------------------------------------------------

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

TEST (GetImageCertAuthorityTest, NullCert_ReturnsInvalidParameter) {
  DIGEST_CACHE         Cache;
  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };
  IMAGE_AUTHORITY      Authority = { NULL, 0 };

  InitImageCache (Cache);
  EXPECT_EQ (GetImageCertAuthority (NULL, &Cache, &Databases, &Authority), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (GetImageCertAuthorityTest, NullCache_ReturnsInvalidParameter) {
  std::vector<UINT8>   CertBuf   = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };
  IMAGE_AUTHORITY      Authority = { NULL, 0 };

  EXPECT_EQ (
    GetImageCertAuthority (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      NULL,
      &Databases,
      &Authority
      ),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (GetImageCertAuthorityTest, NullDatabases_ReturnsInvalidParameter) {
  std::vector<UINT8>  CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));
  DIGEST_CACHE        Cache;
  IMAGE_AUTHORITY     Authority = { NULL, 0 };

  InitImageCache (Cache);
  EXPECT_EQ (
    GetImageCertAuthority (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      NULL,
      &Authority
      ),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (Authority.Data, nullptr);
}

//
// GetWinCertificatePkcs7AuthData fails (unsupported WIN_CERTIFICATE type) so
// the prelude bails before touching crypto.
//
TEST (GetImageCertAuthorityTest, UnsupportedCertType_ReturnsError) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;

  WIN_CERTIFICATE  Cert = { sizeof (WIN_CERTIFICATE) + 8, 0x0200, WIN_CERT_TYPE_EFI_PKCS115 };

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };
  IMAGE_AUTHORITY      Authority = { NULL, 0 };

  InitImageCache (Cache);

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);

  // A prelude failure is folded into EFI_ACCESS_DENIED; no hash algorithm was
  // established so SignatureType is left as the zero GUID.
  EXPECT_EQ (GetImageCertAuthority (&Cert, &Cache, &Databases, &Authority), EFI_ACCESS_DENIED);
  EXPECT_EQ (Authority.Data, nullptr);
  EXPECT_TRUE (IsZeroBuffer (&Authority.SignatureType, sizeof (EFI_GUID)));
}

//
// GetAuthenticodeHashAlgorithm fails -> prelude bails, hash and verify are
// never invoked.
//
TEST (GetImageCertAuthorityTest, HashAlgorithmFails_ReturnsError) {
  MockBaseCryptLib    BaseCryptLibMock;
  DIGEST_CACHE        Cache;
  std::vector<UINT8>  CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };
  IMAGE_AUTHORITY      Authority = { NULL, 0 };

  InitImageCache (Cache);

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (Return (EFI_UNSUPPORTED));
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);

  // A prelude failure is folded into EFI_ACCESS_DENIED with a zero SignatureType.
  EXPECT_EQ (
    GetImageCertAuthority (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Authority
      ),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Authority.Data, nullptr);
  EXPECT_TRUE (IsZeroBuffer (&Authority.SignatureType, sizeof (EFI_GUID)));
}

//
// GetAuthenticodeHash fails -> GetHash returns SECURITY_VIOLATION and the
// prelude bails before any revocation / authorization decision.
//
TEST (GetImageCertAuthorityTest, GetHashFails_ReturnsError) {
  MockBaseCryptLib    BaseCryptLibMock;
  DIGEST_CACHE        Cache;
  std::vector<UINT8>  CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));

  SIGNATURE_DATABASES  Databases = { NULL, 0, NULL, 0 };
  IMAGE_AUTHORITY      Authority = { NULL, 0 };

  InitImageCache (Cache);

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
    .WillOnce (Return (EFI_DEVICE_ERROR));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _)).Times (0);

  // GetHash failure is a prelude failure -> EFI_ACCESS_DENIED, zero SignatureType.
  EXPECT_EQ (
    GetImageCertAuthority (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Authority
      ),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Authority.Data, nullptr);
  EXPECT_TRUE (IsZeroBuffer (&Authority.SignatureType, sizeof (EFI_GUID)));
}

//
// Happy path: prelude succeeds, dbx is empty so IsCertRevoked returns FALSE,
// the first db trust anchor verifies via AuthenticodeVerify, and X509GetTBSCert
// succeeds. GetImageCertAuthority returns EFI_SUCCESS with a non-NULL authority.
//
TEST (GetImageCertAuthorityTest, AuthorizedByDb_ReturnsAuthority) {
  MockBaseCryptLib    BaseCryptLibMock;
  DIGEST_CACHE        Cache;
  std::vector<UINT8>  CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  InitImageCache (Cache);

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (
                                Db,
                                gEfiCertX509Guid,
                                0,
                                sizeof (EFI_GUID) + 16,
                                1
                                );

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillRepeatedly (
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

  // dbx is empty so phase 1 of IsCertRevoked never calls AuthenticodeVerify;
  // Pkcs7GetSigners returning FALSE skips phase 2.
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillRepeatedly (Return (FALSE));

  // The single db anchor verifies the signature.
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _))
    .WillOnce (Return (TRUE));
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, UINT8 **OutTbs, UINTN *OutTbsSize) -> BOOLEAN {
    static UINT8  TbsBytes[] = { 0x11 };
    *OutTbs                  = TbsBytes;
    *OutTbsSize              = sizeof (TbsBytes);
    return TRUE;
  }
         )
       );

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  EXPECT_EQ (
    GetImageCertAuthority (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Authority
      ),
    EFI_SUCCESS
    );
  EXPECT_NE (Authority.Data, nullptr);
}

//
// A dbx X.509 anchor verifies the signature: IsCertRevoked returns TRUE so
// the cert is skipped before IsCertAuthorized is consulted.
//
TEST (GetImageCertAuthorityTest, RevokedByDbx_ReturnsNoAuthority) {
  MockBaseCryptLib    BaseCryptLibMock;
  DIGEST_CACHE        Cache;
  std::vector<UINT8>  CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  InitImageCache (Cache);

  // dbx with a single X.509 anchor that AuthenticodeVerify accepts.
  std::vector<UINT8>  Dbx;
  size_t              DbxOff = AppendSignatureList (
                                 Dbx,
                                 gEfiCertX509Guid,
                                 0,
                                 sizeof (EFI_GUID) + 16,
                                 1
                                 );

  SetEntryPayload (Dbx, DbxOff, 0, std::vector<UINT8>(16, 0xCC));

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
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _))
    .WillOnce (Return (TRUE));

  SIGNATURE_DATABASES  Databases = { NULL, 0, Dbx.data (), Dbx.size () };

  // Revocation by dbx -> EFI_ACCESS_DENIED. The hash algorithm was established
  // before revocation, so SignatureType carries it for the rejection record.
  EXPECT_EQ (
    GetImageCertAuthority (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Authority
      ),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Authority.Data, nullptr);
  EXPECT_TRUE (CompareGuid (&Authority.SignatureType, &gEfiCertSha256Guid));
}

//
// Prelude succeeds, dbx is empty (not revoked), but db has no trust anchor
// that verifies the signature, so IsCertAuthorized returns FALSE.
//
TEST (GetImageCertAuthorityTest, NotRevokedNotAuthorized_ReturnsNoAuthority) {
  MockBaseCryptLib    BaseCryptLibMock;
  DIGEST_CACHE        Cache;
  std::vector<UINT8>  CertBuf = MakePkcsSignedDataCert (std::vector<UINT8>(16, 0xA1));

  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  InitImageCache (Cache);

  std::vector<UINT8>  Db;
  size_t              DbOff = AppendSignatureList (
                                Db,
                                gEfiCertX509Guid,
                                0,
                                sizeof (EFI_GUID) + 16,
                                2
                                );

  SetEntryPayload (Db, DbOff, 0, std::vector<UINT8>(16, 0x11));
  SetEntryPayload (Db, DbOff, 1, std::vector<UINT8>(16, 0x22));

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
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillRepeatedly (Return (FALSE));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerify (_, _, _, _, _, _))
    .Times (2)
    .WillRepeatedly (Return (FALSE));

  SIGNATURE_DATABASES  Databases = { Db.data (), Db.size (), NULL, 0 };

  // Not revoked, but no db anchor authorizes -> EFI_NOT_FOUND.
  EXPECT_EQ (
    GetImageCertAuthority (
      (CONST WIN_CERTIFICATE *)CertBuf.data (),
      &Cache,
      &Databases,
      &Authority
      ),
    EFI_NOT_FOUND
    );
  EXPECT_EQ (Authority.Data, nullptr);
}
