/** @file
  Internal declarations for the DXE Image Verification Library.

  This header is consumed by the library's source files (and its unit
  tests) to share prototypes for the constructor and the Security2
  verification handler.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <UefiSecureBoot.h>
#include <Guid/ImageAuthentication.h>
#include <IndustryStandard/PeImage.h>
#include <Library/SecureBootVariableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/SecurityManagementLib.h>
#include <Library/AuthenticodeLib.h>
#include <Pi/PiFirmwareFile.h>
#include <Pi/PiFirmwareVolume.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Hash.h>

//
// Authorization policy bit definition
//
#define ALWAYS_EXECUTE                      0x00000000
#define DENY_EXECUTE_ON_SECURITY_VIOLATION  0x00000001

//
// Image type definitions
//
#define IMAGE_UNKNOWN  0x00000000
#define IMAGE_FROM_FV  0x00000001

#define MAX_DIGEST_SIZE  SHA512_DIGEST_SIZE

//
// One supported signature-list type for the database visitors: its EFI_SIGNATURE_LIST SignatureType,
// the algorithm its entries are hashed under (NULL for a full-certificate type, which carries a DER
// certificate rather than a digest), and the per-entry SignatureOwner size (sizeof (EFI_GUID) for a
// V1 EFI_SIGNATURE_DATA type, 0 for a V2 EFI_SIGNATURE_V2_DATA type).
//
typedef struct {
  CONST EFI_GUID    *SignatureType;
  CONST EFI_GUID    *HashAlgorithm;
  UINTN             OwnerSize;
} SIGNATURE_TYPE_MAP;

//
// The image-hash signature types (EFI_CERT_SHA* / EFI_CERT_V2_SHA*) a hash-membership search may
// match, each mapped to its hash algorithm and owner size.
//
GLOBAL_REMOVE_IF_UNREFERENCED STATIC CONST SIGNATURE_TYPE_MAP  mImageHashSignatures[] = {
  { &gEfiCertSha256Guid,   &gEfiHashAlgorithmSha256Guid, sizeof (EFI_GUID) },
  { &gEfiCertV2Sha256Guid, &gEfiHashAlgorithmSha256Guid, 0                 },
  { &gEfiCertSha384Guid,   &gEfiHashAlgorithmSha384Guid, sizeof (EFI_GUID) },
  { &gEfiCertV2Sha384Guid, &gEfiHashAlgorithmSha384Guid, 0                 },
  { &gEfiCertSha512Guid,   &gEfiHashAlgorithmSha512Guid, sizeof (EFI_GUID) },
  { &gEfiCertV2Sha512Guid, &gEfiHashAlgorithmSha512Guid, 0                 },
};

//
// The X.509 TBS-cert-hash signature types (EFI_CERT_X509_SHA* / EFI_CERT_V2_X509_SHA*) a
// hash-membership search may match.
//
GLOBAL_REMOVE_IF_UNREFERENCED STATIC CONST SIGNATURE_TYPE_MAP  mTbsHashSignatures[] = {
  { &gEfiCertX509Sha256Guid,   &gEfiHashAlgorithmSha256Guid, sizeof (EFI_GUID) },
  { &gEfiCertV2X509Sha256Guid, &gEfiHashAlgorithmSha256Guid, 0                 },
  { &gEfiCertX509Sha384Guid,   &gEfiHashAlgorithmSha384Guid, sizeof (EFI_GUID) },
  { &gEfiCertV2X509Sha384Guid, &gEfiHashAlgorithmSha384Guid, 0                 },
  { &gEfiCertX509Sha512Guid,   &gEfiHashAlgorithmSha512Guid, sizeof (EFI_GUID) },
  { &gEfiCertV2X509Sha512Guid, &gEfiHashAlgorithmSha512Guid, 0                 },
};

//
// The full X.509 certificate signature types (EFI_CERT_X509 / EFI_CERT_V2_X509) and their per-entry
// SignatureOwner size, reusing SIGNATURE_TYPE_MAP with a NULL hash-algorithm column: these carry a
// DER certificate rather than a digest, so they are never hashed.
//
GLOBAL_REMOVE_IF_UNREFERENCED STATIC CONST SIGNATURE_TYPE_MAP  mX509CertSignatures[] = {
  { &gEfiCertX509Guid,   NULL, sizeof (EFI_GUID) },
  { &gEfiCertV2X509Guid, NULL, 0                 },
};

//
// An authority entry drawn from a signature database.
//
// Data is an owned, pool-allocated V1 EFI_SIGNATURE_DATA: a 16-byte SignatureOwner followed by the
// authorizing (`db`) or revoking (`dbx`) X.509 certificate. It is built with BuildImageAuthority ()
// and released with FreeImageAuthority (); Size is its total length in bytes. Data is NULL and Size
// is 0 when no authority applies.
//
// The SignatureOwner is copied from the matching V1 signature-list entry, or zeroed when that entry
// used the V2 (EFI_SIGNATURE_V2_DATA) layout. When the matching entry is a TBS cert-hash, the
// reconstructed certificate - not the hash - is stored.
//
// SignatureType is the signature-type GUID of the authorizing `db` list the Data entry came from
// (the EFI_SIGNATURE_LIST.SignatureType); it is zeroed when no authority applies.
//
typedef struct {
  EFI_SIGNATURE_DATA    *Data;
  UINTN                 Size;
  EFI_GUID              SignatureType;
} IMAGE_AUTHORITY;

//
// A single Secure Boot authority entry that has been measured into PCR 7.
// VariableName / VendorGuid reference canonical storage owned by the library;
// Data is a pool-allocated copy of the EFI_SIGNATURE_DATA that authorized an
// image.
//
typedef struct {
  CHAR16      *VariableName;
  EFI_GUID    *VendorGuid;
  VOID        *Data;
  UINTN       Size;
} MEASURED_VARIABLE;

//
// Tracks the set of authority entries that have been measured into PCR 7
// during this boot so that no single entry is measured more than once. The
// only instance of this structure is the module global owned by Measurement.c
// and obtained through GetMeasuredAuthorities ().
//
typedef struct {
  MEASURED_VARIABLE    *List;
  UINTN                Count;
  UINTN                Max;
} MEASURED_AUTHORITIES;

/**
  Return the module-global authority measurement state.

  The returned pointer references storage that persists for the lifetime of
  the module so that measurement de-duplication is maintained across every
  image the verification handler processes.

  @return  Pointer to the module-global MEASURED_AUTHORITIES instance.
**/
MEASURED_AUTHORITIES *
GetMeasuredAuthorities (
  VOID
  );

