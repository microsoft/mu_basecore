/** @file -- OemTpm2InitLibVendorNull.c

MU_CHANGE

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

This is an null version of the vendor-specific lib that can be used
for TPM parts that don't require an special initialization.

**/

#include <Uefi.h>

/**
  This function will perform additional TPM initialization
  that may be require for a specific vendor part. It will be invoked
  during the DXE phase.

  @retval     EFI_SUCCESS   TPM was successfully initialized.
  @retval     Others        Something went wrong.

**/
EFI_STATUS
OemTpm2VendorSpecificInit (
  VOID
  )
{
  // For the NULL implementation, we don't need to do anything.
  return EFI_SUCCESS;
} // OemTpm2VendorSpecificInit()
