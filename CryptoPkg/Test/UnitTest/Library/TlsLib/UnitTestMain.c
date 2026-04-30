/** @file
  TLS Library Unit Test entry point for DXE/UEFI Shell.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestTlsLib.h"

/**
  Initialize the unit test framework, suite, and unit tests,
  then run all test suites.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));
  Status = CreateUnitTest (UNIT_TEST_NAME, UNIT_TEST_VERSION, &Framework);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to create unit tests! Status = %r\n", Status));
    goto Done;
  }

  Status = RunAllTestSuites (Framework);

Done:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard UEFI entry point for target based unit test execution from DXE,
  SMM, UEFI Shell.
**/
EFI_STATUS
EFIAPI
DxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UefiTestMain ();
}
