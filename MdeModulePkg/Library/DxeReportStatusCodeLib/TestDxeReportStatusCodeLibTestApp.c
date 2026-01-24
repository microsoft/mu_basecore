/** @file
  UEFI based application for unit testing the DxeReportStatusCodeLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <TestDxeReportStatusCodeLibTestApp.h>

#define UNIT_TEST_NAME     "Dxe Report Status Code Lib Unit Test Application"
#define UNIT_TEST_VERSION  "0.1"

UNIT_TEST_STATUS
EFIAPI
TestReportStatusCodeDoesNotInvertTpl (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;
  EFI_SYSTEM_TABLE  *SystemTable = (EFI_SYSTEM_TABLE *)Context;

  OldTpl = SystemTable->BootServices->RaiseTPL (TPL_HIGH_LEVEL);

  Status = ReportStatusCode (EFI_PROGRESS_CODE, 0x0);

  SystemTable->BootServices->RestoreTPL (OldTpl);

  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("ReportStatusCode returned error: %r", Status);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

EFI_STATUS
EFIAPI
UefiTestMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      NoTplInversionTestSuite;

  Status                  = EFI_SUCCESS;
  Framework               = NULL;
  NoTplInversionTestSuite = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Test Suite 1
  //
  Status = CreateUnitTestSuite (&NoTplInversionTestSuite, Framework, "No TPL Inversion Tests", "Dxe.ReportStatusCode.NoTplInversion", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for No TPL Inversion Tests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  AddTestCase (NoTplInversionTestSuite, "Test ReportStatusCode TPL High", "TestReportStatusCodeTplHigh", TestReportStatusCodeDoesNotInvertTpl, NULL, NULL, SystemTable);

  Status = RunAllTestSuites (Framework);

Exit:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}
