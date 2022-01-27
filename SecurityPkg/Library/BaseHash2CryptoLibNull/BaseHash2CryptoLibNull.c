/** @file
  Null instance of Hash 2 Crypto Library.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/Hash2CryptoLib.h>

// MU_CHANGE - NEW FILE

/**
  Installs an instance of the Hash 2 Service Binding protocol.

  @retval EFI_SUCCESS           The Hash 2 Service Binding protocol was installed successfully.
  @retval EFI_UNSUPPORTED       The given implementation does not support installing the Hash 2
                                Service Binding protocol.
  @retval EFI_OUT_OF_RESOURCES  Insufficient resources to allocate memory to install the protocol.
  @retval Others                An error occurred installing the Hash 2 Service Binding protocol.

**/
EFI_STATUS
EFIAPI
InstallHash2ServiceBindingProtocol (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
