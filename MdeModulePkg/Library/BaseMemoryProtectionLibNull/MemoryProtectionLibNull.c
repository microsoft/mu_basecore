/**@file
Library to support platform-specific global controls for all memory protections.

Enables platform-specific logic around when to enforce boot time and runtime memory
protections. This global control works with the PCDs to fine-tune support, but will override
the PCDs in the "disabled" case.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MemoryProtectionLib.h>

/**
 Updates the memory protection global toggle

  @param[in] Setting      What the memory protection global toggle should
                          be set to

  @retval EFI_SUCCESS     Memory protection global toggle was updated
  @retval EFI_UNSUPPORTED This function is either not available in this
                          execution phase or a null instance is used
 **/
EFI_STATUS
EFIAPI
SetMemoryProtectionGlobalToggle(
  IN BOOLEAN     Setting
  )
{
  return EFI_UNSUPPORTED;
}

/**
 Updates the memory protection global toggle

  @retval TRUE            Memory protection global toggle is on
  @retval FALSE           Memory protection global toggle is off, meaning
                          no memory protections can be initialized
 **/
BOOLEAN
EFIAPI
IsMemoryProtectionGlobalToggleEnabled(
  VOID
  )
{
  return TRUE;
}
