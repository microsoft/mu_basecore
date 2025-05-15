/** @file
  SMM Core SMM Services Table Library.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>

EFI_MM_SYSTEM_TABLE           *gMmst = NULL;
extern EFI_SMM_SYSTEM_TABLE2  gSmmCoreSmst;

/**
  The constructor function caches the pointer of SMM Services Table.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmCoreMmServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gMmst = (EFI_MM_SYSTEM_TABLE *)&gSmmCoreSmst;
  return EFI_SUCCESS;
}
