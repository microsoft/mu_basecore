/** @file
  Base Library CPU Functions for all architectures.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

/**
  Disables CPU interrupts and returns the interrupt state prior to the disable
  operation.

  @retval TRUE  CPU interrupts were enabled on entry to this call.
  @retval FALSE CPU interrupts were disabled on entry to this call.

**/
BOOLEAN
EFIAPI
SaveAndDisableInterrupts (
  VOID
  )
{
  return FALSE;
}

/**
  Set the current CPU interrupt state.

  Sets the current CPU interrupt state to the state specified by
  InterruptState. If InterruptState is TRUE, then interrupts are enabled. If
  InterruptState is FALSE, then interrupts are disabled. InterruptState is
  returned.

  @param  InterruptState  TRUE if interrupts should be enabled. FALSE if
                          interrupts should be disabled.

  @return InterruptState

**/
BOOLEAN
EFIAPI
SetInterruptState (
  IN      BOOLEAN                   InterruptState
  )
{
  return InterruptState;
}

/**
  Uses as a barrier to stop speculative execution.

  Ensures that no later instruction will execute speculatively, until all prior
  instructions have completed.

**/
VOID
EFIAPI
SpeculationBarrier (
  VOID
  )
{
}

/**
  Requests CPU to pause for a short period of time.

  Requests CPU to pause for a short period of time. Typically used in MP
  systems to prevent memory starvation while waiting for a spin lock.

**/
VOID
EFIAPI
CpuPause (
  VOID
  )
{
}

/**
  Performs a serializing operation on all load-from-memory instructions that
  were issued prior the AsmLfence function.

  Executes a LFENCE instruction. This function is only available on IA-32 and x64.

**/
VOID
EFIAPI
AsmLfence (
  VOID
  )
{
  return;
}