/** @file
  Null instance of CryptoIndicatorRegistrationLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/CryptoIndicatorRegistrationLib.h>

/**
  Ignore a request to report a caller-supplied capability.

  @param[in] FeatureId    ECIT feature GUID this capability describes.
  @param[in] Payload      Feature capability payload.
  @param[in] PayloadSize  Size of Payload in bytes.

  @retval EFI_SUCCESS  No action was performed.
**/
EFI_STATUS
EFIAPI
EcitRegisterCryptoCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST VOID      *Payload        OPTIONAL,
  IN UINTN           PayloadSize
  )
{
  return EFI_SUCCESS;
}
