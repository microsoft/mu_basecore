/** @file
  Implements unit tests for the policy lib.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <PolicyInterface.h>
#include <Library/PolicyLib.h>
#include <Library/UnitTestLib.h>

#include "PolicyTest.h"

/**
  Tests scenarios when the major version is mismatched.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
MajorVersionChangeTest (
  IN UNIT_TEST_CONTEXT  Context
  )

{
  EFI_STATUS      Status;
  EFI_HANDLE      DataHandle;
  CONST EFI_GUID  PolicyGuid = {
    0x7673d164, 0xe0a5, 0x4c9d, { 0xb1, 0x1f, 0xe0, 0xd7, 0x9d, 0xdd, 0xfc, 0xdc }
  };

  VERIFIED_POLICY_DESCRIPTOR  TestDescriptor = {
    SIGNATURE_64 ('T', 'E', 'S', 'T', 'P', 'O', 'L', '2'),
    2,
    0,
    sizeof (UINT64)
  };

  //
  // Create and set the policy
  //

  Status = CreateVerifiedPolicy (&TestDescriptor, &DataHandle);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = SetVerifiedPolicy (&PolicyGuid, 0, DataHandle);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = CloseVerifiedPolicy (DataHandle);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  DataHandle = NULL;

  //
  // Retrieve the policy.
  //

  TestDescriptor.MajorVersion = 1;
  Status                      = GetVerifiedPolicy (&PolicyGuid, &TestDescriptor, NULL, &DataHandle);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INCOMPATIBLE_VERSION);

  return UNIT_TEST_PASSED;
}

/**
  Tests the basic verified test scenario.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
BasicVerifiedPolicyTest (
  IN UNIT_TEST_CONTEXT  Context
  )

{
  EFI_STATUS      Status;
  EFI_HANDLE      DataHandle;
  CONST EFI_GUID  PolicyGuid = {
    0x3bc1b571, 0x4755, 0x4d6a, { 0x9d, 0x5d, 0xcb, 0x71, 0x9a, 0x9f, 0xef, 0x6a }
  };

  VERIFIED_POLICY_DESCRIPTOR  TestDescriptor = {
    SIGNATURE_64 ('T', 'E', 'S', 'T', 'P', 'O', 'L', '1'),
    1,
    0,
    sizeof (UINT64)
  };

  DataHandle = NULL;

  //
  // Create and set the policy
  //

  Status = CreateVerifiedPolicy (&TestDescriptor, &DataHandle);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = SetVerifiedPolicy (&PolicyGuid, 0, DataHandle);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = CloseVerifiedPolicy (DataHandle);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  DataHandle = NULL;

  //
  // Retrieve the policy.
  //

  Status = GetVerifiedPolicy (&PolicyGuid, &TestDescriptor, NULL, &DataHandle);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = CloseVerifiedPolicy (DataHandle);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}

/**
  Tests the basic test scenario.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
BasicPolicyTest (
  IN UNIT_TEST_CONTEXT  Context
  )

{
  EFI_STATUS      Status;
  UINT8           TestData[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
  UINT8           ReadData[16] = { 0 };
  UINT16          PolicySize;
  CONST EFI_GUID  PolicyGuid = {
    0x30c24026, 0xee8f, 0x494a, { 0x92, 0x48, 0xc0, 0x05, 0x28, 0xc6, 0x7f, 0x5b }
  };

  PolicySize = sizeof (ReadData);
  Status     = GetPolicy (&PolicyGuid, NULL, &ReadData[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  Status = SetPolicy (&PolicyGuid, 0, &TestData[0], sizeof (TestData));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = GetPolicy (&PolicyGuid, NULL, &ReadData[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_MEM_EQUAL (&TestData[0], &ReadData[0], sizeof (TestData));

  Status = RemovePolicy (&PolicyGuid);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  Status = RemovePolicy (&PolicyGuid);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  return UNIT_TEST_PASSED;
}

/**
  Add the common policy library tests.

  @param[in]  Framework       The test framework to add the common policy
                              library tests

  @retval   EFI_SUCCESS       Test added to framework.
  @retval   OTHER             Error returned by subfunction.
**/
EFI_STATUS
PolicyLibCommonCreateTests (
  UNIT_TEST_FRAMEWORK_HANDLE  Framework
  )

{
  UNIT_TEST_SUITE_HANDLE  LibCommonTests;
  EFI_STATUS              Status;

  Status = CreateUnitTestSuite (
             &LibCommonTests,
             Framework,
             "Common Policy Library Tests",
             "Policy.Lib.Common",
             NULL,
             NULL
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  AddTestCase (LibCommonTests, "Tests the basic policy lib interfaces", "BasicPolicyTest", BasicPolicyTest, NULL, NULL, NULL);
  AddTestCase (LibCommonTests, "Test basic verified policy creation", "BasicVerifiedPolicyTest", BasicVerifiedPolicyTest, NULL, NULL, NULL);
  AddTestCase (LibCommonTests, "Test change of major version", "MajorVersionChangeTest", MajorVersionChangeTest, NULL, NULL, NULL);

  return EFI_SUCCESS;
}
