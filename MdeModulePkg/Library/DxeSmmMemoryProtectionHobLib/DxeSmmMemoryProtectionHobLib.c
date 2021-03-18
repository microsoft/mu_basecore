/**@file
Library fills out gMPS global for DXE and SMM instance.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include "CommonMemoryProtectionHobLib.h"

/**
  Abstraction layer for library constructor of DXE and SMM instances.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
DxeSmmMemoryProtectionHobLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return CommonMemoryProtectionHobLibConstructor ();
}
