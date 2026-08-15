/** @file
  Library for reporting EFI Crypto Indicator Table capabilities.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ECIT_REPORT_LIB_H_
#define ECIT_REPORT_LIB_H_

/**
  Query the linked crypto provider for the algorithms it accepts for OpId and
  register the result with the ECIT collector under FeatureId.

  @param[in] FeatureId  ECIT feature GUID this capability describes (see
                        <Guid/CryptoIndicatorTable.h>).
  @param[in] OpId       Crypto-operation GUID to query (see
                        <Guid/CryptoOpId.h>).

  @retval EFI_SUCCESS            The capability was registered or queued (or the
                                 Null instance did nothing).
  @retval EFI_INVALID_PARAMETER  FeatureId or OpId is NULL.
  @retval EFI_NOT_FOUND          The provider does not recognize OpId.
  @retval EFI_UNSUPPORTED        The provider does not implement capability
                                 reporting.
  @retval EFI_OUT_OF_RESOURCES   A buffer allocation failed.
  @retval Others                A registration error from the collector.
**/
EFI_STATUS
EFIAPI
EcitReportCryptoOpCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST EFI_GUID  *OpId
  );

/**
  Register a caller-supplied capability payload with the ECIT collector under
  FeatureId.

  Use this when the capability is known to the feature owner itself rather than
  queried from the crypto provider (for example, the set of EFI_SIGNATURE_LIST
  type GUIDs a feature can evaluate). The payload is feature-typed and opaque to
  this library; the collector copies it.

  @param[in] FeatureId    ECIT feature GUID this capability describes (see
                          <Guid/CryptoIndicatorTable.h>).
  @param[in] Payload      Feature-typed capability payload. May be NULL only
                          when PayloadSize is 0.
  @param[in] PayloadSize  Size of Payload in bytes.

  @retval EFI_SUCCESS            The payload was registered or queued (or the
                                 Null instance did nothing).
  @retval EFI_INVALID_PARAMETER  FeatureId is NULL, or Payload is NULL with a
                                 non-zero PayloadSize.
  @retval Others                A registration error from the collector.
**/
EFI_STATUS
EFIAPI
EcitReportCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST VOID      *Payload        OPTIONAL,
  IN UINTN           PayloadSize
  );

/**
  Register the combined capabilities of several crypto operations.

  Non-empty OID lists are joined into one comma-separated payload. Operations
  that cannot report a capability are skipped.

  @param[in] FeatureId  ECIT feature GUID this capability describes.
  @param[in] Ops        Array of OpCount crypto-operation GUID pointers.
  @param[in] OpCount    Number of entries in Ops. Must be non-zero.

  @retval EFI_SUCCESS            The capability was registered or queued (or the
                                 Null instance did nothing).
  @retval EFI_INVALID_PARAMETER  FeatureId or Ops is NULL, or OpCount is 0.
  @retval EFI_OUT_OF_RESOURCES   A buffer allocation failed.
  @retval Others                A registration error from the collector.
**/
EFI_STATUS
EFIAPI
EcitReportCryptoOpCapabilities (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST EFI_GUID  **Ops,
  IN UINTN           OpCount
  );

#endif // ECIT_REPORT_LIB_H_
