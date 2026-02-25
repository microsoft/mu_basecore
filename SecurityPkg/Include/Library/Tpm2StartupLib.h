/** @file
  Definitions for TPM 2.0 startup and initialization

Copyright (c), Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM2_STARTUP_LIB_H_
#define TPM2_STARTUP_LIB_H_

/**
  This function initializes the TPM if required

  @retval EFI_SUCCESS       TPM successfully initialized
  @retval EFI_UNSUPPORTED   TPM is not supported
  @retval EFI_NOT_FOUND     TPM device not found
  @retval EFI_DEVICE_ERROR  Unexpected device error
**/
EFI_STATUS
EFIAPI
Tpm2StartupInit (
  VOID
  );

#endif
