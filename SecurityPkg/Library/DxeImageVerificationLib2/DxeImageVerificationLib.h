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
#include <Pi/PiFirmwareFile.h>
#include <Pi/PiFirmwareVolume.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/DevicePath.h>

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
// Type definition for all information necessary to describe hash algorithm usage in this library.
//
// Each algorithm lists its signature-type GUIDs for both roles (raw image hash, X.509 TBS-cert
// hash) in both layouts: the V1 GUID whose list entries use EFI_SIGNATURE_DATA (a 16-byte
// SignatureOwner precedes the payload) and the V2 GUID whose entries use EFI_SIGNATURE_V2_DATA (no
// SignatureOwner). ImageHashGuid (V1) is the canonical GUID handed to the BaseCryptLib hashing
// primitives, which only recognize the V1 image-hash GUIDs.
//
typedef struct {
  CONST CHAR8       *Name;
  CONST EFI_GUID    *ImageHashGuid;        // EFI_CERT_SHA*_GUID          (V1)
  CONST EFI_GUID    *ImageHashGuidV2;      // EFI_CERT_V2_SHA*_GUID       (V2)
  CONST EFI_GUID    *X509CertHashGuid;     // EFI_CERT_X509_SHA*_GUID     (V1)
  CONST EFI_GUID    *X509CertHashGuidV2;   // EFI_CERT_V2_X509_SHA*_GUID  (V2)
} HASH_ALGORITHM;

//
// All supported hash algorithms for secureboot validation. Adding a new algorithm to this list
// will add support for that algorithm across the entire library.
//
STATIC CONST HASH_ALGORITHM  mHashAlgorithms[] = {
  { "SHA256", &gEfiCertSha256Guid, &gEfiCertV2Sha256Guid, &gEfiCertX509Sha256Guid, &gEfiCertV2X509Sha256Guid },
  { "SHA384", &gEfiCertSha384Guid, &gEfiCertV2Sha384Guid, &gEfiCertX509Sha384Guid, &gEfiCertV2X509Sha384Guid },
  { "SHA512", &gEfiCertSha512Guid, &gEfiCertV2Sha512Guid, &gEfiCertX509Sha512Guid, &gEfiCertV2X509Sha512Guid },
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
