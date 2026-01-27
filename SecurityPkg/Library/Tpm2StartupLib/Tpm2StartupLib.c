/** @file
  Setup and initialization of TPM 2.0

Copyright (c), Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Pi/PiMultiPhase.h>
#include <Guid/TcgEventHob.h>
#include <Guid/TpmInstance.h>

#include <Library/Tpm2StartupLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/ReportStatusCodeLib.h>

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
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - Entry\n", __func__));

  if (CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceNoneGuid) ||
      CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid))
  {
    DEBUG ((DEBUG_INFO, "No TPM2 instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  // Request use of the TPM
  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TPM2 not detected! Status: %r\n", Status));
    goto Done;
  }

  // Determine if initialization is requested
  if (PcdGet8 (PcdTpm2InitializationPolicy) == 1) {
    // Setup/Initialize the TPM
    Status = Tpm2Startup (TPM_SU_CLEAR);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Tpm2Startup::%a - TPM failed Startup! Status: %r\n", __func__, Status));
    }
  }

Done:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TPM2 error! Building Hob.\n"));
    BuildGuidHob (&gTpmErrorHobGuid, 0);
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
      );
  }

  DEBUG ((DEBUG_INFO, "%a - Exit\n", __func__));

  return Status;
}
