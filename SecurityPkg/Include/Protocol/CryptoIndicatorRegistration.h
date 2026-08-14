/** @file
  EFI Crypto Indicator Table (ECIT) registration protocol.

  Produced by the ECIT collector driver. Libraries and drivers that own a UEFI
  feature use this protocol to submit their feature's cryptographic-capability
  record; the collector accumulates the records and, at ready-to-boot, assembles
  and publishes the EFI_CRYPTO_INDICATOR_TABLE (see <Guid/CryptoIndicatorTable.h>).

  Contributors should register during DXE (a DEPEX on this protocol GUID, or the
  CryptoIndicatorRegistrationLib helper, guarantees the collector is present).
  Once the table has been sealed and published, RegisterEntry() returns
  EFI_ACCESS_DENIED.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CRYPTO_INDICATOR_REGISTRATION_PROTOCOL_H_
#define CRYPTO_INDICATOR_REGISTRATION_PROTOCOL_H_

//
// Project-defined protocol GUID (not UEFI-spec defined).
// {2b9a1f74-3c6e-4d18-9a2f-d70e4c118355}
//
#define EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL_GUID \
  { 0x2b9a1f74, 0x3c6e, 0x4d18, { 0x9a, 0x2f, 0xd7, 0x0e, 0x4c, 0x11, 0x83, 0x55 } }

typedef struct _EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL;

#define EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL_REVISION  0x00010000

/**
  Register one ECIT entry with the collector.

  The collector makes its own copy of EntryData, so the caller may free or reuse
  the buffer after this call returns. FeatureIdentifier must be unique across the
  table (the ECIT spec requires one entry per feature GUID).

  @param[in] This               Pointer to this protocol instance.
  @param[in] FeatureIdentifier  GUID identifying the feature this record
                                describes (see <Guid/CryptoIndicatorTable.h> for
                                the well-known feature GUIDs and their EntryData
                                layouts).
  @param[in] EntryData          Feature-typed capability payload. May be NULL
                                only when EntryDataSize is 0.
  @param[in] EntryDataSize      Size of EntryData in bytes.

  @retval EFI_SUCCESS            The entry was accepted.
  @retval EFI_INVALID_PARAMETER  This or FeatureIdentifier is NULL, or EntryData
                                 is NULL with a non-zero EntryDataSize, or the
                                 resulting entry would overflow the table.
  @retval EFI_ALREADY_STARTED    An entry for FeatureIdentifier is already
                                 registered.
  @retval EFI_ACCESS_DENIED      The table has already been sealed and published.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CRYPTO_INDICATOR_REGISTER_ENTRY)(
  IN EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL  *This,
  IN CONST EFI_GUID                              *FeatureIdentifier,
  IN CONST VOID                                  *EntryData        OPTIONAL,
  IN UINTN                                       EntryDataSize
  );

struct _EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL {
  UINT32                                 Revision;
  EFI_CRYPTO_INDICATOR_REGISTER_ENTRY    RegisterEntry;
};

extern EFI_GUID  gEfiCryptoIndicatorRegistrationProtocolGuid;

#endif // CRYPTO_INDICATOR_REGISTRATION_PROTOCOL_H_
