/** @file
  Implements sample policy for DXE environment.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PeiServicesLib.h>

#include <Ppi/Policy.h>
#include <Library/UnitTestLib.h>

#include "PolicyTest.h"

#define UNIT_TEST_APP_NAME     "Policy PEI Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

//
// Global to store the protocol.
//
POLICY_PPI        *mPolicyPpi;
POLICY_INTERFACE  *mPolicyInterface;

/**
  Tests creating a policy to be consumed by later test phases.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
PeiPersistenceTest (
  IN UNIT_TEST_CONTEXT  Context
  )

{
  CONST EFI_GUID  PolicyGuid                          = PEI_TO_DXE_TEST_GUID;
  CONST EFI_GUID  PolicyFinalGuid                     = PEI_TO_DXE_TEST_GUID_FINALIZED;
  UINT8           Policy[PEI_TO_DXE_POLICY_SIZE]      = PEI_TO_DXE_POLICY;
  UINT8           PolicyFinal[PEI_TO_DXE_POLICY_SIZE] = PEI_TO_DXE_POLICY_FINALIZED;
  EFI_STATUS      Status;

  Status = mPolicyPpi->SetPolicy (&PolicyGuid, 0, &Policy[0], sizeof (Policy));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = mPolicyPpi->SetPolicy (&PolicyFinalGuid, POLICY_ATTRIBUTE_FINALIZED, &PolicyFinal[0], sizeof (PolicyFinal));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}

/**
  Entry point for the policy PEIM. Installs the policy service PPI.

  @param[in]    FileHandle      UNUSED.
  @param[in]    PeiServices     UNUSED.

  @retval EFI_SUCCESS           The interface was successfully installed.
  @retval EFI_OUT_OF_RESOURCES  There is no additional space in the PPI database.
**/
EFI_STATUS
EFIAPI
PeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      ServicePeiTests;

  DEBUG (
    (DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION)
    );

  // Retrieve the Policy PPI.
  Status = PeiServicesLocatePpi (
             &gPeiPolicyPpiGuid,
             0,
             NULL,
             (VOID **)&mPolicyPpi
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate policy PPI. Status = %r\n", Status));
    goto EXIT;
  }

  mPolicyInterface = mPolicyPpi;

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
  // Add the PEI service tests.
  //

  Status = CreateUnitTestSuite (
             &ServicePeiTests,
             Framework,
             "PEI Policy Service Tests",
             "Policy.Service.Pei",
             NULL,
             NULL
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to create PEI service tests. Status = %r\n", Status));
    goto EXIT;
  }

  AddTestCase (ServicePeiTests, "Create PEI policies for DXE", "PeiPersistenceTest", PeiPersistenceTest, NULL, NULL, NULL);

  //
  // Execute the tests.
  //

  Status = RunAllTestSuites (Framework);

EXIT:
  return Status;
}
