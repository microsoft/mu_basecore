/** @file
  Protocol capability forwarding and ABI compatibility tests.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "../BaseCryptLib/TestBaseCryptLib.h"
#include <Guid/CryptoOpId.h>
#include <Protocol/OneCrypto.h>

extern ONE_CRYPTO_PROTOCOL  *gCryptoProtocol;

VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

STATIC_ASSERT (
  OFFSET_OF (ONE_CRYPTO_PROTOCOL, GetCryptoOpCapability) ==
  OFFSET_OF (ONE_CRYPTO_PROTOCOL, GetCryptoProviderVersionString) +
  sizeof (ONE_CRYPTO_GET_CRYPTO_PROVIDER_VERSION_STRING),
  "Capability reporting must be appended after all protocol 1.0 services"
  );

STATIC UINTN       mCalls;
STATIC EFI_STATUS  mProviderStatus;

STATIC
EFI_STATUS
EFIAPI
ReportCapabilities (
  IN  CONST EFI_GUID            *OpIdGuid,
  OUT BASE_CRYPT_OP_CAPABILITY  **Capabilities,
  OUT UINTN                     *CapabilityCount
  )
{
  STATIC CONST CHAR8  Oid[] = "2.16.840.1.101.3.4.2.1";
  CHAR8               *OidCopy;

  mCalls++;
  *Capabilities    = NULL;
  *CapabilityCount = 0;
  if (!CompareGuid (OpIdGuid, &gCryptoOpCmsVerifyGuid)) {
    return EFI_INVALID_PARAMETER;
  }

  if (EFI_ERROR (mProviderStatus)) {
    return mProviderStatus;
  }

  *Capabilities = AllocatePool (sizeof (**Capabilities) + sizeof (Oid));
  if (*Capabilities == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OidCopy = (CHAR8 *)(*Capabilities + 1);
  CopyMem (OidCopy, Oid, sizeof (Oid));
  (*Capabilities)->AlgorithmOid     = OidCopy;
  (*Capabilities)->AlgorithmOidSize = sizeof (Oid);
  *CapabilityCount                  = 1;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ReportPartialCapabilities (
  IN  CONST EFI_GUID            *OpIdGuid,
  OUT BASE_CRYPT_OP_CAPABILITY  **Capabilities,
  OUT UINTN                     *CapabilityCount
  )
{
  mCalls++;
  *Capabilities    = NULL;
  *CapabilityCount = 0;
  if (CompareGuid (OpIdGuid, &gCryptoOpCmsContentDigestGuid) ||
      CompareGuid (OpIdGuid, &gCryptoOpAuthenticodeHashGuid))
  {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestUnavailableProtocols (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST UINT16       Versions[][2] = {
    { 1, 0 }, { 2, 0 }, { 0, 0 }
  };
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;
  UINTN                     Index;
  EFI_STATUS                Status;
  ONE_CRYPTO_PROTOCOL       Protocol;

  gCryptoProtocol = NULL;
  Capabilities    = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
  CapabilityCount = MAX_UINTN;
  Status          = GetCryptoOpCapability (&gCryptoOpCmsVerifyGuid, &Capabilities, &CapabilityCount);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_UNSUPPORTED);
  UT_ASSERT_TRUE (Capabilities == NULL);
  UT_ASSERT_EQUAL (CapabilityCount, 0);

  for (Index = 0; Index < ARRAY_SIZE (Versions); Index++) {
    // Deliberately omit the new field to exercise version-before-field checks.
    gCryptoProtocol = AllocateZeroPool (OFFSET_OF (ONE_CRYPTO_PROTOCOL, GetCryptoOpCapability));
    UT_ASSERT_NOT_NULL (gCryptoProtocol);
    gCryptoProtocol->Major = Versions[Index][0];
    gCryptoProtocol->Minor = Versions[Index][1];
    Capabilities           = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
    CapabilityCount        = MAX_UINTN;
    Status                 = GetCryptoOpCapability (&gCryptoOpCmsVerifyGuid, &Capabilities, &CapabilityCount);
    FreePool (gCryptoProtocol);
    gCryptoProtocol = NULL;
    UT_ASSERT_STATUS_EQUAL (Status, EFI_UNSUPPORTED);
    UT_ASSERT_TRUE (Capabilities == NULL);
    UT_ASSERT_EQUAL (CapabilityCount, 0);
  }

  ZeroMem (&Protocol, sizeof (Protocol));
  Protocol.Major  = 1;
  Protocol.Minor  = 1;
  gCryptoProtocol = &Protocol;
  Capabilities    = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
  CapabilityCount = MAX_UINTN;
  Status          = GetCryptoOpCapability (&gCryptoOpCmsVerifyGuid, &Capabilities, &CapabilityCount);
  gCryptoProtocol = NULL;
  UT_ASSERT_STATUS_EQUAL (Status, EFI_UNSUPPORTED);
  UT_ASSERT_TRUE (Capabilities == NULL);
  UT_ASSERT_EQUAL (CapabilityCount, 0);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestForwarding (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST UINT16       MinorVersions[] = { 1, 2, MAX_UINT16 };
  ONE_CRYPTO_PROTOCOL       Protocol;
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;
  UINTN                     Index;
  EFI_STATUS                Status;

  ZeroMem (&Protocol, sizeof (Protocol));
  Protocol.Major                 = 1;
  Protocol.GetCryptoOpCapability = ReportCapabilities;
  mProviderStatus                = EFI_SUCCESS;
  for (Index = 0; Index < ARRAY_SIZE (MinorVersions); Index++) {
    Protocol.Minor  = MinorVersions[Index];
    gCryptoProtocol = &Protocol;
    mCalls          = 0;
    Capabilities    = NULL;
    CapabilityCount = 0;
    Status          = GetCryptoOpCapability (&gCryptoOpCmsVerifyGuid, &Capabilities, &CapabilityCount);
    gCryptoProtocol = NULL;
    UT_ASSERT_NOT_EFI_ERROR (Status);
    UT_ASSERT_EQUAL (mCalls, 1);
    UT_ASSERT_EQUAL (CapabilityCount, 1);
    UT_ASSERT_NOT_NULL (Capabilities);
    UT_ASSERT_EQUAL (Capabilities->AlgorithmOidSize, sizeof ("2.16.840.1.101.3.4.2.1"));
    UT_ASSERT_MEM_EQUAL (Capabilities->AlgorithmOid, "2.16.840.1.101.3.4.2.1", Capabilities->AlgorithmOidSize);
    FreePool (Capabilities);
  }

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestPartialCapabilityArrays (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  ONE_CRYPTO_PROTOCOL  Protocol;
  UNIT_TEST_STATUS     Result;

  ZeroMem (&Protocol, sizeof (Protocol));
  Protocol.Major                 = 1;
  Protocol.Minor                 = 1;
  Protocol.GetCryptoOpCapability = ReportPartialCapabilities;
  gCryptoProtocol                = &Protocol;
  mCalls                         = 0;
  Result                         = TestCryptoOpCapabilityArrays (NULL);
  gCryptoProtocol                = NULL;
  UT_ASSERT_EQUAL (Result, UNIT_TEST_PASSED);
  UT_ASSERT_EQUAL (mCalls, 4);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestProviderErrors (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST EFI_STATUS   Errors[] = { EFI_NOT_FOUND, EFI_UNSUPPORTED, EFI_OUT_OF_RESOURCES };
  ONE_CRYPTO_PROTOCOL       Protocol;
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;
  UINTN                     Index;
  EFI_STATUS                Status;

  ZeroMem (&Protocol, sizeof (Protocol));
  Protocol.Major                 = 1;
  Protocol.Minor                 = 1;
  Protocol.GetCryptoOpCapability = ReportCapabilities;
  for (Index = 0; Index < ARRAY_SIZE (Errors); Index++) {
    gCryptoProtocol = &Protocol;
    mCalls          = 0;
    mProviderStatus = Errors[Index];
    Capabilities    = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
    CapabilityCount = MAX_UINTN;
    Status          = GetCryptoOpCapability (&gCryptoOpCmsVerifyGuid, &Capabilities, &CapabilityCount);
    gCryptoProtocol = NULL;
    mProviderStatus = EFI_SUCCESS;
    UT_ASSERT_STATUS_EQUAL (Status, Errors[Index]);
    UT_ASSERT_EQUAL (mCalls, 1);
    UT_ASSERT_TRUE (Capabilities == NULL);
    UT_ASSERT_EQUAL (CapabilityCount, 0);
  }

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestInvalidArguments (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;
  EFI_STATUS                Status;

  gCryptoProtocol = NULL;
  Capabilities    = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
  CapabilityCount = MAX_UINTN;
  Status          = GetCryptoOpCapability (NULL, &Capabilities, &CapabilityCount);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (Capabilities == NULL);
  UT_ASSERT_EQUAL (CapabilityCount, 0);

  CapabilityCount = MAX_UINTN;
  Status          = GetCryptoOpCapability (&gCryptoOpCmsVerifyGuid, NULL, &CapabilityCount);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (CapabilityCount, 0);

  Capabilities = (BASE_CRYPT_OP_CAPABILITY *)(UINTN)1;
  Status       = GetCryptoOpCapability (&gCryptoOpCmsVerifyGuid, &Capabilities, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (Capabilities == NULL);
  return UNIT_TEST_PASSED;
}

int
main (
  int   Argc,
  char  *Argv[]
  )
{
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Suite;
  EFI_STATUS                  Status;

  ProcessLibraryConstructorList ();
  Status = InitUnitTestFramework (&Framework, "OneCrypto capability forwarding", gEfiCallerBaseName, "1.0");
  if (EFI_ERROR (Status)) {
    return 1;
  }

  Status = CreateUnitTestSuite (&Suite, Framework, "Protocol 1.1 capabilities", "CryptoPkg.OneCrypto", NULL, NULL);
  if (!EFI_ERROR (Status)) {
    Status = AddTestCase (Suite, "Older, incompatible, and missing services", "CryptoPkg.OneCrypto.Capability", TestUnavailableProtocols, NULL, NULL, NULL);
  }

  if (!EFI_ERROR (Status)) {
    Status = AddTestCase (Suite, "Protocol 1.1 and later minors forward owned capabilities", "CryptoPkg.OneCrypto.Capability", TestForwarding, NULL, NULL, NULL);
  }

  if (!EFI_ERROR (Status)) {
    Status = AddTestCase (Suite, "Empty sets and partial operation support satisfy the contract", "CryptoPkg.OneCrypto.Capability", TestPartialCapabilityArrays, NULL, NULL, NULL);
  }

  if (!EFI_ERROR (Status)) {
    Status = AddTestCase (Suite, "Provider errors propagate with cleared outputs", "CryptoPkg.OneCrypto.Capability", TestProviderErrors, NULL, NULL, NULL);
  }

  if (!EFI_ERROR (Status)) {
    Status = AddTestCase (Suite, "Invalid arguments clear available outputs", "CryptoPkg.OneCrypto.Capability", TestInvalidArguments, NULL, NULL, NULL);
  }

  if (!EFI_ERROR (Status)) {
    Status = RunAllTestSuites (Framework);
  }

  FreeUnitTestFramework (Framework);
  return EFI_ERROR (Status) ? 1 : 0;
}
