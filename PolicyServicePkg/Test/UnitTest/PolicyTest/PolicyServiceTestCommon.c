/** @file
  Implements unit tests for the policy service.

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
#include <Library/UnitTestLib.h>

#include "PolicyTest.h"

/**
  Tests the basic lifecycle of a policy.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
BasicCreatePolicyTest (
  IN UNIT_TEST_CONTEXT  Context
  )

{
  EFI_STATUS      Status;
  UINT8           SetPolicy[100];
  UINT8           GetPolicy[100];
  UINT16          PolicySize;
  CONST EFI_GUID  PolicyGuid = {
    0x576e3fbc, 0x0cf1, 0x4b54, { 0x82, 0xc1, 0x98, 0xbf, 0xff, 0xa0, 0xcb, 0x7e }
  };

  PolicySize = 0;
  SetMem (&SetPolicy[0], sizeof (SetPolicy), 0xCD);
  ZeroMem (&GetPolicy[0], sizeof (GetPolicy));

  Status = mPolicyInterface->GetPolicy (&PolicyGuid, NULL, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  Status = mPolicyInterface->SetPolicy (&PolicyGuid, 0, &SetPolicy[0], sizeof (SetPolicy));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  //
  // Ensure the get policy returns the correct size.
  //

  PolicySize = 0;

  Status = mPolicyInterface->GetPolicy (&PolicyGuid, NULL, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_BUFFER_TOO_SMALL);
  UT_ASSERT_EQUAL (PolicySize, sizeof (SetPolicy));

  //
  // Check the policy comes back correctly.
  //

  Status = mPolicyInterface->GetPolicy (&PolicyGuid, NULL, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (PolicySize, sizeof (SetPolicy));
  UT_ASSERT_MEM_EQUAL (&GetPolicy[0], &SetPolicy[0], PolicySize);

  //
  // Check the policy can be removed.
  //

  Status = mPolicyInterface->RemovePolicy (&PolicyGuid);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = mPolicyInterface->GetPolicy (&PolicyGuid, NULL, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  return UNIT_TEST_PASSED;
}

/**
  Tests scenarios where multiple policies are created with the same GUID.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
DuplicatePolicyTest (
  IN UNIT_TEST_CONTEXT  Context
  )

{
  EFI_STATUS      Status;
  UINT8           SetPolicy[100];
  UINT8           GetPolicy[100];
  UINT16          PolicySize;
  UINT64          Attributes;
  CONST EFI_GUID  PolicyGuid = {
    0x9af7da34, 0x0f81, 0x4921, { 0xa1, 0x98, 0xa9, 0x04, 0x1a, 0x33, 0xa1, 0x9f }
  };

  PolicySize = 0;
  ZeroMem (&GetPolicy[0], sizeof (GetPolicy));
  ZeroMem (&SetPolicy[0], sizeof (SetPolicy));

  Status = mPolicyInterface->SetPolicy (&PolicyGuid, 0, &SetPolicy[0], sizeof (SetPolicy));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  //
  // Override the policy with a different value.
  //

  SetMem (&SetPolicy[0], sizeof (SetPolicy), 0xCD);
  Status = mPolicyInterface->SetPolicy (&PolicyGuid, POLICY_ATTRIBUTE_FINALIZED, &SetPolicy[0], sizeof (SetPolicy));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  PolicySize = sizeof (SetPolicy);
  Status     = mPolicyInterface->GetPolicy (&PolicyGuid, &Attributes, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (PolicySize, sizeof (SetPolicy));
  UT_ASSERT_EQUAL (Attributes, POLICY_ATTRIBUTE_FINALIZED);
  UT_ASSERT_MEM_EQUAL (&GetPolicy[0], &SetPolicy[0], PolicySize);

  //
  // Ensure the policy cannot be written after being finalized.
  //

  SetMem (&SetPolicy[0], sizeof (SetPolicy), 0xCD);
  Status = mPolicyInterface->SetPolicy (&PolicyGuid, POLICY_ATTRIBUTE_FINALIZED, &SetPolicy[0], sizeof (SetPolicy));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);

  Status = mPolicyInterface->RemovePolicy (&PolicyGuid);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}

/**
  Tests multiple policies existing at once.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
MultiplePolicyTest (
  IN UNIT_TEST_CONTEXT  Context
  )

{
  EFI_STATUS      Status;
  UINT64          Policies[10];
  UINT64          GetPolicy;
  UINT32          PolicyIndex;
  UINT16          PolicySize;
  CONST EFI_GUID  PolicyGuids[10] = {
    { 0xaff48896, 0x6725, 0x40a8, { 0xa1, 0x8e, 0x69, 0xb8, 0x36, 0x88, 0xfa, 0x71 }
    },
    { 0xd3f329d8, 0x5ab5, 0x471c, { 0xaf, 0x32, 0x1f, 0x94, 0xde, 0xba, 0x71, 0x02 }
    },
    { 0x0b490c23, 0x09a5, 0x4f5c, { 0x97, 0xee, 0xc0, 0x83, 0xaf, 0xbb, 0x21, 0x76 }
    },
    { 0xcf81f8c6, 0xc65f, 0x44d3, { 0x92, 0xaa, 0x36, 0xf6, 0x0f, 0xd8, 0x9f, 0x20 }
    },
    { 0x67f9e373, 0x99a2, 0x43d6, { 0x83, 0x7f, 0xc4, 0xe1, 0xba, 0x6d, 0x0b, 0xc1 }
    },
    { 0x0b3cfd24, 0xcfe3, 0x4614, { 0x81, 0x78, 0xc1, 0x4a, 0x12, 0xe2, 0x15, 0x5e }
    },
    { 0xc6c8edfe, 0x7867, 0x4aac, { 0xa8, 0x02, 0xda, 0xde, 0xe3, 0x98, 0x1e, 0x50 }
    },
    { 0xa7f9df09, 0x5502, 0x45cb, { 0x93, 0x77, 0xd4, 0x80, 0x09, 0x30, 0xed, 0xe8 }
    },
    { 0x1daddb05, 0xd36f, 0x4e6c, { 0x8b, 0x6b, 0x64, 0x7b, 0xb9, 0x2b, 0x29, 0x65 }
    },
    { 0x46433b36, 0x871a, 0x4755, { 0x96, 0x36, 0xce, 0x9f, 0x3a, 0x44, 0x67, 0x51 }
    }
  };

  //
  // Set all of the policies.
  //

  for (PolicyIndex = 0; PolicyIndex < 10; PolicyIndex++) {
    Policies[PolicyIndex] = PolicyIndex;
    PolicySize            = 0;

    Status = mPolicyInterface->GetPolicy (&PolicyGuids[PolicyIndex], NULL, &GetPolicy, &PolicySize);
    UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

    Status = mPolicyInterface->SetPolicy (&PolicyGuids[PolicyIndex], 0, &Policies[PolicyIndex], sizeof (Policies[0]));
    UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  }

  //
  // Get all the policies.
  //

  for (PolicyIndex = 0; PolicyIndex < 10; PolicyIndex++) {
    PolicySize = sizeof (Policies[0]);

    Status = mPolicyInterface->GetPolicy (&PolicyGuids[PolicyIndex], NULL, &GetPolicy, &PolicySize);
    UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
    UT_ASSERT_EQUAL (PolicySize, sizeof (Policies[0]));
    UT_ASSERT_EQUAL (GetPolicy, Policies[PolicyIndex]);
  }

  //
  // Remove all the policies.
  //

  for (PolicyIndex = 0; PolicyIndex < 10; PolicyIndex++) {
    PolicySize = sizeof (Policies[0]);

    Status = mPolicyInterface->RemovePolicy (&PolicyGuids[PolicyIndex]);
    UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  }

  return UNIT_TEST_PASSED;
}

/**
  Add the common policy service tests.

  @param[in]  Framework       The test framework to add the common policy
                              service tests

  @retval   EFI_SUCCESS       Test added to framework.
  @retval   OTHER             Error returned by subfunction.
**/
EFI_STATUS
PolicyServiceCommonCreateTests (
  UNIT_TEST_FRAMEWORK_HANDLE  Framework
  )

{
  UNIT_TEST_SUITE_HANDLE  ServiceCommonTests;
  EFI_STATUS              Status;

  Status = CreateUnitTestSuite (
             &ServiceCommonTests,
             Framework,
             "Common Policy Service Tests",
             "Policy.Service.Common",
             NULL,
             NULL
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  AddTestCase (ServiceCommonTests, "Test basic policy creation", "BasicCreatePolicyTest", BasicCreatePolicyTest, NULL, NULL, NULL);
  AddTestCase (ServiceCommonTests, "Test duplicate/override policy", "DuplicatePolicyTest", DuplicatePolicyTest, NULL, NULL, NULL);
  AddTestCase (ServiceCommonTests, "Multiple policy test", "MultiplePolicyTest", MultiplePolicyTest, NULL, NULL, NULL);

  return EFI_SUCCESS;
}
