/** @file
  Host-based entry point for BaseCryptLib performance tests.

  Note: When using BaseTimerLibNullTemplate, all timing results will be 0.
  Use a platform-specific TimerLib for meaningful results.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptoPerfLib.h"

VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  ProcessLibraryConstructorList ();
  return (int)UefiTestMain ();
}
