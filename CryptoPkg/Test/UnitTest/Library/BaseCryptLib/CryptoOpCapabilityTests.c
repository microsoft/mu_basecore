/** @file
  ECIT capability reporting unit tests -- CMS content-digest op.

  Exercises GetCryptoOpCapability(gCryptoOpCmsContentDigestGuid): allocated
  capability ownership, presence of a provider digest OID (SHA-256), and the
  dispatcher error paths. The content-digest op is accept-all, so it reports
  exactly the linked provider's message-digest set.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"
#include <Library/BaseLib.h>
#include <Guid/CryptoOpId.h>

#define OID_SHA256  "2.16.840.1.101.3.4.2.1"

/**
  The CMS content-digest op returns an allocated, non-empty capability array
  that includes the SHA-256 OID.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestCmsContentDigestReport (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;
  UINTN                     Index;
  BOOLEAN                   FoundSha256;

  Capabilities    = NULL;
  CapabilityCount = 0;
  Status          = GetCryptoOpCapability (
                      &gCryptoOpCmsContentDigestGuid,
                      &Capabilities,
                      &CapabilityCount
                      );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (Capabilities);
  UT_ASSERT_TRUE (CapabilityCount > 0);

  FoundSha256 = FALSE;
  for (Index = 0; Index < CapabilityCount; Index++) {
    UT_ASSERT_TRUE (Capabilities[Index].AlgorithmOid != NULL);
    UT_ASSERT_EQUAL (
      Capabilities[Index].AlgorithmOidSize,
      AsciiStrSize (Capabilities[Index].AlgorithmOid)
      );
    if (AsciiStrCmp (Capabilities[Index].AlgorithmOid, OID_SHA256) == 0) {
      FoundSha256 = TRUE;
    }
  }

  UT_ASSERT_TRUE (FoundSha256);

  FreePool (Capabilities);
  return UNIT_TEST_PASSED;
}

/**
  An unrecognized op GUID is rejected with EFI_NOT_FOUND.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestGetCryptoOpCapabilityUnknownOp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;
  CONST EFI_GUID            Unknown = {
    0x00000000, 0x1111, 0x2222, { 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa }
  };

  Capabilities    = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
  CapabilityCount = MAX_UINTN;
  Status          = GetCryptoOpCapability (
                      &Unknown,
                      &Capabilities,
                      &CapabilityCount
                      );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);
  UT_ASSERT_TRUE (Capabilities == NULL);
  UT_ASSERT_EQUAL (CapabilityCount, 0);

  return UNIT_TEST_PASSED;
}

/**
  NULL output arguments are rejected with EFI_INVALID_PARAMETER.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestGetCryptoOpCapabilityNullOutputs (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                Status;
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;

  CapabilityCount = MAX_UINTN;
  Status          = GetCryptoOpCapability (
                      &gCryptoOpCmsContentDigestGuid,
                      NULL,
                      &CapabilityCount
                      );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (CapabilityCount, 0);

  Capabilities = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
  Status       = GetCryptoOpCapability (
                   &gCryptoOpCmsContentDigestGuid,
                   &Capabilities,
                   NULL
                   );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (Capabilities == NULL);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mCryptoOpCapabilityTest[] = {
  //
  // Description                              Class                            Function                             PreReq  CleanUp  Context
  //
  { "CMS content-digest reports SHA-256", "CryptoPkg.BaseCryptLib.OpCap", TestCmsContentDigestReport,           NULL, NULL, NULL },
  { "GetCryptoOpCapability unknown op",   "CryptoPkg.BaseCryptLib.OpCap", TestGetCryptoOpCapabilityUnknownOp,   NULL, NULL, NULL },
  { "GetCryptoOpCapability NULL outputs", "CryptoPkg.BaseCryptLib.OpCap", TestGetCryptoOpCapabilityNullOutputs, NULL, NULL, NULL },
};

UINTN  mCryptoOpCapabilityTestNum = ARRAY_SIZE (mCryptoOpCapabilityTest);
