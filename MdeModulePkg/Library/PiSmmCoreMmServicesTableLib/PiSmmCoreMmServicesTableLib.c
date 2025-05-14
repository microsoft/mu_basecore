/** @file
  SMM Core SMM Services Table Library.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>

EFI_MM_SYSTEM_TABLE  *gMmst = NULL;

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
  return EFI_SUCCESS;
}

/**
  This function allows the caller to determine if the driver is executing in
  System Management Mode(SMM).

  This function returns TRUE if the driver is executing in SMM and FALSE if the
  driver is not executing in SMM.

  @retval  TRUE  The driver is executing in System Management Mode (SMM).
  @retval  FALSE The driver is not executing in System Management Mode (SMM).

**/
BOOLEAN
EFIAPI
InSmm (
  VOID
  )
{
  return TRUE;
}
