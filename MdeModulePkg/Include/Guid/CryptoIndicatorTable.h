/** @file
  EFI Crypto Indicator Table definitions.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  - UEFI Code First: Crypto Indicator Table
    https://github.com/tianocore/edk2/issues/12408
**/

#ifndef CRYPTO_INDICATOR_TABLE_H_
#define CRYPTO_INDICATOR_TABLE_H_

//
// EFI_CONFIGURATION_TABLE GUID for the ECIT.
// {1768b8b1-1605-401a-bc49-d612d2b98c4e}
//
#define EFI_CRYPTO_INDICATOR_TABLE_GUID \
  { 0x1768b8b1, 0x1605, 0x401a, { 0xbc, 0x49, 0xd6, 0x12, 0xd2, 0xb9, 0x8c, 0x4e } }

//
// ACPI-style signature ("ECIT") and table version.
//
#define EFI_CRYPTO_INDICATOR_TABLE_SIGNATURE  SIGNATURE_32 ('E', 'C', 'I', 'T')
#define EFI_CRYPTO_INDICATOR_TABLE_VERSION    1

#pragma pack(1)

///
/// ECIT header. The leading fields use the common ACPI SDT header layout.
///
typedef struct {
  UINT8     Signature[4];      ///< "ECIT".
  UINT32    Length;            ///< Length of the entire table (header + all entries), in bytes.
  UINT8     Version;           ///< EFI_CRYPTO_INDICATOR_TABLE_VERSION.
  UINT8     Checksum;          ///< 8-bit checksum: the whole table must sum to zero.
  UINT8     OemId[6];          ///< ACPI OEM ID.
  UINT8     OemTableId[8];     ///< ACPI OEM Table ID.
  UINT32    OemRevision;       ///< ACPI OEM revision.
  UINT32    CreatorId;         ///< ACPI creator ID.
  UINT32    CreatorRevision;   ///< ACPI creator revision.
  //
  // ECIT-specific fields.
  //
  UINT8     NumberOfEntries;   ///< Number of EFI_CRYPTO_INDICATOR_ENTRY records that follow.
  UINT8     Reserved[3];       ///< Reserved for future use; must be zero.
  // EFI_CRYPTO_INDICATOR_ENTRY  Entries[];
} EFI_CRYPTO_INDICATOR_TABLE;

///
/// A single ECIT entry. EntryData is opaque to the collector and is typed by
/// FeatureIdentifier (see the EFI_CIE_DATA_* structures below).
///
typedef struct {
  EFI_GUID    FeatureIdentifier;  ///< The feature this entry describes.
  UINT16      EntryLength;        ///< sizeof (EFI_CRYPTO_INDICATOR_ENTRY) + sizeof (EntryData).
  UINT8       Reserved[6];        ///< Reserved for future use; must be zero.
  // UINT8    EntryData[];
} EFI_CRYPTO_INDICATOR_ENTRY;

#pragma pack()

//
// Well-known feature identifiers and their EntryData types.
//

///
/// Secure Boot Image Verification. EntryData is a CHAR8 CSV OID string of the
/// signature and digest algorithms accepted when verifying images.
/// {08324cfc-efe6-4211-a858-d4cac8915aef}
///
#define EFI_ECIT_FEATURE_SB_IMAGE_VERIFICATION_GUID \
  { 0x08324cfc, 0xefe6, 0x4211, { 0xa8, 0x58, 0xd4, 0xca, 0xc8, 0x91, 0x5a, 0xef } }

///
/// Secure Boot Image Authorization. EntryData is an array of EFI_SIGNATURE_LIST
/// type GUIDs accepted from the authorized signature database (db).
/// {335f880f-180f-43d9-8ed9-ce584ed9b6f0}
///
#define EFI_ECIT_FEATURE_SB_IMAGE_AUTHORIZATION_GUID \
  { 0x335f880f, 0x180f, 0x43d9, { 0x8e, 0xd9, 0xce, 0x58, 0x4e, 0xd9, 0xb6, 0xf0 } }

///
/// Secure Boot Database Update Authorization. EntryData is an array of
/// EFI_SIGNATURE_LIST type GUIDs accepted when authorizing db and dbx updates.
/// {304b3849-4906-40ea-8ade-751d6da7d4f9}
///
#define EFI_ECIT_FEATURE_SB_DATABASE_UPDATE_AUTHORIZATION_GUID \
  { 0x304b3849, 0x4906, 0x40ea, { 0x8a, 0xde, 0x75, 0x1d, 0x6d, 0xa7, 0xd4, 0xf9 } }

///
/// Secure Boot Image Revocation. EntryData is an array of EFI_SIGNATURE_LIST
/// type GUIDs supported in the revocation (dbx) database.
/// {02913331-2f71-43db-8277-7be88ecc651c}
///
#define EFI_ECIT_FEATURE_SB_IMAGE_REVOCATION_GUID \
  { 0x02913331, 0x2f71, 0x43db, { 0x82, 0x77, 0x7b, 0xe8, 0x8e, 0xcc, 0x65, 0x1c } }

///
/// Secure Boot Database Update Verification. EntryData is a CHAR8 CSV OID
/// string of the signature algorithms accepted for authenticated updates.
/// {03092d2c-9a52-4c5c-8bf5-eaf04f45229d}
///
#define EFI_ECIT_FEATURE_SB_DATABASE_UPDATE_VERIFICATION_GUID \
  { 0x03092d2c, 0x9a52, 0x4c5c, { 0x8b, 0xf5, 0xea, 0xf0, 0x4f, 0x45, 0x22, 0x9d } }

///
/// System Firmware Update. EntryData is a CHAR8 CSV OID string.
/// {8417f337-8e42-4657-aeae-9b21a4b90258}
///
#define EFI_ECIT_FEATURE_SYSTEM_FIRMWARE_UPDATE_GUID \
  { 0x8417f337, 0x8e42, 0x4657, { 0xae, 0xae, 0x9b, 0x21, 0xa4, 0xb9, 0x02, 0x58 } }

///
/// ESRT Device Firmware Update. EntryData pairs an ESRT GUID with a supported
/// algorithm identifier.
/// {41c7bd17-6bd4-4df5-aaad-8987164ead4c}
///
#define EFI_ECIT_FEATURE_ESRT_FIRMWARE_UPDATE_GUID \
  { 0x41c7bd17, 0x6bd4, 0x4df5, { 0xaa, 0xad, 0x89, 0x87, 0x16, 0x4e, 0xad, 0x4c } }

#pragma pack(1)
typedef struct {
  EFI_GUID    EsrtGuid;             ///< The ESRT entry this record applies to.
  UINT16      SupportedAlgorithm;   ///< Supported algorithm identifier.
} EFI_CIE_DATA_ESRT_ENTRY;
#pragma pack()

extern EFI_GUID  gEfiCryptoIndicatorTableGuid;
extern EFI_GUID  gEfiEcitFeatureSbImageVerificationGuid;
extern EFI_GUID  gEfiEcitFeatureSbImageAuthorizationGuid;
extern EFI_GUID  gEfiEcitFeatureSbDatabaseUpdateAuthorizationGuid;
extern EFI_GUID  gEfiEcitFeatureSbImageRevocationGuid;
extern EFI_GUID  gEfiEcitFeatureSbDatabaseUpdateVerificationGuid;
extern EFI_GUID  gEfiEcitFeatureSystemFirmwareUpdateGuid;
extern EFI_GUID  gEfiEcitFeatureEsrtFirmwareUpdateGuid;

#endif // CRYPTO_INDICATOR_TABLE_H_
