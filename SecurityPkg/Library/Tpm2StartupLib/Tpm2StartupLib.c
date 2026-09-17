/** @file
  Setup and initialization of TPM 2.0

Copyright (c), Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Pi/PiMultiPhase.h>
#include <Guid/TcgEventHob.h>
#include <Guid/TpmInstance.h>
#include <Protocol/Tcg2Protocol.h>

#include <Library/Tpm2StartupLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/PcdLib.h>
#include <Library/ResetSystemLib.h>

/**
  Reports whether the platform is configured to use TPM 2.0.

  Checks PcdTpmInstanceGuid without accessing the TPM or checking error HOBs.
  This does not verify that a TPM is physically present or operational.

  @retval TRUE   A TPM 2.0 instance is selected, including platform-specific instances.
  @retval FALSE  No TPM or TPM 1.2 is selected, or the null library instance is used.
**/
BOOLEAN
EFIAPI
Tpm2StartupIsTpmSupported (
  VOID
  )
{
  if (CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceNoneGuid) ||
      CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid))
  {
    DEBUG ((DEBUG_INFO, "No TPM2 instance required!\n"));
    return FALSE;
  }

  return TRUE;
}

/**
  Requests access to the TPM 2.0 device in the current phase.

  Rejects a prior TPM error reported through gTpmErrorHobGuid before calling
  Tpm2RequestUseTpm. Device-access errors are returned unchanged.

  Precondition: Tpm2StartupIsTpmSupported must have returned TRUE.

  @retval EFI_SUCCESS       TPM access was granted.
  @retval EFI_DEVICE_ERROR  A prior phase reported a TPM error, or the device
                            access request returned EFI_DEVICE_ERROR.
  @retval other             Error returned by Tpm2RequestUseTpm.
**/
EFI_STATUS
EFIAPI
Tpm2StartupRequest (
  VOID
  )
{
  EFI_STATUS  Status;

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Prior phase reported a TPM error!\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - TPM2 access request failed! Status: %r\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This function initializes the TPM if required.

  Precondition: The caller must have successfully called Tpm2StartupRequest
  in the current phase before invoking this function.

  @param[in]  IsS3Resume     TRUE if the current boot is an S3 resume.
  @param[out] S3ErrorReport  Set to TRUE on return when the S3 SU_STATE
                             startup failed and the SU_CLEAR fallback
                             succeeded.

  @retval EFI_SUCCESS       TPM successfully initialized
  @retval EFI_DEVICE_ERROR  Unexpected device error
**/
EFI_STATUS
EFIAPI
Tpm2StartupInit (
  IN  BOOLEAN  IsS3Resume,
  OUT BOOLEAN  *S3ErrorReport
  )
{
  EFI_STATUS  Status;

  // Default the S3ErrorReport value if provided.
  if (S3ErrorReport != NULL) {
    *S3ErrorReport = FALSE;
  }

  // Determine if the init is being initiated on an S3_RESUME.
  if (IsS3Resume) {
    // Send SU_STATE to restore PCR state.
    Status = Tpm2Startup (TPM_SU_STATE);
    if (EFI_ERROR (Status)) {
      // Fallback to SU_CLEAR to initialize the TPM on failure.
      Status = Tpm2Startup (TPM_SU_CLEAR);
      if (!EFI_ERROR (Status)) {
        // If successful, indicate S3_RESUME failed but TPM initialization was successful.
        if (S3ErrorReport != NULL) {
          *S3ErrorReport = TRUE;
        }
      }
    }
  } else {
    // Initialize the TPM.
    Status = Tpm2Startup (TPM_SU_CLEAR);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tpm2Startup::%a - TPM failed Startup! Status: %r\n", __func__, Status));
  }

  return Status;
}

/**
  Ensures that the active TPM PCR banks are supported by the platform.

  Platform support is defined by PcdTcg2HashLibSupportMask, and platform
  intent is defined by PcdTpm2HashMask. A zero intent mask defaults to
  platform support. The intent must be a subset of platform support, and
  the platform and TPM must support at least one common hash algorithm.

  If no PCR banks are active, or any active bank is unsupported by the
  platform, reallocates the banks to the intersection of platform intent
  and TPM support. Otherwise, leaves the active banks unchanged.

  A successful reallocation triggers a cold reset and does not return.

  Precondition: The caller must have successfully called Tpm2StartupRequest
  in the current phase before invoking this function.

  EXAMPLE: TPM shows SHA-1, SHA-256, SHA-384, and SHA-512 active. Platform
           supports SHA-256 and SHA-384 but intent is SHA-256 only. The TPM
           would sync to SHA-256 only.
  EXAMPLE: TPM shows SHA-256 active. Platform supports SHA-256 and SHA-384.
           No sync necessary.

  @retval EFI_SUCCESS       Active PCR banks satisfy platform support; no
                            reallocation was required.
  @retval EFI_UNSUPPORTED   Platform support is empty, intent is not a subset
                            of platform support, the platform and TPM have no
                            common hash algorithm, or reallocation is required
                            but the TPM supports none of the intended banks.
  @retval EFI_DEVICE_ERROR  Querying TPM capabilities returned EFI_DEVICE_ERROR,
                            or PCR bank reallocation failed.
**/
EFI_STATUS
EFIAPI
Tpm2StartupSecuritySync (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP  TpmHashAlgorithmBitmap;
  UINT32                           TpmActivePcrBanks;
  UINT32                           SupportedHashAlgorithms;
  UINT32                           Tpm2PcrMask;

  // Acquire the active banks and TPM supported hashing algorithms.
  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashAlgorithmBitmap, &TpmActivePcrBanks);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to determine TPM capabilities!\n", __func__));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Acquire the platform intent and platform supported hashing algorithms.
  Tpm2PcrMask             = PcdGet32 (PcdTpm2HashMask);
  SupportedHashAlgorithms = PcdGet32 (PcdTcg2HashLibSupportMask);

  // If the platform indicated no intent, default to platform support.
  if (Tpm2PcrMask == 0) {
    Tpm2PcrMask = SupportedHashAlgorithms;
  }

  // The platform must support at least one hashing algorithm.
  if (SupportedHashAlgorithms == 0) {
    DEBUG ((DEBUG_ERROR, "%a - The platform must support at least one hashing algorithm\n", __func__));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  // The platform intent must be a subset of what the platform supports.
  if ((Tpm2PcrMask & ~SupportedHashAlgorithms) != 0) {
    DEBUG ((DEBUG_ERROR, "%a - The platform default must be a subset of platform support\n", __func__));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  // Confirm the TPM supports hashing algorithms the platform supports.
  if ((SupportedHashAlgorithms & TpmHashAlgorithmBitmap) == 0) {
    DEBUG ((DEBUG_ERROR, "%a - No common hash algorithm between platform and TPM\n", __func__));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  // Determine if syncing is required.
  if ((TpmActivePcrBanks == 0) || ((TpmActivePcrBanks & SupportedHashAlgorithms) != TpmActivePcrBanks)) {
    // Confirm the TPM supports the platform intent.
    Tpm2PcrMask &= TpmHashAlgorithmBitmap;
    if (Tpm2PcrMask == 0) {
      DEBUG ((DEBUG_ERROR, "%a - TPM doesn't support platform default\n", __func__));
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
    }

    DEBUG ((DEBUG_INFO, "%a - Reallocating PCR banks from 0x%X to 0x%X.\n", __func__, TpmActivePcrBanks, Tpm2PcrMask));

    // Sync to platform intent (i.e. Tpm2PcrMask).
    Status = Tpm2PcrAllocateBanks (NULL, (UINT32)TpmHashAlgorithmBitmap, Tpm2PcrMask);
    if (EFI_ERROR (Status)) {
      // We can't do much here, but we hope that this doesn't happen.
      DEBUG ((DEBUG_ERROR, "%a - Failed to reallocate PCRs!\n", __func__));
      ASSERT_EFI_ERROR (Status);
      return EFI_DEVICE_ERROR;
    }

    // Need reset system, since we just called Tpm2PcrAllocateBanks().
    ResetCold ();
  }

  return Status;
}

/**
  Synchronizes the active TPM PCR banks with the supplied platform/user intent.

  A zero HashMask leaves the active PCR banks unchanged. Otherwise, filters
  HashMask to the TPM-supported hash algorithms and requests reallocation only
  if the filtered mask differs from the active PCR bank mask.

  A successful reallocation triggers a cold reset and does not return.

  Precondition: The caller must have successfully called Tpm2StartupRequest
  in the current phase before invoking this function.

  @param[in] HashMask  The mask containing hash algorithm intent.
                       If a bit is set, the algorithm is selected. If a bit
                       is clear, the algorithm is not selected. A zero mask
                       leaves the active banks unchanged. A nonzero mask must
                       be a subset of platform support. Of those algorithms,
                       any not supported by the TPM are ignored.
                       BIT0: SHA-1
                       BIT1: SHA-256
                       BIT2: SHA-384
                       BIT3: SHA-512
                       BIT4: SM3-256

  EXAMPLE: TPM shows SHA-1, SHA-256, SHA-384, and SHA-512 active. Platform
           supports SHA-1, SHA-256, SHA-384, and SHA-512 but intent is only
           SHA-256 active. The TPM would sync to SHA-256 only.

  @retval EFI_SUCCESS            HashMask is zero, or active PCR banks already match
                                 the TPM-supported intent; no reallocation was required.
  @retval EFI_INVALID_PARAMETER  Platform support is nonempty, but HashMask is not
                                 a subset of platform support.
  @retval EFI_UNSUPPORTED        HashMask is nonzero and platform support is empty,
                                 or the TPM supports none of the requested algorithms.
  @retval EFI_DEVICE_ERROR       Querying TPM capabilities returned EFI_DEVICE_ERROR,
                                 or PCR bank reallocation failed.
**/
EFI_STATUS
EFIAPI
Tpm2StartupIntentSync (
  IN UINT32  HashMask
  )
{
  EFI_STATUS                       Status;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP  TpmHashAlgorithmBitmap;
  UINT32                           TpmActivePcrBanks;
  UINT32                           SupportedHashAlgorithms;

  // If no intent was provided, we can exit early.
  if (HashMask == 0) {
    DEBUG ((DEBUG_INFO, "%a - No intent provided, using active banks\n", __func__));
    return EFI_SUCCESS;
  }

  // Validate the intent provided is supported by the platform.
  SupportedHashAlgorithms = PcdGet32 (PcdTcg2HashLibSupportMask);

  // The platform must support at least one hashing algorithm.
  if (SupportedHashAlgorithms == 0) {
    DEBUG ((DEBUG_ERROR, "%a - The platform must support at least one hashing algorithm\n", __func__));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  // The provided intent must be a subset of what the platform supports.
  if ((HashMask & ~SupportedHashAlgorithms) != 0) {
    DEBUG ((DEBUG_ERROR, "%a - The intent provided must be a subset of platform support\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Acquire the active banks and TPM supported hashing algorithms.
  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashAlgorithmBitmap, &TpmActivePcrBanks);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to determine TPM capabilities!\n", __func__));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Determine if syncing is required.
  if (HashMask != TpmActivePcrBanks) {
    // Confirm the TPM supports the requested intent.
    HashMask &= TpmHashAlgorithmBitmap;
    if (HashMask == 0) {
      DEBUG ((DEBUG_ERROR, "%a - TPM doesn't support requested intent\n", __func__));
      return EFI_UNSUPPORTED;
    }

    // Avoid reallocating and resetting when the supported intent is already active.
    if (HashMask == TpmActivePcrBanks) {
      return EFI_SUCCESS;
    }

    DEBUG ((DEBUG_INFO, "%a - Reallocating PCR banks from 0x%X to 0x%X.\n", __func__, TpmActivePcrBanks, HashMask));

    // Sync to the provided HashMask which indicates platform/user intent.
    Status = Tpm2PcrAllocateBanks (NULL, (UINT32)TpmHashAlgorithmBitmap, HashMask);
    if (EFI_ERROR (Status)) {
      // We can't do much here, but we hope that this doesn't happen.
      DEBUG ((DEBUG_ERROR, "%a - Failed to reallocate PCRs!\n", __func__));
      ASSERT_EFI_ERROR (Status);
      return EFI_DEVICE_ERROR;
    }

    // Need reset system, since we just called Tpm2PcrAllocateBanks().
    ResetCold ();
  }

  return Status;
}
