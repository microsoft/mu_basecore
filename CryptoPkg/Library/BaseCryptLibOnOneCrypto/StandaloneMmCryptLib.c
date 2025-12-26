/** @file
  Implements the GetCryptoServices() API that returns a pointer to the
  OneCrypto Standalone MM Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/OneCrypto.h>

extern ONE_CRYPTO_PROTOCOL  *gCryptoProtocol;

/**
  Constructor looks up the One Crypto Protocol and verifies that it is
  not NULL and has a high enough version value to support all the BaseCryptLib
  functions.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  MmSystemTable A pointer to the MM System Table.

  @retval  EFI_SUCCESS    The One Crypto Protocol was found.
  @retval  EFI_NOT_FOUND  The One Crypto Protocol was not found.
**/
EFI_STATUS
EFIAPI
StandaloneMmCryptLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS  Status;

  Status = gMmst->MmLocateProtocol (
                    &gOneCryptoProtocolGuid,
                    NULL,
                    (VOID **)&gCryptoProtocol
                    );
  if (EFI_ERROR (Status) || (gCryptoProtocol == NULL)) {
    DEBUG ((DEBUG_ERROR, "[StandaloneMmCryptLib] Failed to locate Crypto SMM Protocol. Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (gCryptoProtocol != NULL);
    gCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
