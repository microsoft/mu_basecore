/** @file
  Definitions for TPM 2.0 startup and initialization

Copyright (c), Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM2_STARTUP_LIB_H_
#define TPM2_STARTUP_LIB_H_

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
  );

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
  );

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
  );

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
  );

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
  );

#endif
