/** @file
  This sample application uses LocateProtocol, to print "Hello Uefi!" to the UEFI Console.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *mTextOut;

  Status = gBS->LocateProtocol (
                  &gEfiSimpleTextOutProtocolGuid,
                  NULL,
                  (VOID **)&mTextOut
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate Simple Text Out: Status(%r)\n", Status));
    return Status;
  }

  mTextOut->OutputString (mTextOut, L"Hello Uefi!\r\n");

  return EFI_SUCCESS;
}
