/** @file
  Image Secure Boot Verification Result Table (IVRT) layout and structure definitions.

  This configuration table records the outcome of DXE Secure Boot image
  verification for every image that runs the full verification path, both
  approved and rejected. This table fully describes the image and it's
  evaluated signature(s) so that a consumer can determine the reason for
  approval or rejection.

  The table is a three-tier, self-describing structure:

    EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE
      +-- EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT
      |     ImageStatus
      |     ImageDigestAlgorithm
      |     Name[]
      |     DevicePath
      |     +-- EFI_SIGNATURE_VERIFICATION_RESULT (per evaluated WIN_CERTIFICATE)
      |          +-- Signature Index
      |          +-- Verification Status
      |          +-- ThumbprintAlgorithm
      |          +-- Thumbprint[]
      |     +-- EFI_SIGNATURE_VERIFICATION_RESULT
      |     +-- ...
      +-- EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT
      +-- ...

    Each entry (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT) describes one image that was
    evaluated. Each entry will always contain the approval status of the image
    and the device path. The name will be present if available, or NULL. If the
    image status is a digest-based approval or rejection, the ImageDigestAlgorithm
    field will contain the matching algorithm. Finally, each entry will contain
    zero or more EFI_SIGNATURE_VERIFICATION_RESULT records. Each one represents
    the evaluation of a single WIN_CERTIFICATE in the image's certificate table.
    Note that the number of entries is not indicitive of the number of
    signatures in the image due to how the image is processed.

    Each EFI_SIGNATURE_VERIFICATION_RESULT record describes the outcome of evaluating
    a single WIN_CERTIFICATE. The Signature Index matches the index of the WIN_CERTIFICATE
    in the image's certificate table while the Status field describes the outcome of the
    evaluation. The thumbprint is the TBS-cert hash of the certificate that drove the
    approval or rejection decision from either the DB or DBX. If no certificate in the
    signer's chain matched a DB entry, the thumbprint is not present and the
    ThumbprintAlgorithm is the zero GUID.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_H_
#define EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_H_

#include <Uefi.h>

///
/// GUID that identifies the Image Secure Boot Verification Result Table (IVRT)
/// when it is installed as a UEFI configuration table (gST->ConfigurationTable).
///
#define EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_GUID \
  { 0x5ee66bd4, 0x895d, 0x421d, { 0x80, 0x05, 0x3a, 0x0c, 0x6b, 0x55, 0x18, 0xa1 } }

extern EFI_GUID  gEfiImageSecureBootVerificationResultTableGuid;

///
/// Table signature ('IVRT') and current layout version.
///
#define EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE  SIGNATURE_32 ('I', 'V', 'R', 'T')
#define EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION    0x00010000

// ***************************************************************************
// Per-image status
// ***************************************************************************

#define EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_DIGEST     0x00000001 // Image digest is in db and not revoked by dbx.
#define EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY  0x00000002 // A signature chained to an unrevoked authority in the db.

//
// Rejections (bit 31 set).
//
#define EFI_IMAGE_VERIFICATION_STATUS_REJECTED_MALFORMED     0x80000001 // The image PE/COFF headers could not be parsed.
#define EFI_IMAGE_VERIFICATION_STATUS_REJECTED_BY_DIGEST     0x80000002 // The image digest is in the dbx.
#define EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY  0x80000003 // No signature chained to an unrevoked db authority.

// ***************************************************************************
// Per-signature status
// ***************************************************************************

///
/// Outcome for a single evaluated WIN_CERTIFICATE.
///
/// A signer's chain is the set of certificates from the signer to the root CA.
/// A certificate found in db is treated as the trust anchor for that chain; only
/// certificates in the chain between the signer and that anchor are checked
/// against dbx to decide whether the signature is authorized or revoked.
///
///                 | Full X.509 certificate           | Precomputed TBS-cert hash
///   --------------+----------------------------------+------------------------------
///     db (allow)  | CERTIFICATE_AUTHORITY            | TBS_HASH_AUTHORITY
///    dbx (revoke) | AUTHORITY_REVOKED_BY_CERTIFICATE | AUTHORITY_REVOKED_BY_TBS_HASH
///

#define EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY  0x00000001 // A certificate in the signer's chain was found in the db and was not revoked by the dbx.
#define EFI_SIGNATURE_VERIFICATION_TBS_HASH_AUTHORITY     0x00000002 // A db TBS-cert hash derived from a certificate in the signer's chain was found in the db and was not revoked by the dbx.

//
// Rejections (bit 31 set).
//
#define EFI_SIGNATURE_VERIFICATION_MALFORMED                         0x80000000 // Payload unparseable or hash unsupported.
#define EFI_SIGNATURE_VERIFICATION_AUTHORITY_REVOKED_BY_CERTIFICATE  0x80000001 // db authority existed, but a cert between the signer and that authority was in the dbx.
#define EFI_SIGNATURE_VERIFICATION_AUTHORITY_REVOKED_BY_TBS_HASH     0x80000002 // db authority existed, but a TBS-cert hash derived from a cert between the signer and that authority was in the dbx.
#define EFI_SIGNATURE_VERIFICATION_REJECTED_NO_AUTHORITY             0x80000003 // No db authority in the signer's chain.

// ***************************************************************************
// Structures
// ***************************************************************************

///
/// Result for one evaluated WIN_CERTIFICATE within an image.
///
/// When the status code is one of the following, the contents of `Thumbprint` is as follows:
///   - CERTIFICATE_AUTHORITY: TBS hash of the certificate in the signer's chain that matched a db entry.
///   - TBS_HASH_AUTHORITY: TBS hash found in the db that was derived from a certificate in the signer's chain.
///   - AUTHORITY_REVOKED_BY_CERTIFICATE: TBS hash of the certificate in the signer's chain between the signer and the authority that was found in the dbx.
///   - AUTHORITY_REVOKED_BY_TBS_HASH: TBS hash found in the dbx that was derived from a certificate in the signer's chain between the signer and the authority that was found in the db.
///   - REJECTED_NO_AUTHORITY: No certificate or TBS hash derived from a certificate in the signer's chain was found in the db, so no thumbprint is present.
///   - MALFORMED: none; algorithm is the zero GUID.
///
typedef struct {
  UINT32      Length;                // Total size of this record, including the trailing thumbprint.
  UINT32      SignatureIndex;        // 0-based ordinal of this WIN_CERTIFICATE within the image.
  UINT32      Status;                // EFI_SIGNATURE_VERIFICATION_* outcome for this WIN_CERTIFICATE.
  EFI_GUID    ThumbprintAlgorithm;   // Digest algorithm of the trailing thumbprint (zero GUID if none).
  // UINT8    Thumbprint[];          // TBS-cert hash of the decisive certificate (see notes above).
} EFI_SIGNATURE_VERIFICATION_RESULT;

///
/// Overall result for one image.
///
/// ImageStatus reflects if the image was approved or rejected, and the generic
/// reason for that decision (See EFI_IMAGE_VERIFICATION_STATUS_*).
///
/// If the image was approved or rejected by a digest match, ImageDigestAlgorithm
/// contains the algorithm used to compute the image's hash that matched either
/// the DB or DBX. Otherwise, ImageDigestAlgorithm is the zero GUID.
///
/// The payload that trails the fixed header is, in order:
///   1. Name          - NameLength bytes. A NUL-terminated CHAR16 string, or absent
///                      (NameLength == 0) when no user-friendly name was available.
///   2. DevicePath    - DevicePathLength bytes. The self-terminating device path of
///                      the image. Always present (DevicePathLength != 0).
///   3. Signatures[]  - NumberOfSignatures self-sized EFI_SIGNATURE_VERIFICATION_RESULT
///                      records, one per evaluated WIN_CERTIFICATE.
///
/// The Signatures[] array begins at
/// (UINT8 *)Record + sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT) + NameLength + DevicePathLength.
///
typedef struct {
  UINT32      Length;                // Total size of this record, including Name, DevicePath, Signatures[].
  UINT32      ImageStatus;           // EFI_IMAGE_VERIFICATION_STATUS_*.
  UINT32      NumberOfSignatures;    // Count of trailing EFI_SIGNATURE_VERIFICATION_RESULT records.
  UINT32      NameLength;            // Size of the trailing Name in bytes (including NUL), or 0 if absent.
  UINT32      DevicePathLength;      // Size of the trailing DevicePath in bytes (including end-of-path node).
  EFI_GUID    ImageDigestAlgorithm;  // Matching algorithm for *_IMAGE_DIGEST outcomes; zero GUID otherwise.
  // CHAR16                             Name[];        // NameLength bytes.
  // EFI_DEVICE_PATH_PROTOCOL           DevicePath;    // DevicePathLength bytes.
  // EFI_SIGNATURE_VERIFICATION_RESULT  Signatures[];  // NumberOfSignatures self-sized records.
} EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT;

///
/// The configuration table.
///
/// A fixed header followed by a variable number of EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT
/// records (one per evaluated image).
///
typedef struct {
  UINT32    Signature;               // EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE ('IVRT').
  UINT32    Version;                 // EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION.
  UINT32    Length;                  // Total table size in bytes, including all image records.
  UINT32    NumberOfImages;          // Count of EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT records that follow.
  // EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT  Images[];  // NumberOfImages records, each self-sized.
} EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE;

#endif // EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_H_
