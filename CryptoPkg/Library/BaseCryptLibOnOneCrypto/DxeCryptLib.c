/** @file
  Locates and sets the global gCryptoProtocol pointer to the OneCrypto DXE Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/OneCrypto.h>

extern ONE_CRYPTO_PROTOCOL  *gCryptoProtocol;

/**
  Locate the valid Crypto Protocol.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor executed correctly.
  @retval EFI_NOT_FOUND Found no valid Crypto Protocol.
**/
EFI_STATUS
EFIAPI
DxeCryptLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  &gOneCryptoProtocolGuid,
                  NULL,
                  (VOID **)&gCryptoProtocol
                  );

  if (EFI_ERROR (Status) || (gCryptoProtocol == NULL)) {
    DEBUG ((DEBUG_ERROR, "[DxeCryptLib] Failed to locate Crypto Protocol. Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (gCryptoProtocol != NULL);
    gCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
