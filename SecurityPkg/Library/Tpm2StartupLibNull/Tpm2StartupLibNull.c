/** @file
  NULL instance of Tpm2StartupLib. All entry points are no-ops that
  return EFI_SUCCESS, for platforms that do not need TPM 2.0 startup
  work in a given phase.

Copyright (c), Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/Tpm2StartupLib.h>

/**
  Null. See Tpm2StartupLib.h.
**/
EFI_STATUS
EFIAPI
Tpm2StartupInitializeTpm (
  IN BOOLEAN  IsS3Resume
  )
{
  (VOID)IsS3Resume;
  return EFI_SUCCESS;
}

/**
  Null. See Tpm2StartupLib.h.
**/
EFI_STATUS
EFIAPI
Tpm2StartupMeasureCoreEvents (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Null. See Tpm2StartupLib.h.
**/
EFI_STATUS
EFIAPI
Tpm2StartupMeasureFvImage (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  (VOID)FvBase;
  (VOID)FvLength;
  return EFI_SUCCESS;
}

/**
  Null. See Tpm2StartupLib.h.
**/
VOID
EFIAPI
Tpm2StartupRecordChildFv (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  (VOID)FvBase;
  (VOID)FvLength;
}

/**
  Null. See Tpm2StartupLib.h.
**/
VOID
EFIAPI
Tpm2StartupPublishMeasuredFvHob (
  VOID
  )
{
}
