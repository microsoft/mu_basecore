/** @file
  NULL instance of TPM 2.0 Startup

Copyright (c), Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/Tpm2StartupLib.h>

/**
  This function initializes the TPM if required

  @retval EFI_UNSUPPORTED   TPM is not supported
**/
EFI_STATUS
EFIAPI
Tpm2StartupInit (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
