/**@file
Library fills out gMPS global for Standalone MM instance.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiMm.h>

#include "CommonMemoryProtectionHobLib.h"

/**
  Abstraction layer for library constructor of Standalone MM instances.

  @param  ImageHandle    The image handle of the Standalone MM Driver.
  @param  MmSystemTable  A pointer to the MM System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
StandaloneMmMemoryProtectionHobLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  return CommonMemoryProtectionHobLibConstructor ();
}
