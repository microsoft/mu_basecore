/** @file
  This is a sample to demostrate the usage of the Unit Test Library that
  supports the PEI, DXE, SMM, and UEFI Shell environments.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "TestBaseCryptLib.h"

/**
  Initialize the unit test framework, suite, and unit tests for the
  sample unit tests and run the unit tests.

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
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestsfor BaseCryptLib Tests!  Status = %r\n", Status));
    goto Done;
  }

  //
  // Execute the tests.
  //
  UINT64  StartTick;
  UINT64  EndTick;
  UINT64  ElapsedNs;

  StartTick = GetPerformanceCounter ();
  Status    = RunAllTestSuites (Framework);
  EndTick   = GetPerformanceCounter ();

  //
  // The performance counter may count up or down, so take the absolute delta.
  //
  if (EndTick >= StartTick) {
    ElapsedNs = GetTimeInNanoSecond (EndTick - StartTick);
  } else {
    ElapsedNs = GetTimeInNanoSecond (StartTick - EndTick);
  }

  DEBUG ((DEBUG_INFO, "All test suites completed in %lu ms\n", ElapsedNs / 1000000));

Done:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard PEIM entry point for target based unit test execution from PEI.
**/
EFI_STATUS
EFIAPI
PeiEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  return UefiTestMain ();
}

/**
  Standard UEFI entry point for target based unit test execution from DXE, SMM,
  UEFI Shell.
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
