/** @file
  Report a feature's cryptographic capabilities to the EFI Crypto Indicator
  Table (ECIT).

  A feature-owning library or driver reports the capabilities it implements to
  the ECIT collector, so the assembled table describes what the firmware
  actually supports per UEFI feature. Two entry points cover the two capability
  sources:

    - EcitReportCryptoOpCapability(): the capability comes from the linked crypto
      provider (queried via GetCryptoOpCapability); the caller supplies the
      feature it owns and the crypto operation that feature relies on, and the
      helper fetches and registers the accepted-algorithm payload.

    - EcitReportCapability(): the capability is known to the caller itself (for
      example, the set of EFI_SIGNATURE_LIST types a feature can evaluate); the
      caller supplies the feature and a ready-made payload.

  A Null instance of this class is a no-op, so feature owners can call these
  unconditionally and platforms opt in to ECIT reporting by resolving the class
  to the functional instance.

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
  @retval other                  A registration error from the collector.
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
  @retval other                  A registration error from the collector.
**/
EFI_STATUS
EFIAPI
EcitReportCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST VOID      *Payload        OPTIONAL,
  IN UINTN           PayloadSize
  );

#endif // ECIT_REPORT_LIB_H_
