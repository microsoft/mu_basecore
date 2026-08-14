/** @file
  Functional instance of EcitReportLib.

  Reports a feature's cryptographic capabilities to the ECIT collector, either
  by querying the linked crypto provider (EcitReportCryptoOpCapability) or by
  registering a caller-supplied payload (EcitReportCapability).

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseCryptLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/CryptoIndicatorRegistrationLib.h>
#include <Library/EcitReportLib.h>

/**
  Query the linked crypto provider for the algorithms it accepts for OpId and
  register the result with the ECIT collector under FeatureId. See
  <Library/EcitReportLib.h>.
**/
EFI_STATUS
EFIAPI
EcitReportCryptoOpCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST EFI_GUID  *OpId
  )
{
  EFI_STATUS  Status;
  UINTN       PayloadSize;
  VOID        *Payload;

  if ((FeatureId == NULL) || (OpId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Probe for the payload size (Buffer == NULL returns the required size).
  //
  PayloadSize = 0;
  Status      = GetCryptoOpCapability (OpId, NULL, &PayloadSize);
  if (EFI_ERROR (Status) || (PayloadSize == 0)) {
    DEBUG ((
      DEBUG_WARN,
      "EcitReport: op %g capability unavailable (%r); nothing reported for feature %g\n",
      OpId,
      Status,
      FeatureId
      ));
    return EFI_ERROR (Status) ? Status : EFI_NOT_FOUND;
  }

  Payload = AllocatePool (PayloadSize);
  if (Payload == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GetCryptoOpCapability (OpId, Payload, &PayloadSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EcitReport: failed to read op %g capability - %r\n", OpId, Status));
    FreePool (Payload);
    return Status;
  }

  Status = EcitRegisterCryptoCapability (FeatureId, Payload, PayloadSize);
  DEBUG ((
    DEBUG_INFO,
    "EcitReport: feature %g from op %g (%u byte payload) - %r\n",
    FeatureId,
    OpId,
    (UINT32)PayloadSize,
    Status
    ));

  FreePool (Payload);
  return Status;
}

/**
  Register a caller-supplied capability payload with the ECIT collector under
  FeatureId. See <Library/EcitReportLib.h>.
**/
EFI_STATUS
EFIAPI
EcitReportCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST VOID      *Payload        OPTIONAL,
  IN UINTN           PayloadSize
  )
{
  EFI_STATUS  Status;

  if ((FeatureId == NULL) || ((Payload == NULL) && (PayloadSize != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EcitRegisterCryptoCapability (FeatureId, Payload, PayloadSize);
  DEBUG ((
    DEBUG_INFO,
    "EcitReport: feature %g (%u byte payload) - %r\n",
    FeatureId,
    (UINT32)PayloadSize,
    Status
    ));

  return Status;
}
