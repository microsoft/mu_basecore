/** @file
  Functional instance of EcitReportLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseCryptLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CryptoIndicatorRegistrationLib.h>
#include <Library/EcitReportLib.h>
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
  EFI_STATUS  Status;
  UINTN       PayloadSize;
  VOID        *Payload;

  if ((FeatureId == NULL) || (OpId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

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
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       *OpSizes;
  UINTN       PayloadSize;
  UINT8       *Payload;
  UINT8       *Cursor;

  if ((FeatureId == NULL) || (Ops == NULL) || (OpCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  OpSizes = AllocateZeroPool (OpCount * sizeof (UINTN));
  if (OpSizes == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Size each non-empty OID list without its trailing NUL.
  //
  PayloadSize = 0;
  for (Index = 0; Index < OpCount; Index++) {
    UINTN  OpSize;

    if (Ops[Index] == NULL) {
      continue;
    }

    OpSize = 0;
    Status = GetCryptoOpCapability (Ops[Index], NULL, &OpSize);
    if (EFI_ERROR (Status) || (OpSize <= 1)) {
      DEBUG ((DEBUG_WARN, "EcitReport: op %g unavailable or empty (%r)\n", Ops[Index], Status));
      continue;
    }

    OpSizes[Index] = OpSize;
    if (PayloadSize != 0) {
      PayloadSize += 1;             // comma separator between operations
    }

    PayloadSize += OpSize - 1;      // OID content, excluding this op's NUL
  }

  PayloadSize += 1;                 // final NUL

  Payload = AllocatePool (PayloadSize);
  if (Payload == NULL) {
    FreePool (OpSizes);
    return EFI_OUT_OF_RESOURCES;
  }

  Cursor = Payload;

  for (Index = 0; Index < OpCount; Index++) {
    CHAR8  *OpCsv;
    UINTN  OpSize;

    if (OpSizes[Index] == 0) {
      continue;
    }

    OpSize = OpSizes[Index];
    OpCsv  = AllocatePool (OpSize);
    if (OpCsv == NULL) {
      FreePool (Payload);
      FreePool (OpSizes);
      return EFI_OUT_OF_RESOURCES;
    }

    Status = GetCryptoOpCapability (Ops[Index], OpCsv, &OpSize);
    if (EFI_ERROR (Status) || (OpSize <= 1) || (OpSize > OpSizes[Index])) {
      FreePool (OpCsv);
      continue;
    }

    if (Cursor != Payload) {
      *Cursor++ = ',';
    }

    CopyMem (Cursor, OpCsv, OpSize - 1);   // append OIDs, excluding this op's NUL
    Cursor += OpSize - 1;

    FreePool (OpCsv);
  }

  *Cursor++ = '\0';

  Status = EcitRegisterCryptoCapability (FeatureId, Payload, (UINTN)(Cursor - Payload));
  DEBUG ((
    DEBUG_INFO,
    "EcitReport: feature %g reported OID list (%u byte payload) - %r\n",
    FeatureId,
    (UINT32)(Cursor - Payload),
    Status
    ));

  FreePool (Payload);
  FreePool (OpSizes);
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
