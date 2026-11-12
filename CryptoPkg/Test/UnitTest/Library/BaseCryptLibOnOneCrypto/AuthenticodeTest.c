/** @file
  OneCrypto Authenticode service forwarding tests.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "../BaseCryptLib/TestBaseCryptLib.h"
#include <Protocol/OneCrypto.h>

extern ONE_CRYPTO_PROTOCOL  *gCryptoProtocol;

STATIC_ASSERT (
  OFFSET_OF (ONE_CRYPTO_PROTOCOL, GetTrustAnchorX509FromAuthData) ==
  OFFSET_OF (ONE_CRYPTO_PROTOCOL, GetCryptoOpCapability) +
  sizeof (ONE_CRYPTO_GET_CRYPTO_OP_CAPABILITY),
  "Authenticode services must preserve the capability service offset"
  );
STATIC_ASSERT (
  OFFSET_OF (ONE_CRYPTO_PROTOCOL, HashAllByGuid) ==
  OFFSET_OF (ONE_CRYPTO_PROTOCOL, GetTrustAnchorX509FromAuthData) +
  sizeof (ONE_CRYPTO_GET_TRUST_ANCHOR_X509_FROM_AUTH_DATA) +
  sizeof (ONE_CRYPTO_FREE_TRUST_ANCHOR_X509_CACHE) +
  sizeof (ONE_CRYPTO_AUTHENTICODE_VERIFY_EX) +
  sizeof (ONE_CRYPTO_GET_AUTHENTICODE_HASH_ALGORITHM),
  "Authenticode services must retain their published order"
  );

STATIC CONST UINT8  mData[]   = { 0x30, 0x00 };
STATIC EFI_GUID     mHashGuid = {
  0x12345678, 0, 0, { 0 }
};
STATIC EFI_STATUS   mStatus;
STATIC UINTN        mCalls;
STATIC BOOLEAN      mArgumentsMatch;
STATIC UINT8        mCache;

STATIC
EFI_STATUS
EFIAPI
FindAnchor (
  IN OUT VOID     **CacheHandle OPTIONAL,
  IN CONST UINT8  *TbsCertHash,
  IN UINTN        TbsCertHashSize,
  IN CONST UINT8  *AuthData,
  IN UINTN        AuthDataSize,
  OUT UINT8       **TrustAnchorX509,
  OUT UINTN       *TrustAnchorX509Size
  )
{
  mCalls++;
  mArgumentsMatch = mArgumentsMatch && (TbsCertHash == mData) &&
                    (TbsCertHashSize == sizeof (mData)) &&
                    (AuthData == mData) && (AuthDataSize == sizeof (mData));
  if (CacheHandle != NULL) {
    *CacheHandle = &mCache;
  }

  *TrustAnchorX509     = NULL;
  *TrustAnchorX509Size = 0;
  if (!EFI_ERROR (mStatus)) {
    *TrustAnchorX509 = AllocateCopyPool (sizeof (mData), mData);
    if (*TrustAnchorX509 == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *TrustAnchorX509Size = sizeof (mData);
  }

  return mStatus;
}

STATIC
VOID
EFIAPI
FreeAnchorCache (
  IN VOID  *CacheHandle OPTIONAL
  )
{
  mCalls++;
  mArgumentsMatch = mArgumentsMatch && (CacheHandle == &mCache);
}

STATIC
EFI_STATUS
EFIAPI
VerifyImage (
  IN CONST UINT8  *AuthData,
  IN UINTN        DataSize,
  IN CONST UINT8  *TrustedCert,
  IN UINTN        CertSize,
  IN CONST UINT8  *ImageHash,
  IN UINTN        HashSize,
  OUT UINT8       **CertChain OPTIONAL,
  OUT UINTN       *CertChainSize OPTIONAL
  )
{
  mCalls++;
  mArgumentsMatch = mArgumentsMatch && (AuthData == mData) &&
                    (DataSize == sizeof (mData)) && (TrustedCert == mData) &&
                    (CertSize == sizeof (mData)) && (ImageHash == mData) &&
                    (HashSize == sizeof (mData));
  if (CertChain != NULL) {
    *CertChain = NULL;
  }

  if (CertChainSize != NULL) {
    *CertChainSize = 0;
  }

  if (!EFI_ERROR (mStatus) && (CertChain != NULL) && (CertChainSize != NULL)) {
    *CertChain = AllocateCopyPool (sizeof (mData), mData);
    if (*CertChain == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *CertChainSize = sizeof (mData);
  }

  return mStatus;
}

STATIC
EFI_STATUS
EFIAPI
FindHashAlgorithm (
  IN CONST UINT8  *AuthData,
  IN UINTN        AuthDataSize,
  OUT EFI_GUID    *HashType
  )
{
  mCalls++;
  mArgumentsMatch = mArgumentsMatch && (AuthData == mData) && (AuthDataSize == sizeof (mData));
  if (!EFI_ERROR (mStatus)) {
    CopyGuid (HashType, &mHashGuid);
  }

  return mStatus;
}

STATIC
EFI_STATUS
EFIAPI
HashBuffer (
  IN CONST EFI_GUID  *HashType,
  IN CONST VOID      *Buffer,
  IN UINTN           BufferSize,
  OUT UINT8          *Digest,
  OUT UINTN          *DigestSize
  )
{
  mCalls++;
  mArgumentsMatch = mArgumentsMatch && (HashType == &mHashGuid) &&
                    (Buffer == mData) && (BufferSize == sizeof (mData));
  if (!EFI_ERROR (mStatus)) {
    CopyMem (Digest, mData, sizeof (mData));
    *DigestSize = sizeof (mData);
  }

  return mStatus;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestUnavailableAuthenticodeServices (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST UINT16  Versions[][2] = {
    { 1, 0 }, { 2, 0 }, { 0, 0 }
  };
  ONE_CRYPTO_PROTOCOL  Protocol;
  EFI_GUID             HashGuid;
  UINT8                Digest[SHA512_DIGEST_SIZE];
  UINT8                *Cert;
  UINTN                CertSize;
  UINTN                DigestSize;
  UINTN                Index;

  for (Index = 0; Index < ARRAY_SIZE (Versions) + 2; Index++) {
    if (Index < ARRAY_SIZE (Versions)) {
      gCryptoProtocol = AllocateZeroPool (OFFSET_OF (ONE_CRYPTO_PROTOCOL, GetCryptoOpCapability));
      UT_ASSERT_NOT_NULL (gCryptoProtocol);
      gCryptoProtocol->Major = Versions[Index][0];
      gCryptoProtocol->Minor = Versions[Index][1];
    } else if (Index == ARRAY_SIZE (Versions)) {
      gCryptoProtocol = NULL;
    } else {
      ZeroMem (&Protocol, sizeof (Protocol));
      Protocol.Major  = 1;
      Protocol.Minor  = 1;
      gCryptoProtocol = &Protocol;
    }

    UT_ASSERT_STATUS_EQUAL (GetTrustAnchorX509FromAuthData (NULL, mData, sizeof (mData), mData, sizeof (mData), &Cert, &CertSize), EFI_UNSUPPORTED);
    UT_ASSERT_STATUS_EQUAL (AuthenticodeVerifyEx (mData, sizeof (mData), mData, sizeof (mData), mData, sizeof (mData), &Cert, &CertSize), EFI_UNSUPPORTED);
    UT_ASSERT_STATUS_EQUAL (GetAuthenticodeHashAlgorithm (mData, sizeof (mData), &HashGuid), EFI_UNSUPPORTED);
    UT_ASSERT_STATUS_EQUAL (HashAllByGuid (&mHashGuid, mData, sizeof (mData), Digest, &DigestSize), EFI_UNSUPPORTED);
    FreeTrustAnchorX509Cache (&mCache);
    FreeTrustAnchorX509Cache (NULL);
    if (Index < ARRAY_SIZE (Versions)) {
      FreePool (gCryptoProtocol);
    }

    gCryptoProtocol = NULL;
  }

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestAuthenticodeForwarding (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST UINT16      Minors[]   = { 1, 2, MAX_UINT16 };
  STATIC CONST EFI_STATUS  Statuses[] = {
    EFI_SUCCESS,            EFI_NOT_FOUND, EFI_UNSUPPORTED, EFI_INVALID_PARAMETER,
    EFI_SECURITY_VIOLATION, EFI_OUT_OF_RESOURCES
  };
  ONE_CRYPTO_PROTOCOL      Protocol;
  EFI_GUID                 HashGuid;
  UINT8                    Digest[SHA512_DIGEST_SIZE];
  UINT8                    *Cert;
  UINTN                    CertSize;
  UINTN                    DigestSize;
  UINTN                    MinorIndex;
  UINTN                    StatusIndex;
  VOID                     *Cache;

  ZeroMem (&Protocol, sizeof (Protocol));
  Protocol.Major                          = 1;
  Protocol.GetTrustAnchorX509FromAuthData = FindAnchor;
  Protocol.FreeTrustAnchorX509Cache       = FreeAnchorCache;
  Protocol.AuthenticodeVerifyEx           = VerifyImage;
  Protocol.GetAuthenticodeHashAlgorithm   = FindHashAlgorithm;
  Protocol.HashAllByGuid                  = HashBuffer;

  for (MinorIndex = 0; MinorIndex < ARRAY_SIZE (Minors); MinorIndex++) {
    Protocol.Minor = Minors[MinorIndex];
    for (StatusIndex = 0; StatusIndex < ARRAY_SIZE (Statuses); StatusIndex++) {
      gCryptoProtocol = &Protocol;
      mStatus         = Statuses[StatusIndex];
      mCalls          = 0;
      mArgumentsMatch = TRUE;
      Cache           = NULL;
      UT_ASSERT_STATUS_EQUAL (GetTrustAnchorX509FromAuthData (&Cache, mData, sizeof (mData), mData, sizeof (mData), &Cert, &CertSize), mStatus);
      UT_ASSERT_TRUE (Cache == &mCache);
      if (!EFI_ERROR (mStatus)) {
        UT_ASSERT_EQUAL (CertSize, sizeof (mData));
        UT_ASSERT_MEM_EQUAL (Cert, mData, CertSize);
        FreePool (Cert);
      }

      UT_ASSERT_STATUS_EQUAL (AuthenticodeVerifyEx (mData, sizeof (mData), mData, sizeof (mData), mData, sizeof (mData), &Cert, &CertSize), mStatus);
      if (!EFI_ERROR (mStatus)) {
        UT_ASSERT_EQUAL (CertSize, sizeof (mData));
        UT_ASSERT_MEM_EQUAL (Cert, mData, CertSize);
        FreePool (Cert);
      }

      UT_ASSERT_STATUS_EQUAL (AuthenticodeVerifyEx (mData, sizeof (mData), mData, sizeof (mData), mData, sizeof (mData), NULL, NULL), mStatus);
      UT_ASSERT_STATUS_EQUAL (GetAuthenticodeHashAlgorithm (mData, sizeof (mData), &HashGuid), mStatus);
      UT_ASSERT_STATUS_EQUAL (HashAllByGuid (&mHashGuid, mData, sizeof (mData), Digest, &DigestSize), mStatus);
      if (!EFI_ERROR (mStatus)) {
        UT_ASSERT_TRUE (CompareGuid (&HashGuid, &mHashGuid));
        UT_ASSERT_EQUAL (DigestSize, sizeof (mData));
        UT_ASSERT_MEM_EQUAL (Digest, mData, DigestSize);
      }

      FreeTrustAnchorX509Cache (Cache);
      FreeTrustAnchorX509Cache (NULL);
      gCryptoProtocol = NULL;
      UT_ASSERT_EQUAL (mCalls, 6);
      UT_ASSERT_TRUE (mArgumentsMatch);
    }
  }

  return UNIT_TEST_PASSED;
}

EFI_STATUS
AddAuthenticodeForwardingTests (
  IN UNIT_TEST_SUITE_HANDLE  Suite
  )
{
  EFI_STATUS  Status;

  Status = AddTestCase (Suite, "Missing and incompatible Authenticode services", "CryptoPkg.OneCrypto.Authenticode", TestUnavailableAuthenticodeServices, NULL, NULL, NULL);
  if (!EFI_ERROR (Status)) {
    Status = AddTestCase (Suite, "Authenticode arguments, results, errors, and later minor versions", "CryptoPkg.OneCrypto.Authenticode", TestAuthenticodeForwarding, NULL, NULL, NULL);
  }

  return Status;
}
