/** @file
  This sample application that is the simplest UEFI application possible.
  It simply prints "Hello Uefi!" to the UEFI Console Out device.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle        The firmware allocated handle for the EFI image.
  @param[in] SystemTable        A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The entry point is executed successfully.
  @retval EFI_INVALID_PARAMETER SystemTable provided was not valid.
  @retval other                 Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (SystemTable == NULL || SystemTable->ConOut == NULL || SystemTable->ConOut->OutputString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SystemTable->ConOut->OutputString (SystemTable->ConOut, L"Hello Uefi!\r\n");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
