/** @file
  Null crypto-operation capability reporting.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "InternalCryptLib.h"

/**
  Reports that this NULL library instance implements no crypto operations.

  No allocation is made. Every non-NULL output argument is cleared, including
  when another argument is NULL.

  @param[in]  OpIdGuid         GUID identifying the crypto operation.
  @param[out] Capabilities     Receives NULL.
  @param[out] CapabilityCount  Receives zero.

  @retval EFI_NOT_FOUND          No operation is implemented by this instance.
  @retval EFI_INVALID_PARAMETER  Any argument is NULL.
**/
EFI_STATUS
EFIAPI
GetCryptoOpCapability (
  IN  CONST EFI_GUID            *OpIdGuid,
  OUT BASE_CRYPT_OP_CAPABILITY  **Capabilities,
  OUT UINTN                     *CapabilityCount
  )
{
  if (Capabilities != NULL) {
    *Capabilities = NULL;
  }

  if (CapabilityCount != NULL) {
    *CapabilityCount = 0;
  }

  if ((OpIdGuid == NULL) || (Capabilities == NULL) || (CapabilityCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_NOT_FOUND;
}
