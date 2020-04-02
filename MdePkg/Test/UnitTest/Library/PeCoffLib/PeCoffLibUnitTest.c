/** @file
  Interface-level Unit Tests for PeCoffLib aimed at BasePeCoffLib.
  Hopefully are applicable to any implementation of PeCoffLib

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>

#include <stdio.h>

#define UNIT_TEST_APP_NAME     "PeCoffLib UnitTest"
#define UNIT_TEST_APP_VERSION  "1.0"


STATIC
UINT8*
InternalReadFile (
  IN CONST CHAR8    *Filename
  )
{
  return NULL;
}

/**
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TempTest(
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return UNIT_TEST_PASSED;
}

/**
  Initialze the unit test framework, suite, and unit tests for the
  Base64 conversion APIs of BaseLib and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw;
  UNIT_TEST_SUITE_HANDLE      GetImageInfoSuite;

  Fw = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
      goto EXIT;
  }

  //
  // Populate the B64 Encode Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&GetImageInfoSuite, Fw, "Name Me", "PeCoffLib.GetImageInfoSuite", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for GetImageInfoSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // --------------Suite-----------Description--------------Class Name----------Function--------Pre---Post-------------------Context-----------
  AddTestCase (GetImageInfoSuite, "Dummy", "Dummy", TempTest, NULL, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Fw);

EXIT:
  if (Fw) {
    FreeUnitTestFramework (Fw);
  }

  return Status;
}

/**
  Standard UEFI entry point for target based unit test execution from UEFI Shell.
**/
EFI_STATUS
EFIAPI
BaseLibUnitTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UnitTestingEntry ();
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int argc,
  char *argv[]
  )
{
  return UnitTestingEntry ();
}
