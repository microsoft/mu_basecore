/** @file
  Functional instance of EcitReportLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CryptoIndicatorRegistrationLib.h>
#include <Library/DebugLib.h>
#include <Library/EcitEncodingLib.h>
#include <Library/EcitReportLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/CryptoIndicatorTable.h>

/**
  Query the linked crypto provider for the algorithms it accepts for OpId and
  register the result with the ECIT collector under FeatureId.

  @param[in] FeatureId  ECIT feature GUID this capability describes.
  @param[in] OpId       Crypto operation GUID to query.

  @retval EFI_SUCCESS            The capability was registered or queued.
  @retval EFI_INVALID_PARAMETER  FeatureId or OpId is NULL.
  @retval EFI_NOT_FOUND          The provider returned an empty capability.
  @retval EFI_UNSUPPORTED        The provider does not implement capability
                                 reporting.
  @retval EFI_OUT_OF_RESOURCES   A buffer allocation failed.
  @retval Others                 The capability query or registration failed.
**/
EFI_STATUS
EFIAPI
EcitReportCryptoOpCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST EFI_GUID  *OpId
  )
{
  EFI_STATUS                Status;
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  UINTN                     CapabilityCount;
  UINTN                     PayloadSize;
  VOID                      *Payload;

  if ((FeatureId == NULL) || (OpId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Capabilities    = NULL;
  CapabilityCount = 0;
  Status          = GetCryptoOpCapability (OpId, &Capabilities, &CapabilityCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "EcitReport: op %g capability unavailable (%r); nothing reported for feature %g\n",
      OpId,
      Status,
      FeatureId
      ));
    return Status;
  }

  Status = EcitEncodeOidSet (Capabilities, CapabilityCount, &Payload, &PayloadSize);
  if (Capabilities != NULL) {
    FreePool (Capabilities);
  }

  if (EFI_ERROR (Status)) {
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
  Query several crypto operations and register their capabilities under one
  ECIT feature as a single flat OID list.

  Non-empty OID lists are joined into one comma-separated payload. Operations
  that cannot report a capability are skipped.

  @param[in] FeatureId  ECIT feature GUID this capability describes.
  @param[in] Ops        Array of OpCount crypto operation GUID pointers.
  @param[in] OpCount    Number of entries in Ops. Must be non-zero.

  @retval EFI_SUCCESS            The capability was registered or queued.
  @retval EFI_INVALID_PARAMETER  FeatureId or Ops is NULL, or OpCount is zero.
  @retval EFI_OUT_OF_RESOURCES   A buffer allocation failed.
  @retval Others                 Capability registration failed.
**/
EFI_STATUS
EFIAPI
EcitReportCryptoOpCapabilities (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST EFI_GUID  **Ops,
  IN UINTN           OpCount
  )
{
  EFI_STATUS                Status;
  EFI_STATUS                QueryStatus;
  UINTN                     Index;
  UINTN                     CapabilityIndex;
  UINTN                     TotalCapabilityCount;
  BASE_CRYPT_OP_CAPABILITY  **CapabilitySets;
  BASE_CRYPT_OP_CAPABILITY  *FlattenedCapabilities;
  UINTN                     *CapabilityCounts;
  UINTN                     PayloadSize;
  VOID                      *Payload;

  if ((FeatureId == NULL) || (Ops == NULL) || (OpCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  CapabilitySets = AllocateZeroPool (OpCount * sizeof (*CapabilitySets));
  if (CapabilitySets == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CapabilityCounts = AllocateZeroPool (OpCount * sizeof (*CapabilityCounts));
  if (CapabilityCounts == NULL) {
    FreePool (CapabilitySets);
    return EFI_OUT_OF_RESOURCES;
  }

  TotalCapabilityCount = 0;
  Status               = EFI_SUCCESS;
  for (Index = 0; Index < OpCount; Index++) {
    if (Ops[Index] == NULL) {
      continue;
    }

    QueryStatus = GetCryptoOpCapability (
                    Ops[Index],
                    &CapabilitySets[Index],
                    &CapabilityCounts[Index]
                    );
    if (EFI_ERROR (QueryStatus)) {
      DEBUG ((DEBUG_WARN, "EcitReport: op %g unavailable or empty (%r)\n", Ops[Index], QueryStatus));
      CapabilitySets[Index]   = NULL;
      CapabilityCounts[Index] = 0;
      continue;
    }

    if (CapabilityCounts[Index] > (MAX_UINTN - TotalCapabilityCount)) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    TotalCapabilityCount += CapabilityCounts[Index];
  }

  FlattenedCapabilities = NULL;
  if (!EFI_ERROR (Status) && (TotalCapabilityCount != 0)) {
    if (TotalCapabilityCount > (MAX_UINTN / sizeof (*FlattenedCapabilities))) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      FlattenedCapabilities = AllocatePool (
                                TotalCapabilityCount * sizeof (*FlattenedCapabilities)
                                );
      if (FlattenedCapabilities == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    CapabilityIndex = 0;
    for (Index = 0; Index < OpCount; Index++) {
      if (CapabilityCounts[Index] == 0) {
        continue;
      }

      CopyMem (
        &FlattenedCapabilities[CapabilityIndex],
        CapabilitySets[Index],
        CapabilityCounts[Index] * sizeof (*FlattenedCapabilities)
        );
      CapabilityIndex += CapabilityCounts[Index];
    }

    Status = EcitEncodeOidSet (
               FlattenedCapabilities,
               TotalCapabilityCount,
               &Payload,
               &PayloadSize
               );
  }

  if (FlattenedCapabilities != NULL) {
    FreePool (FlattenedCapabilities);
  }

  for (Index = 0; Index < OpCount; Index++) {
    if (CapabilitySets[Index] != NULL) {
      FreePool (CapabilitySets[Index]);
    }
  }

  FreePool (CapabilityCounts);
  FreePool (CapabilitySets);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EcitRegisterCryptoCapability (FeatureId, Payload, PayloadSize);
  DEBUG ((
    DEBUG_INFO,
    "EcitReport: feature %g reported OID list (%u byte payload) - %r\n",
    FeatureId,
    (UINT32)PayloadSize,
    Status
    ));

  FreePool (Payload);
  return Status;
}

/**
  Register a caller-supplied capability payload with the ECIT collector under
  FeatureId.

  The payload is feature-typed and opaque to this library. The registration
  library or collector copies it before this function returns.

  @param[in] FeatureId    ECIT feature GUID this capability describes.
  @param[in] Payload      Feature capability payload. May be NULL only when
                          PayloadSize is zero.
  @param[in] PayloadSize  Size of Payload in bytes.

  @retval EFI_SUCCESS            The payload was registered or queued.
  @retval EFI_INVALID_PARAMETER  FeatureId is NULL, or Payload is NULL with a
                                 non-zero PayloadSize.
  @retval Others                 Capability registration failed.
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
