/** @file
  Implement image verification services for secure boot service

  Caution: This file requires additional review when modified.
  This library will have external input - PE/COFF image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  DxeImageVerificationLibImageRead() function will make sure the PE/COFF image content
  read is within the image buffer.

  DxeImageVerificationHandler(), HashPeImageByType(), HashPeImage() function will accept
  untrusted PE/COFF image and validate its data structure within this image buffer before use.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeImageVerificationLib.h"
#include "Database.h"
#include "Support.h"
#include "Policy.h"

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
  )
{
  EFI_STATUS                  Status;
  UINT32                      Policy;
  EFI_IMAGE_DATA_DIRECTORY    SecDataDir;
  EFI_IMAGE_EXECUTION_ACTION  Action;

  Action = EFI_IMAGE_EXECUTION_AUTH_UNTESTED;

  //
  // Sanity check.
  //
  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Resolve the platform authorization policy from the image's origin.
  // This runs before the Secure Boot variable check because it is much
  // cheaper, and the common case (FV-dispatched drivers) short-circuits
  // if the policy is ALWAYS_EXECUTE.
  //
  Status = GetExecutionPolicy (File, &Policy);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Policy unconditionally permits execution; no further checks needed.
  //
  if (Policy == ALWAYS_EXECUTE) {
    return EFI_SUCCESS;
  }

  //
  // This handler only enforces UEFI Secure Boot. If Secure Boot is not
  // enabled there is nothing for us to verify.
  //
  if (!IsSecureBootEnabled ()) {
    return EFI_SUCCESS;
  }

  //
  // Inspect the image to locate its security data directory. Any failure
  // to parse the PE/COFF headers is treated as a verification failure.
  //
  Status = GetImageSecurityDataDirectory (FileBuffer, FileSize, &SecDataDir);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Dispatch to the appropriate verification path based on whether the
  // image carries an embedded signature.
  //
  if (SecDataDir.Size == 0) {
    Status = ValidateUnsignedImage (FileBuffer, FileSize);
  } else {
    Status = ValidateSignedImage (FileBuffer, FileSize, &SecDataDir, &Action);
  }

Exit:
  return Status;
}

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
  )
{
  return RegisterSecurity2Handler (
           DxeImageVerificationHandler,
           EFI_AUTH_OPERATION_VERIFY_IMAGE | EFI_AUTH_OPERATION_IMAGE_REQUIRED
           );
}

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
  )
{
  EFI_STATUS          Status;
  HASH_ALGORITHM_SET  HashAlgorithms;
  UINTN               Index;
  CONST EFI_GUID      *HashType;
  UINTN               DigestSize;
  UINT8               ImageDigest[SHA512_DIGEST_SIZE];
  BOOLEAN             IsFound;
  BOOLEAN             IsFoundInDb;
  VOID                *Db;
  UINTN               DbSize;
  VOID                *Dbx;
  UINTN               DbxSize;

  Db  = NULL;
  Dbx = NULL;

  //
  // Load the authorized (db) and forbidden (dbx) signature databases.
  //
  Status = LoadSignatureDatabase (EFI_IMAGE_SECURITY_DATABASE, &Db, &DbSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to load db - %r\n", Status));
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  Status = LoadSignatureDatabase (EFI_IMAGE_SECURITY_DATABASE1, &Dbx, &DbxSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to load dbx - %r\n", Status));
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  //
  // Determine which algorithms are currently in use across db and dbx.
  // If neither database enrolls any recognized hash type, there is no algorithm
  // with which to authorize an unsigned image, so refuse to dispatch it.
  //
  Status = GetDatabaseHashAlgorithms (Db, DbSize, Dbx, DbxSize, &HashAlgorithms);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: GetDatabaseHashAlgorithms failed - %r\n", Status));
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  if (HashAlgorithms.Count == 0) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: no hash algorithms enrolled in db/dbx; rejecting unsigned image.\n"));
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  //
  // For each algorithm in use, compute the image's Authenticode digest and query the dbx / db.
  // A hit in the dbx immediately denies the image. If no dbx hit occurs across all algorithms,
  // the image is authorized or denied based on whether at least one db hit was recorded.
  //
  IsFoundInDb = FALSE;
  for (Index = 0; Index < HashAlgorithms.Count; Index++) {
    HashType = &HashAlgorithms.Guids[Index];

    Status = GetAuthenticodeHash (FileBuffer, FileSize, HashType, ImageDigest, &DigestSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: GetAuthenticodeHash failed - %r\n", Status));
      Status = EFI_ACCESS_DENIED;
      goto Exit;
    }

    Status = IsSignatureFoundInDatabase (
               Dbx,
               DbxSize,
               ImageDigest,
               HashType,
               DigestSize,
               &IsFound
               );
    if (EFI_ERROR (Status) || IsFound) {
      DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Image is not signed and image is forbidden by DBX.\n"));
      Status = EFI_ACCESS_DENIED;
      goto Exit;
    }

    if (IsFoundInDb) {
      continue;
    }

    Status = IsSignatureFoundInDatabase (
               Db,
               DbSize,
               ImageDigest,
               HashType,
               DigestSize,
               &IsFound
               );
    if (!EFI_ERROR (Status) && IsFound) {
      IsFoundInDb = TRUE;
    }
  }

  Status = IsFoundInDb ? EFI_SUCCESS : EFI_ACCESS_DENIED;

Exit:
  if (Db != NULL) {
    FreePool (Db);
  }

  if (Dbx != NULL) {
    FreePool (Dbx);
  }

  return Status;
}

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
  )
{
  *Action = EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED;
  return EFI_UNSUPPORTED;
}
