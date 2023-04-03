/** @file
  Mocked version of ResetSystemLib for SetupDataPkg unit tests

# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UnitTestLib.h>

/**
  Calling this function causes a system-wide reset. This sets
  all circuitry within the system to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.
  System reset should not return, if it returns, it means the system does
  not support cold reset.
**/
VOID
EFIAPI
ResetCold (
  VOID
  )
{
  BASE_LIBRARY_JUMP_BUFFER  *JumpBuf;

  JumpBuf = (BASE_LIBRARY_JUMP_BUFFER *)mock ();

  LongJump (JumpBuf, 1);
}
