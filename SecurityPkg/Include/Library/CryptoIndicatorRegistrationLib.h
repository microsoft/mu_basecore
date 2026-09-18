/** @file
  Helper library for submitting EFI Crypto Indicator Table (ECIT) records.

  Feature-owning libraries and drivers call EcitRegisterCryptoCapability() to
  hand their cryptographic-capability record to the ECIT collector. The record
  is copied, so the caller may free its buffer afterward. If the collector's
  registration protocol is not yet available, the record is queued and submitted
  automatically when the collector appears, so callers do not need to order
  themselves after the collector.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CRYPTO_INDICATOR_REGISTRATION_LIB_H_
#define CRYPTO_INDICATOR_REGISTRATION_LIB_H_

/**
  Submit an ECIT feature record to the collector.

  If the collector is present the record is registered immediately; otherwise it
  is queued and submitted when the collector's registration protocol is
  installed. The record (EntryData) is copied.

  @param[in] FeatureIdentifier  GUID identifying the feature (see
                                <Guid/CryptoIndicatorTable.h>).
  @param[in] EntryData          Feature-typed capability payload. May be NULL
                                only when EntryDataSize is 0.
  @param[in] EntryDataSize      Size of EntryData in bytes.

  @retval EFI_SUCCESS            The record was registered or queued.
  @retval EFI_INVALID_PARAMETER  FeatureIdentifier is NULL, or EntryData is NULL
                                 with a non-zero EntryDataSize.
  @retval EFI_ACCESS_DENIED      The table has already been sealed and published.
  @retval EFI_ALREADY_STARTED    An entry for FeatureIdentifier already exists.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
EFI_STATUS
EFIAPI
EcitRegisterCryptoCapability (
  IN CONST EFI_GUID  *FeatureIdentifier,
  IN CONST VOID      *EntryData        OPTIONAL,
  IN UINTN           EntryDataSize
  );

#endif // CRYPTO_INDICATOR_REGISTRATION_LIB_H_
