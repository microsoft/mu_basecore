/** @file
  Null instance of EcitReportLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/EcitReportLib.h>

/**
  Ignore a request to report one crypto operation capability.

  @param[in] FeatureId  ECIT feature GUID this capability describes.
  @param[in] OpId       Crypto operation GUID to query.

  @retval EFI_SUCCESS  No action was performed.
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
  Ignore a request to report several crypto operation capabilities.

  @param[in] FeatureId  ECIT feature GUID this capability describes.
  @param[in] Ops        Array of OpCount crypto operation GUID pointers.
  @param[in] OpCount    Number of entries in Ops.

  @retval EFI_SUCCESS  No action was performed.
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
  Ignore a request to report a caller-supplied capability.

  @param[in] FeatureId    ECIT feature GUID this capability describes.
  @param[in] Payload      Feature capability payload.
  @param[in] PayloadSize  Size of Payload in bytes.

  @retval EFI_SUCCESS  No action was performed.
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
