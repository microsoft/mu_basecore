/** @file
  EnableInterrupts function

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
  Enables CPU interrupts.

**/
VOID
EFIAPI
EnableInterrupts (
  VOID
  )
{
  _asm {
    sti
  }
}

// MS_CHANGE - START

/**
  Enables CPU interrupts and then waits for an interrupt to arrive.

**/
VOID
EFIAPI
EnableInterruptsAndSleep (
  VOID
  )
{
  _asm {
    sti
    hlt
  }
}

// MS_CHANGE - END
