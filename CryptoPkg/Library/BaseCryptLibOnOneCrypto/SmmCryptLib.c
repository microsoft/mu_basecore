/** @file
  Implements the GetCryptoServices() API that returns a pointer to the
  OneCrypto Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/OneCrypto.h>

extern ONE_CRYPTO_PROTOCOL  *gCryptoProtocol;

/**
  Constructor looks up the One Crypto Protocol and verifies that it is
  not NULL and has a high enough version value to support all the BaseCryptLib
  functions.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval  EFI_SUCCESS    The One Crypto Protocol was found.
  @retval  EFI_NOT_FOUND  The One Crypto Protocol was not found.
**/
EFI_STATUS
EFIAPI
SmmCryptLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gSmst->SmmLocateProtocol (
                    &gOneCryptoProtocolGuid,
                    NULL,
                    (VOID **)&gCryptoProtocol
                    );
  if (EFI_ERROR (Status) || (gCryptoProtocol == NULL)) {
    DEBUG ((DEBUG_ERROR, "[SmmCryptLib] Failed to locate Crypto SMM Protocol. Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (gCryptoProtocol != NULL);
    gCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
