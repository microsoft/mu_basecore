/** @file
  Base Library CPU Functions for all architectures.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

/**
  Set CD bit and clear NW bit of CR0 followed by a WBINVD.

  Disables the caches by setting the CD bit of CR0 to 1, clearing the NW bit of CR0 to 0,
  and executing a WBINVD instruction.  This function is only available on IA-32 and x64.

**/
VOID
EFIAPI
AsmDisableCache (
  VOID
  )
{
}


/**
  Perform a WBINVD and clear both the CD and NW bits of CR0.

  Enables the caches by executing a WBINVD instruction and then clear both the CD and NW
  bits of CR0 to 0.  This function is only available on IA-32 and x64.

**/
VOID
EFIAPI
AsmEnableCache (
  VOID
  )
{
}