/**
  Resolve an image's authorization policy.

  @param[in]   File    Device path describing the image origin.
  @param[out]  Policy  On success, filled with the resolved policy value.

  @retval EFI_SUCCESS            Policy contains a valid policy value.
  @retval EFI_INVALID_PARAMETER  File or Policy is NULL.
**/
EFI_STATUS
GetExecutionPolicy (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File,
  OUT UINT32                          *Policy
  );

/**
  Measure a Secure Boot authority entry into PCR 7, skipping entries that have
  already been measured this boot.

  If VariableName / VendorGuid do not identify a Secure Boot authority
  variable, or the supplied data has already been measured, the call is a
  no-op. Otherwise the data is measured via TpmMeasureAndLogData and recorded
  in Measured so subsequent identical entries are not measured again.

  @param[in,out]  Measured      Authority measurement state to consult and update.
  @param[in]      VariableName  Name of the variable that authorized the image.
  @param[in]      VendorGuid    Vendor GUID of the variable that authorized the image.
  @param[in]      Authority     The `db` authority that authorized the image, whose Data / Size
                                identify the EFI_SIGNATURE_DATA entry to measure.
**/
VOID
SecureBootHook (
  IN OUT MEASURED_AUTHORITIES   *Measured,
  IN     CHAR16                 *VariableName,
  IN     EFI_GUID               *VendorGuid,
  IN     CONST IMAGE_AUTHORITY  *Authority
  );

/**
  Provide verification service for signed images, which include both signature validation
  and platform policy control. For signature types, both UEFI WIN_CERTIFICATE_UEFI_GUID and
  MSFT Authenticode type signatures are supported.

  In this implementation, only verify external executables when in USER MODE.
  Executables from FV is bypass, so pass in AuthenticationStatus is ignored.

  The image verification policy is:
    If the image is signed,
      At least one valid signature or at least one hash value of the image must match a record
      in the security database "db", and no valid signature nor any hash value of the image may
      be reflected in the security database "dbx".
    Otherwise, the image is not signed,
      The hash value of the image must match a record in the security database "db", and
      not be reflected in the security data base "dbx".

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  @param[in]    AuthenticationStatus
                           This is the authentication status returned from the security
                           measurement services for the input file.
  @param[in]    File       This is a pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.
  @param[in]    FileBuffer File buffer matches the input file device path.
  @param[in]    FileSize   Size of File buffer matches the input file device path.
  @param[in]    BootPolicy A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS            The file specified by DevicePath and non-NULL
                                 FileBuffer did authenticate, and the platform policy dictates
                                 that the DXE Foundation may use the file.
  @retval EFI_SUCCESS            The device path specified by NULL device path DevicePath
                                 and non-NULL FileBuffer did authenticate, and the platform
                                 policy dictates that the DXE Foundation may execute the image in
                                 FileBuffer.
  @retval EFI_ACCESS_DENIED      The file specified by File and FileBuffer did not
                                 authenticate, and the DXE Foundation may not use File. The
                                 image has been added to the file execution table.

**/
EFI_STATUS
EFIAPI
DxeImageVerificationHandler (
  IN  UINT32                          AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File  OPTIONAL,
  IN  VOID                            *FileBuffer,
  IN  UINTN                           FileSize,
  IN  BOOLEAN                         BootPolicy
  );

/**
  Register security measurement handler.

  @param  ImageHandle   ImageHandle of the loaded driver.
  @param  SystemTable   Pointer to the EFI System Table.

  @retval EFI_SUCCESS   The handlers were registered successfully.
**/
EFI_STATUS
EFIAPI
DxeImageVerificationLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  );
