/** @file
  Unit tests for the DxeImageVerificationLib Support helpers: GetHash,
  BuildImageAuthority, and FreeImageAuthority.
  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockBaseCryptLib.h>

#include <vector>
#include <cstring>
#include <functional>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseMemoryLib.h>
  #include "../Support.h"
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

// ---------------------------------------------------------------------------
// GetHash / FreeDigestCache
// ---------------------------------------------------------------------------

//
// A fixed non-zero buffer for the cache to hash. GetHash () only forwards these bytes to the mocked
// HashAllByGuid (), so their exact contents do not affect the assertions below.
//
static UINT8  mCacheBuffer[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

//
// A gmock action for the HashAllByGuid () mock: fill DigestSize bytes of the output digest with Fill
// and report success, standing in for a real hash of the bound buffer.
//
static std::function<EFI_STATUS (CONST EFI_GUID *, CONST VOID *, UINTN, UINT8 *, UINTN *)>
FillDigest (
  UINTN  DigestSize,
  UINT8  Fill
  )
{
  return [DigestSize, Fill](CONST EFI_GUID *HashType, CONST VOID *Buffer, UINTN BufferSize, UINT8 *Digest, UINTN *OutDigestSize) -> EFI_STATUS {
           (VOID)HashType;
           (VOID)Buffer;
           (VOID)BufferSize;
           SetMem (Digest, DigestSize, Fill);
           *OutDigestSize = DigestSize;
           return EFI_SUCCESS;
  };
}

TEST (GetHashTest, NullParameters_ReturnsInvalidParameter) {
  DIGEST_CACHE  Cache;
  CONST UINT8   *Digest    = NULL;
  UINTN         DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mCacheBuffer;
  Cache.BufferSize = sizeof (mCacheBuffer);

  EXPECT_EQ (GetHash (NULL, &Cache, &Digest, &DigestSize), EFI_INVALID_PARAMETER);
  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, NULL, &Digest, &DigestSize), EFI_INVALID_PARAMETER);
  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, NULL, &DigestSize), EFI_INVALID_PARAMETER);
  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest, NULL), EFI_INVALID_PARAMETER);
}

TEST (GetHashTest, CacheWithoutFileBuffer_ReturnsInvalidParameter) {
  DIGEST_CACHE  Cache;
  CONST UINT8   *Digest    = NULL;
  UINTN         DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));

  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest, &DigestSize), EFI_INVALID_PARAMETER);
}

TEST (GetHashTest, UnsupportedHashGuid_ReturnsUnsupported) {
  MockBaseCryptLib  BaseCryptLibMock;
  EFI_GUID          UnknownGuid = {
    0xA1B2C3D4,
    0x9999,
    0x8888,
    { 0x10,    0x20,0x30, 0x40, 0x50, 0x60, 0x70, 0x80 }
  };
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest    = NULL;
  UINTN             DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mCacheBuffer;
  Cache.BufferSize = sizeof (mCacheBuffer);

  //
  // GetHash forwards the algorithm GUID to HashAllByGuid (), which rejects an unrecognized algorithm.
  //
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Return (EFI_UNSUPPORTED));

  EXPECT_EQ (GetHash (&UnknownGuid, &Cache, &Digest, &DigestSize), EFI_UNSUPPORTED);
  EXPECT_EQ (Cache.Entries, nullptr);
}

TEST (GetHashTest, Sha256Miss_HashesBufferAndMemoizes) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest    = NULL;
  UINTN             DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mCacheBuffer;
  Cache.BufferSize = sizeof (mCacheBuffer);

  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Invoke (FillDigest (SHA256_DIGEST_SIZE, 0x5A)));

  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest, &DigestSize), EFI_SUCCESS);
  ASSERT_NE (Digest, (CONST UINT8 *)NULL);
  EXPECT_EQ (DigestSize, (UINTN)SHA256_DIGEST_SIZE);
  EXPECT_EQ (Digest[0], (UINT8)0x5A);
  EXPECT_EQ (Digest[SHA256_DIGEST_SIZE - 1], (UINT8)0x5A);
  EXPECT_NE (Cache.Entries, nullptr);

  FreeDigestCache (&Cache);
  EXPECT_EQ (Cache.Entries, nullptr);
}

TEST (GetHashTest, Sha256Hit_ReturnsMemoizedDigestWithoutRehashing) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest1    = NULL;
  CONST UINT8       *Digest2    = NULL;
  UINTN             DigestSize1 = 0;
  UINTN             DigestSize2 = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mCacheBuffer;
  Cache.BufferSize = sizeof (mCacheBuffer);

  //
  // WillOnce means a second HashAllByGuid () call would fail the test, proving the second GetHash ()
  // is served from the memoized entry rather than re-hashing the buffer.
  //
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Invoke (FillDigest (SHA256_DIGEST_SIZE, 0xC3)));

  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest1, &DigestSize1), EFI_SUCCESS);
  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest2, &DigestSize2), EFI_SUCCESS);
  EXPECT_EQ (Digest1, Digest2);
  EXPECT_EQ (DigestSize1, DigestSize2);

  FreeDigestCache (&Cache);
}

TEST (GetHashTest, HashFailure_PropagatesErrorAndLeavesCacheEmpty) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest    = NULL;
  UINTN             DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mCacheBuffer;
  Cache.BufferSize = sizeof (mCacheBuffer);

  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest, &DigestSize), EFI_DEVICE_ERROR);
  EXPECT_EQ (Cache.Entries, nullptr);
}

TEST (GetHashTest, DistinctAlgorithms_MemoizedSeparately) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest256    = NULL;
  CONST UINT8       *Digest384    = NULL;
  CONST UINT8       *Digest512    = NULL;
  UINTN             DigestSize256 = 0;
  UINTN             DigestSize384 = 0;
  UINTN             DigestSize512 = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mCacheBuffer;
  Cache.BufferSize = sizeof (mCacheBuffer);

  //
  // HashAllByGuid () is expected exactly three times - once per algorithm; the second round of
  // GetHash () calls below must be served from the three separate memoized entries. The action fills
  // a per-algorithm digest so each entry is distinguishable.
  //
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .Times (3)
    .WillRepeatedly (
       Invoke (
         [] (CONST EFI_GUID *HashType, CONST VOID *Buffer, UINTN BufferSize, UINT8 *Digest, UINTN *DigestSize) -> EFI_STATUS {
    (VOID)Buffer;
    (VOID)BufferSize;
    if (CompareGuid (HashType, &gEfiHashAlgorithmSha256Guid)) {
      SetMem (Digest, SHA256_DIGEST_SIZE, 0x11);
      *DigestSize = SHA256_DIGEST_SIZE;
    } else if (CompareGuid (HashType, &gEfiHashAlgorithmSha384Guid)) {
      SetMem (Digest, SHA384_DIGEST_SIZE, 0x22);
      *DigestSize = SHA384_DIGEST_SIZE;
    } else {
      SetMem (Digest, SHA512_DIGEST_SIZE, 0x33);
      *DigestSize = SHA512_DIGEST_SIZE;
    }

    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest256, &DigestSize256), EFI_SUCCESS);
  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha384Guid, &Cache, &Digest384, &DigestSize384), EFI_SUCCESS);
  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha512Guid, &Cache, &Digest512, &DigestSize512), EFI_SUCCESS);

  EXPECT_EQ (DigestSize256, (UINTN)SHA256_DIGEST_SIZE);
  EXPECT_EQ (DigestSize384, (UINTN)SHA384_DIGEST_SIZE);
  EXPECT_EQ (DigestSize512, (UINTN)SHA512_DIGEST_SIZE);
  EXPECT_EQ (Digest256[0], (UINT8)0x11);
  EXPECT_EQ (Digest384[0], (UINT8)0x22);
  EXPECT_EQ (Digest512[0], (UINT8)0x33);

  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest256, &DigestSize256), EFI_SUCCESS);
  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha384Guid, &Cache, &Digest384, &DigestSize384), EFI_SUCCESS);
  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha512Guid, &Cache, &Digest512, &DigestSize512), EFI_SUCCESS);

  FreeDigestCache (&Cache);
  EXPECT_EQ (Cache.Entries, nullptr);
}

TEST (GetHashTest, FreeDigestCache_ReleasesEntriesAndAllowsRecompute) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest    = NULL;
  UINTN             DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = mCacheBuffer;
  Cache.BufferSize = sizeof (mCacheBuffer);

  //
  // After FreeDigestCache () drops the memoized entry, a second GetHash () for the same algorithm is
  // a fresh miss and hashes the buffer again, so HashAllByGuid () is expected twice.
  //
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .Times (2)
    .WillRepeatedly (Invoke (FillDigest (SHA256_DIGEST_SIZE, 0x77)));

  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest, &DigestSize), EFI_SUCCESS);
  EXPECT_NE (Cache.Entries, nullptr);

  FreeDigestCache (&Cache);
  EXPECT_EQ (Cache.Entries, nullptr);

  EXPECT_EQ (GetHash (&gEfiHashAlgorithmSha256Guid, &Cache, &Digest, &DigestSize), EFI_SUCCESS);
  EXPECT_EQ (DigestSize, (UINTN)SHA256_DIGEST_SIZE);

  FreeDigestCache (&Cache);
}

TEST (GetHashTest, FreeDigestCache_NullAndEmpty_NoOp) {
  DIGEST_CACHE  Cache;

  ZeroMem (&Cache, sizeof (Cache));

  FreeDigestCache (NULL);
  FreeDigestCache (&Cache);
  EXPECT_EQ (Cache.Entries, nullptr);
}

// ---------------------------------------------------------------------------
// BuildImageAuthority / FreeImageAuthority
// ---------------------------------------------------------------------------

TEST (BuildImageAuthorityTest, NullPayload_InvalidParameter) {
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, NULL, 4, &Authority), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (BuildImageAuthorityTest, ZeroPayloadSize_InvalidParameter) {
  UINT8            Payload[4] = { 0 };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, Payload, 0, &Authority), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (BuildImageAuthorityTest, NullAuthority_InvalidParameter) {
  UINT8  Payload[4] = { 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), NULL), EFI_INVALID_PARAMETER);
}

//
// A payload large enough to overflow the EFI_SIGNATURE_DATA header addition is rejected before any
// allocation or read of the payload.
//
TEST (BuildImageAuthorityTest, OverflowingPayloadSize_InvalidParameter) {
  UINT8            Sentinel  = 0;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, &Sentinel, MAX_UINTN, &Authority), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

//
// With no owner, the SignatureOwner is zeroed and the payload follows it. Size is the payload plus
// a SignatureOwner GUID.
//
TEST (BuildImageAuthorityTest, NoOwner_ZeroesOwnerAndCopiesPayload) {
  UINT8            Payload[5] = { 0x11, 0x22, 0x33, 0x44, 0x55 };
  EFI_GUID         ZeroGuid   = { 0 };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), &Authority), EFI_SUCCESS);
  ASSERT_NE (Authority.Data, nullptr);
  EXPECT_EQ (Authority.Size, (UINTN)(sizeof (EFI_GUID) + sizeof (Payload)));
  EXPECT_TRUE (CompareGuid (&Authority.Data->SignatureOwner, &ZeroGuid));
  EXPECT_EQ (CompareMem (Authority.Data->SignatureData, Payload, sizeof (Payload)), 0);

  FreeImageAuthority (&Authority);
  EXPECT_EQ (Authority.Data, nullptr);
  EXPECT_EQ (Authority.Size, (UINTN)0);
}

//
// A supplied owner GUID is stored verbatim ahead of the payload.
//
TEST (BuildImageAuthorityTest, WithOwner_StoresOwnerAndPayload) {
  EFI_GUID         Owner = { 0x11223344, 0x5566, 0x7788, { 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00 }
  };
  UINT8            Payload[3] = { 0xDE, 0xAD, 0xBE };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (&Owner, Payload, sizeof (Payload), &Authority), EFI_SUCCESS);
  ASSERT_NE (Authority.Data, nullptr);
  EXPECT_TRUE (CompareGuid (&Authority.Data->SignatureOwner, &Owner));
  EXPECT_EQ (CompareMem (Authority.Data->SignatureData, Payload, sizeof (Payload)), 0);

  FreeImageAuthority (&Authority);
}

//
// FreeImageAuthority tolerates a NULL pointer and an already-empty authority.
//
TEST (FreeImageAuthorityTest, NullAndEmpty_NoOp) {
  IMAGE_AUTHORITY  Empty = { NULL, 0 };

  FreeImageAuthority (NULL);
  FreeImageAuthority (&Empty);
  EXPECT_EQ (Empty.Data, nullptr);
  EXPECT_EQ (Empty.Size, (UINTN)0);
}

//
// FreeImageAuthority clears the record it frees, so a second call is a safe no-op (no double free).
//
TEST (FreeImageAuthorityTest, DoubleFree_Safe) {
  UINT8            Payload[4] = { 1, 2, 3, 4 };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  ASSERT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), &Authority), EFI_SUCCESS);
  FreeImageAuthority (&Authority);
  FreeImageAuthority (&Authority);
  EXPECT_EQ (Authority.Data, nullptr);
}
