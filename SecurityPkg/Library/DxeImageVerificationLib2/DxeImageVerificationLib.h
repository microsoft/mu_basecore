/** @file
  Internal declarations for the DXE Image Verification Library.

  This header is consumed by the library's source files (and its unit
  tests) to share prototypes for the constructor and the Security2
  verification handler.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DXE_IMAGE_VERIFICATION_LIB_H_
#define DXE_IMAGE_VERIFICATION_LIB_H_

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
#include <Protocol/DevicePath.h>

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

/**
  Validate an unsigned PE/COFF image against the platform signature
  databases.

  For each image-hash algorithm enrolled in db or dbx, computes the
  image's Authenticode digest and checks the dbx then db for the hash.
  A dbx hit denies the image. The image is authorized only if it is
  found in db and never in dbx.

  @param[in]  FileBuffer  Pointer to the in-memory PE/COFF image.
  @param[in]  FileSize    Size of FileBuffer in bytes.

  @retval EFI_SUCCESS        The image's hash was found in db (and not
                             in dbx) under at least one enrolled
                             algorithm.
  @retval EFI_ACCESS_DENIED  The image was rejected: either no hash
                             algorithm is enrolled, the digest is
                             present in dbx, the digest is not present
                             in db, or a database lookup failed.
**/
EFI_STATUS
ValidateUnsignedImage (
  IN  VOID   *FileBuffer,
  IN  UINTN  FileSize
  );

/**
Validate a signed PE/COFF image's embedded Authenticode/UEFI signatures
against the platform signature databases.

@param[in]   FileBuffer  Pointer to the in-memory PE/COFF image.
@param[in]   FileSize    Size of FileBuffer in bytes.
@param[in]   SecDataDir  Security data directory describing the
                         embedded WIN_CERTIFICATE table.
@param[out]  Action      Set to the EFI_IMAGE_EXECUTION_ACTION value
                         that best describes the outcome.

@retval EFI_UNSUPPORTED  The signed-image verification path is not yet
                         implemented.
**/
EFI_STATUS
ValidateSignedImage (
  IN  VOID                            *FileBuffer,
  IN  UINTN                           FileSize,
  IN  CONST EFI_IMAGE_DATA_DIRECTORY  *SecDataDir,
  OUT EFI_IMAGE_EXECUTION_ACTION      *Action
  );

#endif // DXE_IMAGE_VERIFICATION_LIB_H_
