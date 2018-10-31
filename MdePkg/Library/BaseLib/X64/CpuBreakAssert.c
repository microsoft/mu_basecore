/**@file CpuBreakAssert.c

CpuBreakAssert function for X64.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

void
__int2c (
  void
  );

#pragma intrinsic(__int2c)

/**
  Generates a debugger assertion break on the CPU.

  This does a special break into the debugger such that the debugger knows
  that the code running has hit an assertion, not a generic breakpoint.

**/
VOID
EFIAPI
CpuBreakAssert (
  VOID
  )
{
  __int2c ();
}
