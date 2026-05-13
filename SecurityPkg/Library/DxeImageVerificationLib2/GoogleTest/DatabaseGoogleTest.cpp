/** @file
  Unit tests for the signature-database helpers in
  DxeImageVerificationLib (Database.c): IsKnownImageHashGuid,
  WalkSignatureDatabase, GetDatabaseHashAlgorithms,
  IsSignatureFoundInDatabase, and LoadSignatureDatabase.
  WalkSignatureDatabase, GetDatabaseHashAlgorithms, and
  IsSignatureFoundInDatabase are exercised against synthetic in-memory
  EFI_SIGNATURE_LIST buffers built by helpers in this file.
  LoadSignatureDatabase is exercised against a mocked GetVariable2.
  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockUefiLib.h>

#include <vector>
#include <cstring>

extern "C" {
  #include <Uefi.h>
  #include <Guid/ImageAuthentication.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include "../Database.h"
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

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
struct CallbackRecorder {
  UINTN                    Count;
  std::vector<EFI_GUID>    SeenTypes;
  RETURN_STATUS            ReturnAt;     // RETURN_SUCCESS to keep going
  UINTN                    AbortAfter;   // applies when ReturnAt != SUCCESS
};

extern "C" EFI_STATUS EFIAPI
RecorderCallback (
  IN CONST EFI_SIGNATURE_LIST  *List,
  IN VOID                      *Context  OPTIONAL
  )
{
  CallbackRecorder  *Rec = (CallbackRecorder *)Context;

  Rec->Count++;
  Rec->SeenTypes.push_back (List->SignatureType);

  if ((Rec->ReturnAt != EFI_SUCCESS) && (Rec->Count >= Rec->AbortAfter)) {
    return Rec->ReturnAt;
  }

  return EFI_SUCCESS;
}

// SHA-256 entry size: 16-byte owner GUID + 32-byte digest.
static constexpr UINT32  kSha256EntrySize = sizeof (EFI_GUID) + 32;
static constexpr UINT32  kSha384EntrySize = sizeof (EFI_GUID) + 48;

// ---------------------------------------------------------------------------
// IsKnownImageHashGuid
// ---------------------------------------------------------------------------

TEST (IsKnownImageHashGuidTest, NullGuid_ReturnsFalse) {
  EXPECT_FALSE (IsKnownImageHashGuid (NULL));
}

TEST (IsKnownImageHashGuidTest, KnownHashGuids_ReturnTrue) {
  EXPECT_TRUE (IsKnownImageHashGuid (&gEfiCertSha1Guid));
  EXPECT_TRUE (IsKnownImageHashGuid (&gEfiCertSha256Guid));
  EXPECT_TRUE (IsKnownImageHashGuid (&gEfiCertSha384Guid));
  EXPECT_TRUE (IsKnownImageHashGuid (&gEfiCertSha512Guid));
}

TEST (IsKnownImageHashGuidTest, X509Guids_ReturnFalse) {
  // X509-with-hash variants are intentionally excluded.
  EXPECT_FALSE (IsKnownImageHashGuid (&gEfiCertX509Guid));
  EXPECT_FALSE (IsKnownImageHashGuid (&gEfiCertX509Sha256Guid));
  EXPECT_FALSE (IsKnownImageHashGuid (&gEfiCertX509Sha384Guid));
  EXPECT_FALSE (IsKnownImageHashGuid (&gEfiCertX509Sha512Guid));
}

TEST (IsKnownImageHashGuidTest, ArbitraryGuid_ReturnsFalse) {
  EFI_GUID  Junk = { 0x12345678, 0x1234, 0x5678,
                     { 0x9a,     0xbc,   0xde,  0xf0,0x12, 0x34, 0x56, 0x78 }
  };

  EXPECT_FALSE (IsKnownImageHashGuid (&Junk));
}

// ---------------------------------------------------------------------------
// WalkSignatureDatabase
// ---------------------------------------------------------------------------

class WalkSignatureDatabaseTest : public ::testing::Test {
protected:
  CallbackRecorder Rec{ };

  void
  SetUp (
    ) override
  {
    Rec.Count      = 0;
    Rec.ReturnAt   = EFI_SUCCESS;
    Rec.AbortAfter = 0;
    Rec.SeenTypes.clear ();
  }
};

TEST_F (WalkSignatureDatabaseTest, NullBuffer_ReturnsInvalidParameter) {
  EXPECT_EQ (
    WalkSignatureDatabase (NULL, 0, RecorderCallback, &Rec),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (Rec.Count, 0u);
}

TEST_F (WalkSignatureDatabaseTest, NullCallback_ReturnsInvalidParameter) {
  UINT8  Dummy = 0;

  EXPECT_EQ (
    WalkSignatureDatabase (&Dummy, 1, NULL, &Rec),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (WalkSignatureDatabaseTest, EmptyBuffer_ReturnsSuccessNoInvocations) {
  UINT8  Dummy = 0;

  EXPECT_EQ (
    WalkSignatureDatabase (&Dummy, 0, RecorderCallback, &Rec),
    EFI_SUCCESS
    );
  EXPECT_EQ (Rec.Count, 0u);
}

TEST_F (WalkSignatureDatabaseTest, SingleList_InvokesCallbackOnce) {
  std::vector<UINT8>  Buf;

  AppendSignatureList (Buf, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);

  EXPECT_EQ (
    WalkSignatureDatabase (Buf.data (), Buf.size (), RecorderCallback, &Rec),
    EFI_SUCCESS
    );
  EXPECT_EQ (Rec.Count, 1u);
  ASSERT_EQ (Rec.SeenTypes.size (), 1u);
  EXPECT_EQ (CompareGuid (&Rec.SeenTypes[0], &gEfiCertSha256Guid), TRUE);
}

TEST_F (WalkSignatureDatabaseTest, MultipleLists_InvokedInOrder) {
  std::vector<UINT8>  Buf;

  AppendSignatureList (Buf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  AppendSignatureList (Buf, gEfiCertSha384Guid, 0, kSha384EntrySize, 3);
  AppendSignatureList (Buf, gEfiCertX509Guid, 0, sizeof (EFI_GUID) + 8, 1);

  EXPECT_EQ (
    WalkSignatureDatabase (Buf.data (), Buf.size (), RecorderCallback, &Rec),
    EFI_SUCCESS
    );
  ASSERT_EQ (Rec.SeenTypes.size (), 3u);
  EXPECT_EQ (CompareGuid (&Rec.SeenTypes[0], &gEfiCertSha256Guid), TRUE);
  EXPECT_EQ (CompareGuid (&Rec.SeenTypes[1], &gEfiCertSha384Guid), TRUE);
  EXPECT_EQ (CompareGuid (&Rec.SeenTypes[2], &gEfiCertX509Guid), TRUE);
}

TEST_F (WalkSignatureDatabaseTest, ListSizeBelowHeader_ReturnsCorrupted) {
  std::vector<UINT8>  Buf (sizeof (EFI_SIGNATURE_LIST), 0);
  EFI_SIGNATURE_LIST  *List = (EFI_SIGNATURE_LIST *)Buf.data ();

  CopyGuid (&List->SignatureType, &gEfiCertSha256Guid);
  List->SignatureListSize   = sizeof (EFI_SIGNATURE_LIST) - 1;  // bad
  List->SignatureHeaderSize = 0;
  List->SignatureSize       = kSha256EntrySize;

  EXPECT_EQ (
    WalkSignatureDatabase (Buf.data (), Buf.size (), RecorderCallback, &Rec),
    EFI_VOLUME_CORRUPTED
    );
  EXPECT_EQ (Rec.Count, 0u);
}

TEST_F (WalkSignatureDatabaseTest, ListSizeExceedsBuffer_ReturnsCorrupted) {
  std::vector<UINT8>  Buf;

  AppendSignatureList (Buf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  // Inflate the declared size past Buf.size().
  ((EFI_SIGNATURE_LIST *)Buf.data ())->SignatureListSize = (UINT32)(Buf.size () + 1);

  EXPECT_EQ (
    WalkSignatureDatabase (Buf.data (), Buf.size (), RecorderCallback, &Rec),
    EFI_VOLUME_CORRUPTED
    );
  EXPECT_EQ (Rec.Count, 0u);
}

TEST_F (WalkSignatureDatabaseTest, SignatureSizeBelowGuidSize_ReturnsCorrupted) {
  std::vector<UINT8>  Buf;

  AppendSignatureList (Buf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  ((EFI_SIGNATURE_LIST *)Buf.data ())->SignatureSize = sizeof (EFI_GUID) - 1;

  EXPECT_EQ (
    WalkSignatureDatabase (Buf.data (), Buf.size (), RecorderCallback, &Rec),
    EFI_VOLUME_CORRUPTED
    );
}

TEST_F (WalkSignatureDatabaseTest, HeaderSizeOverflowsList_ReturnsCorrupted) {
  std::vector<UINT8>  Buf;

  AppendSignatureList (Buf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  // SignatureHeaderSize larger than the per-list payload region.
  ((EFI_SIGNATURE_LIST *)Buf.data ())->SignatureHeaderSize =
    ((EFI_SIGNATURE_LIST *)Buf.data ())->SignatureListSize;

  EXPECT_EQ (
    WalkSignatureDatabase (Buf.data (), Buf.size (), RecorderCallback, &Rec),
    EFI_VOLUME_CORRUPTED
    );
}

TEST_F (WalkSignatureDatabaseTest, PayloadNotMultipleOfEntry_ReturnsCorrupted) {
  std::vector<UINT8>  Buf;

  AppendSignatureList (Buf, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);

  // Bump the entry size so payload no longer divides evenly.
  ((EFI_SIGNATURE_LIST *)Buf.data ())->SignatureSize = kSha256EntrySize + 1;

  EXPECT_EQ (
    WalkSignatureDatabase (Buf.data (), Buf.size (), RecorderCallback, &Rec),
    EFI_VOLUME_CORRUPTED
    );
}

TEST_F (WalkSignatureDatabaseTest, TrailingBytes_ReturnsCorrupted) {
  std::vector<UINT8>  Buf;

  AppendSignatureList (Buf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  // Append a partial header that's smaller than EFI_SIGNATURE_LIST.
  Buf.push_back (0xAA);

  EXPECT_EQ (
    WalkSignatureDatabase (Buf.data (), Buf.size (), RecorderCallback, &Rec),
    EFI_VOLUME_CORRUPTED
    );
  EXPECT_EQ (Rec.Count, 1u);  // first list was visited before trailing-byte check
}

TEST_F (WalkSignatureDatabaseTest, CallbackAborts_PropagatesStatus) {
  std::vector<UINT8>  Buf;

  AppendSignatureList (Buf, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  AppendSignatureList (Buf, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);
  AppendSignatureList (Buf, gEfiCertSha512Guid, 0, sizeof (EFI_GUID) + 64, 1);

  Rec.ReturnAt   = EFI_ABORTED;
  Rec.AbortAfter = 2;

  EXPECT_EQ (
    WalkSignatureDatabase (Buf.data (), Buf.size (), RecorderCallback, &Rec),
    EFI_ABORTED
    );
  EXPECT_EQ (Rec.Count, 2u);  // third list never visited
}

// ---------------------------------------------------------------------------
// GetDatabaseHashAlgorithms
// ---------------------------------------------------------------------------

TEST (GetDatabaseHashAlgorithmsTest, NullArg_ReturnsInvalidParameter) {
  EXPECT_EQ (
    GetDatabaseHashAlgorithms (NULL, 0, NULL, 0, NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST (GetDatabaseHashAlgorithmsTest, BothBuffersNull_EmptySet) {
  HASH_ALGORITHM_SET  Set;

  EXPECT_EQ (
    GetDatabaseHashAlgorithms (NULL, 0, NULL, 0, &Set),
    EFI_SUCCESS
    );
  EXPECT_EQ (Set.Count, 0u);
}

TEST (GetDatabaseHashAlgorithmsTest, OnlyDb_HashType_Reported) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  HASH_ALGORITHM_SET  Set;

  EXPECT_EQ (
    GetDatabaseHashAlgorithms (Db.data (), Db.size (), NULL, 0, &Set),
    EFI_SUCCESS
    );
  ASSERT_EQ (Set.Count, 1u);
  EXPECT_EQ (CompareGuid (&Set.Guids[0], &gEfiCertSha256Guid), TRUE);
}

TEST (GetDatabaseHashAlgorithmsTest, OnlyDbx_HashType_Reported) {
  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha384Guid, 0, kSha384EntrySize, 2);

  HASH_ALGORITHM_SET  Set;

  EXPECT_EQ (
    GetDatabaseHashAlgorithms (NULL, 0, Dbx.data (), Dbx.size (), &Set),
    EFI_SUCCESS
    );
  ASSERT_EQ (Set.Count, 1u);
  EXPECT_EQ (CompareGuid (&Set.Guids[0], &gEfiCertSha384Guid), TRUE);
}

TEST (GetDatabaseHashAlgorithmsTest, BothPresent_Union) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);

  HASH_ALGORITHM_SET  Set;

  EXPECT_EQ (
    GetDatabaseHashAlgorithms (Db.data (), Db.size (), Dbx.data (), Dbx.size (), &Set),
    EFI_SUCCESS
    );
  EXPECT_EQ (Set.Count, 2u);
}

TEST (GetDatabaseHashAlgorithmsTest, DuplicateAcrossDbAndDbx_Deduplicated) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  HASH_ALGORITHM_SET  Set;

  EXPECT_EQ (
    GetDatabaseHashAlgorithms (Db.data (), Db.size (), Dbx.data (), Dbx.size (), &Set),
    EFI_SUCCESS
    );
  ASSERT_EQ (Set.Count, 1u);
  EXPECT_EQ (CompareGuid (&Set.Guids[0], &gEfiCertSha256Guid), TRUE);
}

TEST (GetDatabaseHashAlgorithmsTest, NonHashSignatureTypes_Ignored) {
  std::vector<UINT8>  Db;

  // X509 cert list is not a plain image hash; should not contribute.
  AppendSignatureList (Db, gEfiCertX509Guid, 0, sizeof (EFI_GUID) + 16, 1);

  HASH_ALGORITHM_SET  Set;

  EXPECT_EQ (
    GetDatabaseHashAlgorithms (Db.data (), Db.size (), NULL, 0, &Set),
    EFI_SUCCESS
    );
  EXPECT_EQ (Set.Count, 0u);
}

TEST (GetDatabaseHashAlgorithmsTest, MalformedDb_ReturnsCorrupted) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  // Corrupt the list size.
  ((EFI_SIGNATURE_LIST *)Db.data ())->SignatureListSize = (UINT32)(Db.size () + 1);

  HASH_ALGORITHM_SET  Set;

  EXPECT_EQ (
    GetDatabaseHashAlgorithms (Db.data (), Db.size (), NULL, 0, &Set),
    EFI_VOLUME_CORRUPTED
    );
}

TEST (GetDatabaseHashAlgorithmsTest, MalformedDbx_ReturnsCorrupted) {
  // Db is well-formed; Dbx is corrupt. The walk over Dbx must surface
  // the error rather than silently returning the partial Db result.
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  std::vector<UINT8>  Dbx;

  AppendSignatureList (Dbx, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);
  ((EFI_SIGNATURE_LIST *)Dbx.data ())->SignatureListSize = (UINT32)(Dbx.size () + 1);

  HASH_ALGORITHM_SET  Set;

  EXPECT_EQ (
    GetDatabaseHashAlgorithms (Db.data (), Db.size (), Dbx.data (), Dbx.size (), &Set),
    EFI_VOLUME_CORRUPTED
    );
}

TEST (GetDatabaseHashAlgorithmsTest, MultipleAlgorithmsInSingleBuffer_AllReported) {
  // All four known image-hash algorithms concatenated into a single
  // buffer must all be reported.
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha1Guid, 0, sizeof (EFI_GUID) + 20, 1);
  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  AppendSignatureList (Db, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);
  AppendSignatureList (Db, gEfiCertSha512Guid, 0, sizeof (EFI_GUID) + 64, 1);

  HASH_ALGORITHM_SET  Set;

  EXPECT_EQ (
    GetDatabaseHashAlgorithms (Db.data (), Db.size (), NULL, 0, &Set),
    EFI_SUCCESS
    );
  ASSERT_EQ (Set.Count, 4u);
  EXPECT_EQ (CompareGuid (&Set.Guids[0], &gEfiCertSha1Guid), TRUE);
  EXPECT_EQ (CompareGuid (&Set.Guids[1], &gEfiCertSha256Guid), TRUE);
  EXPECT_EQ (CompareGuid (&Set.Guids[2], &gEfiCertSha384Guid), TRUE);
  EXPECT_EQ (CompareGuid (&Set.Guids[3], &gEfiCertSha512Guid), TRUE);
}

// ---------------------------------------------------------------------------
// IsSignatureFoundInDatabase
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

TEST (IsSignatureFoundInDatabaseTest, NullSignature_ReturnsInvalidParameter) {
  BOOLEAN  Found = FALSE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (NULL, 0, NULL, &gEfiCertSha256Guid, kSha256DigestSize, &Found),
    EFI_INVALID_PARAMETER
    );
}

TEST (IsSignatureFoundInDatabaseTest, NullSignatureType_ReturnsInvalidParameter) {
  UINT8    Sig[kSha256DigestSize] = { 0 };
  BOOLEAN  Found                  = FALSE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (NULL, 0, Sig, NULL, sizeof (Sig), &Found),
    EFI_INVALID_PARAMETER
    );
}

TEST (IsSignatureFoundInDatabaseTest, NullIsFound_ReturnsInvalidParameter) {
  UINT8  Sig[kSha256DigestSize] = { 0 };

  EXPECT_EQ (
    IsSignatureFoundInDatabase (NULL, 0, Sig, &gEfiCertSha256Guid, sizeof (Sig), NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST (IsSignatureFoundInDatabaseTest, ZeroSignatureSize_ReturnsInvalidParameter) {
  UINT8    Sig   = 0;
  BOOLEAN  Found = FALSE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (NULL, 0, &Sig, &gEfiCertSha256Guid, 0, &Found),
    EFI_INVALID_PARAMETER
    );
}

TEST (IsSignatureFoundInDatabaseTest, NullDatabase_NotFoundSuccess) {
  UINT8    Sig[kSha256DigestSize] = { 0xAB };
  BOOLEAN  Found                  = TRUE;  // pre-set to verify it gets cleared

  EXPECT_EQ (
    IsSignatureFoundInDatabase (NULL, 0, Sig, &gEfiCertSha256Guid, sizeof (Sig), &Found),
    EFI_SUCCESS
    );
  EXPECT_FALSE (Found);
}

TEST (IsSignatureFoundInDatabaseTest, ExactMatch_Found) {
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);

  std::vector<UINT8>  Target (kSha256DigestSize, 0xAA);

  SetEntryPayload (Db, Off, 1, Target);

  BOOLEAN  Found = FALSE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (
      Db.data (),
      Db.size (),
      Target.data (),
      &gEfiCertSha256Guid,
      Target.size (),
      &Found
      ),
    EFI_SUCCESS
    );
  EXPECT_TRUE (Found);
}

TEST (IsSignatureFoundInDatabaseTest, NoMatchingEntry_NotFound) {
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  std::vector<UINT8>  Stored (kSha256DigestSize, 0xAA);

  SetEntryPayload (Db, Off, 0, Stored);

  std::vector<UINT8>  Wanted (kSha256DigestSize, 0xBB);
  BOOLEAN             Found = FALSE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (
      Db.data (),
      Db.size (),
      Wanted.data (),
      &gEfiCertSha256Guid,
      Wanted.size (),
      &Found
      ),
    EFI_SUCCESS
    );
  EXPECT_FALSE (Found);
}

TEST (IsSignatureFoundInDatabaseTest, MismatchedSignatureType_Skipped) {
  // Database contains a SHA-384 entry whose payload bytes happen to
  // match the search target; lookup with a SHA-256 type GUID must skip
  // the SHA-384 list and report not-found.
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);

  std::vector<UINT8>  Stored (kSha384DigestSize, 0xAA);

  SetEntryPayload (Db, Off, 0, Stored);

  std::vector<UINT8>  Wanted (kSha256DigestSize, 0xAA);
  BOOLEAN             Found = TRUE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (
      Db.data (),
      Db.size (),
      Wanted.data (),
      &gEfiCertSha256Guid,
      Wanted.size (),
      &Found
      ),
    EFI_SUCCESS
    );
  EXPECT_FALSE (Found);
}

TEST (IsSignatureFoundInDatabaseTest, MismatchedSignatureSize_Skipped) {
  // List type matches but per-entry size doesn't, so the list describes
  // a different algorithm and must be skipped.
  std::vector<UINT8>  Db;
  size_t              Off = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha384EntrySize, 1);

  std::vector<UINT8>  Stored (kSha384DigestSize, 0xCC);

  SetEntryPayload (Db, Off, 0, Stored);

  std::vector<UINT8>  Wanted (kSha256DigestSize, 0xCC);
  BOOLEAN             Found = TRUE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (
      Db.data (),
      Db.size (),
      Wanted.data (),
      &gEfiCertSha256Guid,
      Wanted.size (),
      &Found
      ),
    EFI_SUCCESS
    );
  EXPECT_FALSE (Found);
}

TEST (IsSignatureFoundInDatabaseTest, MatchInSecondList_Found) {
  // First list is the wrong algorithm, second list contains the target.
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha384Guid, 0, kSha384EntrySize, 1);
  size_t  SecondOff = AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);

  std::vector<UINT8>  Target (kSha256DigestSize, 0x77);

  SetEntryPayload (Db, SecondOff, 1, Target);

  BOOLEAN  Found = FALSE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (
      Db.data (),
      Db.size (),
      Target.data (),
      &gEfiCertSha256Guid,
      Target.size (),
      &Found
      ),
    EFI_SUCCESS
    );
  EXPECT_TRUE (Found);
}

TEST (IsSignatureFoundInDatabaseTest, NonZeroSignatureHeaderSize_EntryMathCorrect) {
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

  BOOLEAN  Found = FALSE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (
      Db.data (),
      Db.size (),
      Target.data (),
      &gEfiCertSha256Guid,
      Target.size (),
      &Found
      ),
    EFI_SUCCESS
    );
  EXPECT_TRUE (Found);
}

TEST (IsSignatureFoundInDatabaseTest, ZeroEntryList_NotFound) {
  // A well-formed list with zero entries must be skipped without a
  // false positive (EntryCount == 0 means the inner loop never runs).
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 0);

  UINT8    Sig[kSha256DigestSize] = { 0 };
  BOOLEAN  Found                  = TRUE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (
      Db.data (),
      Db.size (),
      Sig,
      &gEfiCertSha256Guid,
      sizeof (Sig),
      &Found
      ),
    EFI_SUCCESS
    );
  EXPECT_FALSE (Found);
}

TEST (IsSignatureFoundInDatabaseTest, MalformedDb_ReturnsCorrupted) {
  std::vector<UINT8>  Db;

  AppendSignatureList (Db, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  ((EFI_SIGNATURE_LIST *)Db.data ())->SignatureListSize = (UINT32)(Db.size () + 1);

  UINT8    Sig[kSha256DigestSize] = { 0 };
  BOOLEAN  Found                  = FALSE;

  EXPECT_EQ (
    IsSignatureFoundInDatabase (
      Db.data (),
      Db.size (),
      Sig,
      &gEfiCertSha256Guid,
      sizeof (Sig),
      &Found
      ),
    EFI_VOLUME_CORRUPTED
    );
}

// ---------------------------------------------------------------------------
// LoadSignatureDatabase (uses MockUefiLib::GetVariable2)
// ---------------------------------------------------------------------------

class LoadSignatureDatabaseTest : public ::testing::Test {
protected:
  MockUefiLib UefiLibMock;
};

TEST_F (LoadSignatureDatabaseTest, NullDatabaseName_ReturnsInvalidParameter) {
  VOID   *Buffer = NULL;
  UINTN  Size    = 0;

  EXPECT_EQ (
    LoadSignatureDatabase (NULL, &Buffer, &Size),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (LoadSignatureDatabaseTest, NullBuffer_ReturnsInvalidParameter) {
  UINTN  Size = 0;

  EXPECT_EQ (
    LoadSignatureDatabase ((const CHAR16 *)u"db", NULL, &Size),
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

  VOID   *Buffer = (VOID *)(UINTN)0xDEADBEEF;  // pre-set: must be cleared
  UINTN  Size    = 0xAA;

  EXPECT_EQ (
    LoadSignatureDatabase ((const CHAR16 *)u"db", &Buffer, &Size),
    EFI_SUCCESS
    );
  EXPECT_EQ (Buffer, (VOID *)NULL);
  EXPECT_EQ (Size, 0u);
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
             OUT      UINTN     *Size
         ) -> EFI_STATUS {
    (VOID)Name;
    (VOID)Guid;
    *Value = AllocateCopyPool (sizeof (kPayload), kPayload);
    *Size  = sizeof (kPayload);
    return EFI_SUCCESS;
  }
         )
       );

  VOID   *Buffer = NULL;
  UINTN  Size    = 0;

  EXPECT_EQ (
    LoadSignatureDatabase ((const CHAR16 *)u"db", &Buffer, &Size),
    EFI_SUCCESS
    );
  ASSERT_NE (Buffer, (VOID *)NULL);
  EXPECT_EQ (Size, sizeof (kPayload));
  EXPECT_EQ (CompareMem (Buffer, kPayload, sizeof (kPayload)), 0);

  FreePool (Buffer);
}

TEST_F (LoadSignatureDatabaseTest, GetVariableUnexpectedError_PropagatedVerbatim) {
  // Errors other than EFI_NOT_FOUND must be reported unchanged.
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  VOID   *Buffer = NULL;
  UINTN  Size    = 0;

  EXPECT_EQ (
    LoadSignatureDatabase ((const CHAR16 *)u"db", &Buffer, &Size),
    EFI_DEVICE_ERROR
    );
}
