/** @file
  This module produces the Hash 2 Service Binding protocol and Hash 2 protocol.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// MU_CHANGE - WHOLE FILE

#include <Uefi.h>

#include <Library/Hash2CryptoLib.h>

/**
  The entry point for Hash driver which installs the service binding protocol.

  @param[in]  ImageHandle  The image handle of the driver.
  @param[in]  SystemTable  The system table.

  @retval EFI_SUCCESS      The service binding protocols is successfully installed.
  @retval Others           Other errors as indicated.

**/
EFI_STATUS
EFIAPI
Hash2DriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  return InstallHash2ServiceBindingProtocol ();
}
