/** @file
  Implements sample policy for DXE environment.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/MmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/MmPolicy.h>
#include <Library/UnitTestLib.h>

#include "PolicyTest.h"

#define UNIT_TEST_APP_NAME     "Policy MM Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

//
// Global to store the protocol.
//
MM_POLICY_PROTOCOL  *mPolicyProtocol;
POLICY_INTERFACE    *mPolicyInterface;

/**
  Test DXE specific scenarios.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
IngestedPolicyTest (
  IN UNIT_TEST_CONTEXT  Context
  )

{
  CONST EFI_GUID  PolicyGuid                          = PEI_TO_DXE_TEST_GUID;
  CONST EFI_GUID  PolicyFinalGuid                     = PEI_TO_DXE_TEST_GUID_FINALIZED;
  UINT8           Policy[PEI_TO_DXE_POLICY_SIZE]      = PEI_TO_DXE_POLICY;
  UINT8           PolicyFinal[PEI_TO_DXE_POLICY_SIZE] = PEI_TO_DXE_POLICY_FINALIZED;
  UINT8           GetPolicy[PEI_TO_DXE_POLICY_SIZE];
  EFI_STATUS      Status;
  UINT16          PolicySize;
  UINT64          Attributes;

  ZeroMem (&GetPolicy[0], PEI_TO_DXE_POLICY_SIZE);
  PolicySize = 0;

  //
  // Ensure the policy can be retrieved.
  //

  Status = mPolicyProtocol->GetPolicy (&PolicyGuid, &Attributes, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_BUFFER_TOO_SMALL);
  UT_ASSERT_EQUAL (PolicySize, PEI_TO_DXE_POLICY_SIZE);
  UT_ASSERT_EQUAL ((Attributes & POLICY_ATTRIBUTE_FINALIZED), 0);

  Status = mPolicyProtocol->GetPolicy (&PolicyGuid, &Attributes, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (PolicySize, PEI_TO_DXE_POLICY_SIZE);
  UT_ASSERT_EQUAL ((Attributes & POLICY_ATTRIBUTE_FINALIZED), 0);
  UT_ASSERT_MEM_EQUAL (&GetPolicy[0], &Policy[0], PolicySize);

  //
  // The policy should be updatable without problem.
  //

  Policy[0] = 0xAF;
  Policy[5] = 0xFA;
  Status    = mPolicyProtocol->SetPolicy (&PolicyGuid, POLICY_ATTRIBUTE_FINALIZED, &Policy[0], PEI_TO_DXE_POLICY_SIZE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = mPolicyProtocol->GetPolicy (&PolicyGuid, &Attributes, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (PolicySize, PEI_TO_DXE_POLICY_SIZE);
  UT_ASSERT_EQUAL ((Attributes & POLICY_ATTRIBUTE_FINALIZED), POLICY_ATTRIBUTE_FINALIZED);
  UT_ASSERT_MEM_EQUAL (&GetPolicy[0], &Policy[0], PolicySize);

  //
  // The policy should also be removeable without any trace of the previous values.
  //

  Status = mPolicyProtocol->RemovePolicy (&PolicyGuid);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = mPolicyProtocol->GetPolicy (&PolicyGuid, NULL, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  //
  // Check the finalized policy also exists.
  //

  PolicySize = 0;

  Status = mPolicyProtocol->GetPolicy (&PolicyFinalGuid, &Attributes, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_BUFFER_TOO_SMALL);
  UT_ASSERT_EQUAL (PolicySize, PEI_TO_DXE_POLICY_SIZE);
  UT_ASSERT_EQUAL ((Attributes & POLICY_ATTRIBUTE_FINALIZED), POLICY_ATTRIBUTE_FINALIZED);

  Status = mPolicyProtocol->GetPolicy (&PolicyFinalGuid, &Attributes, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (PolicySize, PEI_TO_DXE_POLICY_SIZE);
  UT_ASSERT_EQUAL ((Attributes & POLICY_ATTRIBUTE_FINALIZED), POLICY_ATTRIBUTE_FINALIZED);
  UT_ASSERT_MEM_EQUAL (&GetPolicy[0], &PolicyFinal[0], PolicySize);

  //
  // The policy should not be updatable.
  //

  PolicyFinal[0] = 0xAB;
  PolicyFinal[5] = 0xBA;
  Status         = mPolicyProtocol->SetPolicy (&PolicyFinalGuid, 0, &PolicyFinal[0], PEI_TO_DXE_POLICY_SIZE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);

  //
  // Remove the policy.
  //

  Status = mPolicyProtocol->RemovePolicy (&PolicyFinalGuid);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = mPolicyProtocol->GetPolicy (&PolicyFinalGuid, NULL, &GetPolicy[0], &PolicySize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  return UNIT_TEST_PASSED;
}

/**
  Test driver entry point.

  @param[in]  ImageHandle     The firmware allocated handle for the EFI image.
  @param[in]  SystemTable     UNUSED.

  @retval   EFI_SUCCESS       Policy store initialized and protocol installed.
  @retval   other             Sample routines returned a failure.
**/
EFI_STATUS
EFIAPI
MmEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      ServiceDxeTests;

  DEBUG (
    (DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION)
    );

  Status = gMmst->MmLocateProtocol (
                    &gMmPolicyProtocolGuid,
                    NULL,
                    (VOID **)&mPolicyProtocol
                    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate policy protocol. Status = %r\n", Status));
    goto EXIT;
  }

  mPolicyInterface = mPolicyProtocol;

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Add the common service tests.
  //

  Status = PolicyServiceCommonCreateTests (Framework);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in PolicyServiceCommonCreateTests. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Add the common lib tests.
  //

  Status = PolicyLibCommonCreateTests (Framework);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in PolicyLibCommonCreateTests. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Add the DXE service tests.
  //

  Status = CreateUnitTestSuite (
             &ServiceDxeTests,
             Framework,
             "MM Policy Service Tests",
             "Policy.Service.Mm",
             NULL,
             NULL
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to create MM service tests. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // On AARCH64, PEI will not run before standalone so skip the ingestion test.
  //

 #if !defined (MDE_CPU_AARCH64)
  AddTestCase (ServiceDxeTests, "Test PEI created policies", "IngestedPolicyTest", IngestedPolicyTest, NULL, NULL, NULL);
 #endif

  //
  // Execute the tests.
  //

  Status = RunAllTestSuites (Framework);

EXIT:
  return Status;
}
