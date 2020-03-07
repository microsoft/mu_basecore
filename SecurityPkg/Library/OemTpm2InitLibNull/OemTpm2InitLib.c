/** @file -- OemTpm2InitLib.c
Primary library init functions for TPM 2.0.

MS_CHANGE_?

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Pi/PiBootMode.h>

#include <Protocol/Tcg2Protocol.h>

//=================================================================================================
//=================================================================================================
//
//  OemTpm2InitLib LIBRARY FUNCTIONS
//
//=================================================================================================
//=================================================================================================

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
  )
{
  // Do nothing...
  return EFI_SUCCESS;
} // OemTpm2InitPeiPreStartup()


/**
  OEM init hook that is called immediately after TPM2_SelfTest() completes.

  NOTE: If this function returns an EFI_ERROR, TPM initialization WILL NOT continue.
        Make sure this is something you actually want to do.

  IMPLEMENTATION: Read the TPM Enablement NV Index from the TPM itself.
    - If enabled, allow init to continue.
    - If read fails because NV Index missing or uninitialzed:
      - If missing, create and initialize.
      - If uninitialized, set to default value (TPM ON)
    - If disabled, discontinue TPM init.

  @param[in]  BootMode      EFI_BOOT_MODE to indicate the determined boot mode.

  @retval     EFI_SUCCESS   Everything is fine. Continue with init.
  @retval     EFI_ABORTED   TPM is disabled, discontinue initialization.

**/
EFI_STATUS
EFIAPI
OemTpm2InitPeiPostSelfTest (
  IN  EFI_BOOT_MODE    BootMode
  )
{
  // Do nothing...
  return EFI_SUCCESS;
} // OemTpm2InitPeiPostSelfTest()


/**
  OEM init hook that is called immediately before initial measurements of things like
  FV_MAIN and CRTM.

  NOTE: If this function returns an EFI_ERROR, TPM initialization WILL NOT continue.
        Make sure this is something you actually want to do.

  IMPLEMENTATION: Read the TPM Enablement NV Index from the TPM itself.
    - If enabled, allow init to continue.
    - If read fails, allow init to continue (default state is TPM ON).
    - If disabled, discontinue TPM init.

  @retval     EFI_SUCCESS   Everything is fine. Continue with init.
  @retval     Others        Something has gone wrong. Do not initialize TPM any further.

**/
EFI_STATUS
EFIAPI
OemTpm2InitPeiPreMeasurements (
  VOID
  )
{
  
  // Do nothing...
  return EFI_SUCCESS;
} // OemTpm2InitPeiPreMeasurements()


/**
  OEM init hook that is called after all capabilities have been queried, but
  before any of the DXE event callbacks have been registered.

  NOTE: If this function returns an EFI_ERROR, TPM initialization WILL NOT continue.
        Make sure this is something you actually want to do.

  @param[in,out]  BsCap      Structure describing the capabilities of the existing TPM.

  @retval     EFI_SUCCESS   Everything is fine. Continue with init.
  @retval     EFI_ABORTED   TPM is disabled, discontinue initialization.

**/
EFI_STATUS
EFIAPI
OemTpm2InitDxeEntryPreRegistration (
  IN OUT EFI_TCG2_BOOT_SERVICE_CAPABILITY *BsCap
  )
{
  // Do nothing...
  return EFI_SUCCESS;
} // OemTpm2InitDxeEntryPreRegistration()


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
  )
{
  // Do nothing...
  return EFI_SUCCESS;
} // OemTpm2InitDxeReadyToBootEvent()
