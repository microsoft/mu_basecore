/** @file
  TLS Library Host-based Unit Test entry point.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestTlsLib.h"

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

VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

int
main (
  int   argc,
  char  *argv[]
  )
{
  ProcessLibraryConstructorList ();
  return UefiTestMain ();
}
