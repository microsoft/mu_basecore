/** @file
  UnitTestLib APIs to run unit tests using cmocka

  Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <UnitTestFrameworkTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

UNIT_TEST_FRAMEWORK_HANDLE
GetActiveFrameworkHandle (
  VOID
  )
{
  ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
  return NULL;
}
