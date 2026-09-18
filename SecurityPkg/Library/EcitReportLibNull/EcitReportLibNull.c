/** @file
  Null instance of EcitReportLib.

  Does nothing and reports success, so feature owners can call the ECIT
  reporting APIs unconditionally on platforms that do not publish an EFI Crypto
  Indicator Table.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/EcitReportLib.h>

/**
  No-op. See <Library/EcitReportLib.h>.
**/
EFI_STATUS
EFIAPI
EcitReportCryptoOpCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST EFI_GUID  *OpId
  )
{
  return EFI_SUCCESS;
}

/**
  No-op. See <Library/EcitReportLib.h>.
**/
EFI_STATUS
EFIAPI
EcitReportCryptoOpCapabilities (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST EFI_GUID  **Ops,
  IN UINTN           OpCount
  )
{
  return EFI_SUCCESS;
}

/**
  No-op. See <Library/EcitReportLib.h>.
**/
EFI_STATUS
EFIAPI
EcitReportCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST VOID      *Payload        OPTIONAL,
  IN UINTN           PayloadSize
  )
{
  return EFI_SUCCESS;
}
