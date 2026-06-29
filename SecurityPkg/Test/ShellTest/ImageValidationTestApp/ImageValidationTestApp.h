/** @file
  Header for the Image Validation unit test application.

  Defines the data structures and helpers used to drive high level secure boot image
  validation scenarios. Each scenario points the GetVariable() hook at a `db` / `dbx`
  signature database, selects one of the built-in PE/COFF images, calls gBS->LoadImage(),
  and asserts the EFI_STATUS LoadImage() returns.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef IMAGE_VALIDATION_TEST_APP_H_
#define IMAGE_VALIDATION_TEST_APP_H_

#include <Uefi.h>
#include <Library/UnitTestLib.h>

///
/// Selects which built-in PE/COFF image a scenario loads. The scenario runner converts this
/// into the corresponding image buffer and size.
///
typedef enum {
  IMAGE_TYPE_UNSIGNED,                          ///< A valid but unsigned PE/COFF image.
  IMAGE_TYPE_SIGNED,                            ///< An Authenticode-signed image (one signature) PE/COFF image.
  IMAGE_TYPE_MULTI_SIGNED,                      ///< A PE/COFF image with two Authenticode signatures (multi-signed).
  IMAGE_TYPE_SIGNED_CORRUPT_CERT,               ///< Signed image with its single WIN_CERTIFICATE corrupted.
  IMAGE_TYPE_MULTI_SIGNED_CORRUPT_FIRST_CERT,   ///< Multi-signed image with its first (signer 1) WIN_CERTIFICATE corrupted.
  IMAGE_TYPE_MULTI_SIGNED_CORRUPT_SECOND_CERT,  ///< Multi-signed image with its second (signer 2) WIN_CERTIFICATE corrupted.
  IMAGE_TYPE_SIGNED_TAMPERED,                   ///< Signed image whose body was modified after signing, so its Authenticode hash no longer matches.
  IMAGE_TYPE_MAX
} IMAGE_TYPE;

///
/// Bit flags describing the contents of a `db` or `dbx` signature database. The scenario
/// runner builds a serialized set of EFI_SIGNATURE_LISTs from these flags. Flags may be
/// OR-combined, including mixing hash algorithms, so one database can hold (for example)
/// both a SHA-256 and a SHA-512 entry.
///
#define DB_STATE_EMPTY  0x00000000                                 ///< No signature lists (absent variable).

//
// Image Authenticode digest (EFI_CERT_SHA{256,384,512}_GUID).
//
#define DB_STATE_IMAGE_DIGEST_SHA256  0x00000001                   ///< Loaded image's SHA-256 Authenticode digest.
#define DB_STATE_IMAGE_DIGEST_SHA384  0x00000002                   ///< Loaded image's SHA-384 Authenticode digest.
#define DB_STATE_IMAGE_DIGEST_SHA512  0x00000004                   ///< Loaded image's SHA-512 Authenticode digest.

//
// X.509 certificates (EFI_CERT_X509_GUID); algorithm-independent.
//
#define DB_STATE_SIGNER1_LEAF_CERT           0x00000008            ///< Signer 1 leaf certificate.
#define DB_STATE_SIGNER1_INTERMEDIATE1_CERT  0x00000010            ///< Signer 1 intermediate 1 (root-signed) CA certificate.
#define DB_STATE_SIGNER1_INTERMEDIATE2_CERT  0x00000020            ///< Signer 1 intermediate 2 (leaf-signing) CA certificate.
#define DB_STATE_SIGNER1_ROOT_CERT           0x00000040            ///< Signer 1 root CA certificate.
#define DB_STATE_SIGNER2_CERT                0x00000080            ///< Signer 2 certificate (multi-signed only).
#define DB_STATE_SIGNER2_UNRELATED_CERT      0x00000100            ///< Cert embedded in signer 2's signature but unrelated to its chain (multi-signed only).

//
// Certificate TBSCertificate hashes (EFI_CERT_X509_SHA{256,384,512}_GUID).
//
#define DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA256           0x00000200 ///< Signer 1 leaf TBS SHA-256 hash.
#define DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA384           0x00000400 ///< Signer 1 leaf TBS SHA-384 hash.
#define DB_STATE_SIGNER1_LEAF_TBS_HASH_SHA512           0x00000800 ///< Signer 1 leaf TBS SHA-512 hash.
#define DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA256  0x00001000 ///< Signer 1 intermediate 1 TBS SHA-256 hash.
#define DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA384  0x00002000 ///< Signer 1 intermediate 1 TBS SHA-384 hash.
#define DB_STATE_SIGNER1_INTERMEDIATE1_TBS_HASH_SHA512  0x00004000 ///< Signer 1 intermediate 1 TBS SHA-512 hash.
#define DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA256  0x00008000 ///< Signer 1 intermediate 2 TBS SHA-256 hash.
#define DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA384  0x00010000 ///< Signer 1 intermediate 2 TBS SHA-384 hash.
#define DB_STATE_SIGNER1_INTERMEDIATE2_TBS_HASH_SHA512  0x00020000 ///< Signer 1 intermediate 2 TBS SHA-512 hash.
#define DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA256           0x00040000 ///< Signer 1 root TBS SHA-256 hash.
#define DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA384           0x00080000 ///< Signer 1 root TBS SHA-384 hash.
#define DB_STATE_SIGNER1_ROOT_TBS_HASH_SHA512           0x00100000 ///< Signer 1 root TBS SHA-512 hash.
#define DB_STATE_SIGNER2_TBS_HASH_SHA256                0x00200000 ///< Signer 2 TBS SHA-256 hash (multi-signed only).
#define DB_STATE_SIGNER2_TBS_HASH_SHA384                0x00400000 ///< Signer 2 TBS SHA-384 hash (multi-signed only).
#define DB_STATE_SIGNER2_TBS_HASH_SHA512                0x00800000 ///< Signer 2 TBS SHA-512 hash (multi-signed only).

//
// Deliberately malformed database entries (negative / robustness testing). The placement flag
// controls where the malformed EFI_SIGNATURE_LIST is emitted relative to the other entries, so
// a scenario can put a valid approver either before or after the corruption.
//
#define DB_STATE_CORRUPT_SIGNATURE_LIST_FIRST  0x01000000          ///< Malformed EFI_SIGNATURE_LIST placed before all other entries.
#define DB_STATE_CORRUPT_SIGNATURE_LIST_LAST   0x02000000          ///< Malformed EFI_SIGNATURE_LIST placed after all other entries.

//
// TBSCertificate hashes of a certificate embedded in signer 2's signature but not part of its
// chain (EFI_CERT_X509_SHA{256,384,512}_GUID); the counterpart to DB_STATE_SIGNER2_UNRELATED_CERT.
//
#define DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA256  0x04000000     ///< Signer 2 unrelated cert TBS SHA-256 hash (multi-signed only).
#define DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA384  0x08000000     ///< Signer 2 unrelated cert TBS SHA-384 hash (multi-signed only).
#define DB_STATE_SIGNER2_UNRELATED_TBS_HASH_SHA512  0x10000000     ///< Signer 2 unrelated cert TBS SHA-512 hash (multi-signed only).

///
/// Describes a single secure boot image validation scenario.
///
typedef struct {
  ///
  /// The scenario's test ID; used to reference detailed test information in the test app's README.
  ///
  CONST CHAR8    *Id;
  ///
  /// Which built-in image to load (see IMAGE_TYPE).
  ///
  IMAGE_TYPE     ImageType;
  ///
  /// DB_STATE_* flags describing the contents of the `db` database to build.
  ///
  UINT32         DbState;
  ///
  /// DB_STATE_* flags describing the contents of the `dbx` database to build.
  ///
  UINT32         DbxState;
  ///
  /// EFI_STATUS gBS->LoadImage() is expected to return for this scenario.
  ///
  EFI_STATUS     ExpectedStatus;
} SECURE_BOOT_IMAGE_TEST_SCENARIO;

/**
  Convenience initializer for a SECURE_BOOT_IMAGE_TEST_SCENARIO.
**/
#define TEST_SCENARIO(Id, ImageType, DbState, DbxState, ExpectedStatus)  \
  {                                                                          \
    (Id),                                                                    \
    (ImageType),                                                             \
    (DbState),                                                               \
    (DbxState),                                                              \
    (ExpectedStatus)                                                         \
  }

//
// All image validation scenarios.
//
extern CONST SECURE_BOOT_IMAGE_TEST_SCENARIO  mScenarios[];
extern CONST UINTN                            mScenarioCount;

/**
  Unit test body that drives a single secure boot image validation scenario.

  This function is registered as the test case routine for every scenario. It reads the
  SECURE_BOOT_IMAGE_TEST_SCENARIO passed through Context, resolves the image from ImageType,
  builds `db` / `dbx` from the DbState / DbxState flags, points the GetVariable() hook at
  those databases, calls gBS->LoadImage() on the image, unloads the image if it loaded, and
  asserts that the returned EFI_STATUS matches SECURE_BOOT_IMAGE_TEST_SCENARIO.ExpectedStatus.

  @param[in]  Context  A pointer to the SECURE_BOOT_IMAGE_TEST_SCENARIO that describes the
                       inputs and expected result.

  @retval  UNIT_TEST_PASSED             LoadImage() returned the expected status.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  LoadImage() returned an unexpected status.
**/
UNIT_TEST_STATUS
EFIAPI
RunImageValidationScenario (
  IN UNIT_TEST_CONTEXT  Context
  );

#endif // IMAGE_VALIDATION_TEST_APP_H_
