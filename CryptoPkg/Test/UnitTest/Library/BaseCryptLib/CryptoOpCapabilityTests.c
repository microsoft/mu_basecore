/** @file
  Public crypto-operation capability contract tests.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"
#include <Guid/CryptoOpId.h>

STATIC CONST EFI_GUID  *mOperations[] = {
  &gCryptoOpCmsVerifyGuid,
  &gCryptoOpCmsContentDigestGuid,
  &gCryptoOpAuthenticodeVerifyGuid,
  &gCryptoOpAuthenticodeHashGuid
};

UNIT_TEST_STATUS
EFIAPI
TestCryptoOpCapabilityArrays (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;
  UINTN                     OpIndex;
  UINTN                     Index;
  UINTN                     Other;
  EFI_STATUS                Status;
  BOOLEAN                   Supported;

  Supported = FALSE;
  for (OpIndex = 0; OpIndex < ARRAY_SIZE (mOperations); OpIndex++) {
    Capabilities    = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
    CapabilityCount = MAX_UINTN;
    Status          = GetCryptoOpCapability (mOperations[OpIndex], &Capabilities, &CapabilityCount);
    if ((Status == EFI_UNSUPPORTED) || (Status == EFI_NOT_FOUND)) {
      UT_ASSERT_TRUE (Capabilities == NULL);
      UT_ASSERT_EQUAL (CapabilityCount, 0);
      continue;
    }

    UT_ASSERT_NOT_EFI_ERROR (Status);
    UT_ASSERT_TRUE ((CapabilityCount == 0) == (Capabilities == NULL));
    Supported = TRUE;
    for (Index = 0; Index < CapabilityCount; Index++) {
      UT_ASSERT_TRUE (Capabilities[Index].AlgorithmOid != NULL);
      UT_ASSERT_TRUE (Capabilities[Index].AlgorithmOidSize > 1);
      UT_ASSERT_EQUAL (
        Capabilities[Index].AlgorithmOidSize,
        AsciiStrSize (Capabilities[Index].AlgorithmOid)
        );
      for (Other = 0; Other < Index; Other++) {
        UT_ASSERT_TRUE (AsciiStrCmp (Capabilities[Index].AlgorithmOid, Capabilities[Other].AlgorithmOid) != 0);
      }
    }

    if (Capabilities != NULL) {
      FreePool (Capabilities);
    }
  }

  if (!Supported) {
    UT_LOG_WARNING ("Capability reporting is unavailable for all queried operations\n");
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestCapabilityErrors (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CONST EFI_GUID            Unknown = {
    0x00000000, 0x1111, 0x2222, { 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa }
  };
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;
  EFI_STATUS                Status;

  Capabilities    = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
  CapabilityCount = MAX_UINTN;
  Status          = GetCryptoOpCapability (&Unknown, &Capabilities, &CapabilityCount);
  UT_ASSERT_TRUE ((Status == EFI_NOT_FOUND) || (Status == EFI_UNSUPPORTED));
  UT_ASSERT_TRUE (Capabilities == NULL);
  UT_ASSERT_EQUAL (CapabilityCount, 0);

  Capabilities    = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
  CapabilityCount = MAX_UINTN;
  Status          = GetCryptoOpCapability (NULL, &Capabilities, &CapabilityCount);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (Capabilities == NULL);
  UT_ASSERT_EQUAL (CapabilityCount, 0);

  CapabilityCount = MAX_UINTN;
  Status          = GetCryptoOpCapability (mOperations[0], NULL, &CapabilityCount);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (CapabilityCount, 0);

  Capabilities = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
  Status       = GetCryptoOpCapability (mOperations[0], &Capabilities, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (Capabilities == NULL);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mCryptoOpCapabilityTest[] = {
  { "Capability arrays have owned, unique OIDs", "CryptoPkg.BaseCryptLib.OpCap", TestCryptoOpCapabilityArrays, NULL, NULL, NULL },
  { "Capability errors clear outputs",           "CryptoPkg.BaseCryptLib.OpCap", TestCapabilityErrors,         NULL, NULL, NULL },
};

UINTN  mCryptoOpCapabilityTestNum = ARRAY_SIZE (mCryptoOpCapabilityTest);
