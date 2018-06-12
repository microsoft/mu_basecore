/** @file -- OemTpm2InitLib.h
Oem library hooks for additional TPM 2.0 initialization.

MS_CHANGE_?

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OEM_TPM_2_INIT_LIB_H_
#define _OEM_TPM_2_INIT_LIB_H_

// In order to include this header, you will also need to include.
// - Pi/PiBootMode.h
// - Protocol/Tcg2Protocol.h

/**
  OEM init hook that is called before Tcg2 PEI attempts
  to run TPM2_Startup().

  NOTE: If this function returns an EFI_ERROR, TPM initialization WILL NOT continue.
        Make sure this is something you actually want to do.

  @param[in]  BootMode      EFI_BOOT_MODE to indicate the determined boot mode.

  @retval     EFI_SUCCESS   Everything is fine. Continue with init.
  @retval     Others        Something has gone wrong. Do not initialize TPM any further.

**/
EFI_STATUS
EFIAPI
OemTpm2InitPeiPreStartup (
  IN  EFI_BOOT_MODE    BootMode
  );


/**
  OEM init hook that is called immediately after TPM2_SelfTest() completes.
  TODO: Consider passing in the SelfTest results to this function.

  NOTE: If this function returns an EFI_ERROR, TPM initialization WILL NOT continue.
        Make sure this is something you actually want to do.

  @param[in]  BootMode      EFI_BOOT_MODE to indicate the determined boot mode.

  @retval     EFI_SUCCESS   Everything is fine. Continue with init.
  @retval     Others        Something has gone wrong. Do not initialize TPM any further.

**/
EFI_STATUS
EFIAPI
OemTpm2InitPeiPostSelfTest (
  IN  EFI_BOOT_MODE    BootMode
  );


/**
  OEM init hook that is called immediately before initial measurements of things like
  FV_MAIN and CRTM.

  NOTE: If this function returns an EFI_ERROR, TPM initialization WILL NOT continue.
        Make sure this is something you actually want to do.

  @retval     EFI_SUCCESS   Everything is fine. Continue with init.
  @retval     Others        Something has gone wrong. Do not initialize TPM any further.

**/
EFI_STATUS
EFIAPI
OemTpm2InitPeiPreMeasurements (
  VOID
  );


/**
  OEM init hook that is called after all capabilities have been queried, but
  before any of the DXE event callbacks have been registered.

  NOTE: If this function returns an EFI_ERROR, TPM initialization WILL NOT continue.
        Make sure this is something you actually want to do.

  @param[in,out]  BsCap      Structure describing the capabilities of the existing TPM.

  @retval     EFI_SUCCESS   Everything is fine. Continue with init.
  @retval     Others        Something has gone wrong. Do not initialize TPM any further.

**/
EFI_STATUS
EFIAPI
OemTpm2InitDxeEntryPreRegistration (
  IN OUT EFI_TCG2_BOOT_SERVICE_CAPABILITY *BsCap
  );


/**
  OEM init hook that is called during the Tcg2Dxe ReadyToBoot event. It is called before internal
  Tcg2Dxe initialization, most of which is ReadyToBoot measurements.

  NOTE: If this function returns an EFI_ERROR, TPM initialization WILL NOT continue.
        Make sure this is something you actually want to do.

  @param[in]  BootAttemptCount  Number of ReadyToBoot events that have occured.
                                0 indicates that this is the first ReadyToBoot event and
                                is where most of any custom initialization should occur.         

  @retval     EFI_SUCCESS   Everything is fine. Continue with init.
  @retval     Others        Something has gone wrong. Do not initialize TPM any further.

**/
EFI_STATUS
EFIAPI
OemTpm2InitDxeReadyToBootEvent (
  IN UINTN  BootAttemptCount
  );


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
  );

#endif // _OEM_TPM_2_INIT_LIB_H_
