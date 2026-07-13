/** @file
  Declarations for TPM 2.0 startup and initialization.

  A single library instance consolidates the TPM startup and pre-DXE
  measurement work that previously lived in PEI so it can be driven
  from either PEI or from SEC on PEI-less platforms.

Copyright (c), Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  Initialize the TPM 2.0 device. Requests use of the TPM, invokes
  the OEM pre-startup hook, runs TPM startup (with S3-state fallback
  to SU_CLEAR and PCR 0..7 separator error events on fallback),
  runs TPM self test, the OEM post-selftest hook, and a debug PCR read.
  On failure, produces an ERROR HOB so later phases exit early.

  @param[in]  IsS3Resume  TRUE when the platform is resuming from S3.
                          Drives TPM_SU_STATE vs TPM_SU_CLEAR selection,
                          whether self-test runs, and whether error
                          separator events are generated on PCRs 0..7.

  @retval EFI_SUCCESS       Initialization completed.
  @retval EFI_UNSUPPORTED   TPM2 is not required.
  @retval EFI_NOT_FOUND     TPM device not detected.
  @retval EFI_DEVICE_ERROR  A TPM command failed; error HOB was produced,
                            or a prior error HOB was found.
**/
EFI_STATUS
EFIAPI
Tpm2StartupInitializeTpm (
  IN BOOLEAN  IsS3Resume
  );

/**
  Generates the pre-Tcg2 core measurement events. Runs the OEM
  pre-measurement hook, seeds the pre-UEFI event log, measures the
  firmware debugger state (when enabled), and measures the CRTM version
  (when enabled). On failure, produces an ERROR HOB so later phases exit
  early.

  @retval EFI_SUCCESS       All applicable events measured.
  @retval EFI_UNSUPPORTED   TPM2 is not required.
  @retval EFI_NOT_FOUND     TPM device not detected.
  @retval EFI_DEVICE_ERROR  A TPM command failed; error HOB was produced,
                            or a prior error HOB was found.
**/
EFI_STATUS
EFIAPI
Tpm2StartupMeasureCoreEvents (
  VOID
  );

/**
  Measure a single firmware volume image into PCR 0. Handles
  excluded FV HOBs, pre-hashed FV HOBs (where digests are provided
  to prevent re-hashing), and migrated FV HOBs.

  @param[in]  FvBase    Base address of the FV image.
  @param[in]  FvLength  Length of the FV image.

  @retval EFI_SUCCESS            FV was measured or was already
                                 measured/excluded.
  @retval EFI_INVALID_PARAMETER  Malformed data.
  @retval EFI_OUT_OF_RESOURCES   Allocation failure.
  @retval EFI_DEVICE_ERROR       A TPM command failed.
**/
EFI_STATUS
EFIAPI
Tpm2StartupMeasureFvImage (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  );

/**
  Record a child FV (an FV embedded inside a file inside its parent) so
  that when the same address range is presented again during a later
  FV-notify, it is not measured a second time. The child's content was
  already covered by the parent measurement.

  @param[in]  FvBase    Base address of the child FV.
  @param[in]  FvLength  Length of the child FV.
**/
VOID
EFIAPI
Tpm2StartupRecordChildFv (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  );

/**
  Publish gMeasuredFvHobGuid containing every base and child FV that
  was measured.
**/
VOID
EFIAPI
Tpm2StartupPublishMeasuredFvHob (
  VOID
  );
