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
#include "Iterator.h"
#include "Support.h"

/**
  Validate a PE/COFF image against the platform signature databases.

    1. Reject immediately if the image's Authenticode hash is enrolled in the `dbx`.
    2. Walk each WIN_CERTIFICATE in the image's security data directory to determine if the
       Auth Data from it is not revoked by the `dbx` and is authorized by the `db`. Only one
       WIN_CERTIFICATE needs to authorize the image for it to be validated.
    3. Authorize the image if the image's Authenticode hash is enrolled in the `db`.

  When the image is rejected for any reason, an entry describing the rejection is appended to the
  Image Execution Information Table. The recorded EFI_IMAGE_EXECUTION_ACTION is:
    - EFI_IMAGE_EXECUTION_AUTH_UNTESTED      Unsigned image rejected (digest in `dbx`, or digest
                                             not in `db`).
    - EFI_IMAGE_EXECUTION_AUTH_SIG_FOUND     Signed image rejected because its digest is in `dbx`.
    - EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED    Signed image rejected and at least one certificate was
                                             revoked by `dbx` (or could not be evaluated).
    - EFI_IMAGE_EXECUTION_AUTH_SIG_NOT_FOUND Signed image rejected because no certificate is in `db`
                                             and the digest is not in `db`.

  @param[in]   File         Device path of the image being verified. Used to record rejections.
  @param[in]   FileBuffer   Pointer to the in-memory PE/COFF image.
  @param[in]   FileSize     Size of FileBuffer in bytes.
  @param[in]   SecDataDir   Security data directory describing the embedded WIN_CERTIFICATE table.
                            A Size of 0 indicates an unsigned image.
  @param[in,out] Measured   Authority measurement state used to record the `db` entry that
                            authorized the image into PCR 7 (de-duplicated across images).

  @retval EFI_SUCCESS        The image is authorized.
  @retval EFI_ACCESS_DENIED  The image is revoked, not authorized, or the
                             databases could not be loaded.
**/
EFI_STATUS
ValidateImage (
  IN     CONST EFI_DEVICE_PATH_PROTOCOL  *File,
  IN     VOID                            *FileBuffer,
  IN     UINTN                           FileSize,
  IN     CONST EFI_IMAGE_DATA_DIRECTORY  *SecDataDir,
  IN OUT MEASURED_AUTHORITIES            *Measured
  )
{
  EFI_STATUS                  Status;
  DIGEST_CACHE                Cache;
  SIGNATURE_DATABASES         Databases;
  WIN_CERT_ITER               CertIter;
  CONST WIN_CERTIFICATE       *Cert;
  IMAGE_AUTHORITY             Authority;
  EFI_IMAGE_EXECUTION_ACTION  Action;
  EFI_GUID                    RejectHashType;
  CONST UINT8                 *RejectDigest;
  UINTN                       RejectDigestSize;

  Action = EFI_IMAGE_EXECUTION_AUTH_SIG_NOT_FOUND;
  ZeroMem (&RejectHashType, sizeof (EFI_GUID));

  //
  // Setup digest cache for the image. This prevents redundant authenticode hash computations
  // across the image-hash revocation check, per-cert authorization, and the image-hash fallback.
  //
  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = DigestCacheTypeImage;
  Cache.Buffer     = FileBuffer;
  Cache.BufferSize = FileSize;

  Status = LoadSignatureDatabases (&Databases);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Failed to load signature databases (%r).\n", Status));
    goto Reject;
  }

  //
  // Step 1: Reject the image if its Authenticode hash is found in the `dbx`. A failure to search
  // the `dbx` rejects the image.
  //
  Status = GetImageDigestAuthority (Databases.Dbx, Databases.DbxSize, &Cache, &Authority);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Failed to search DBX for image hash (%r).\n", Status));
    Action = EFI_IMAGE_EXECUTION_AUTH_SIG_FOUND;
    goto Reject;
  }

  if (Authority.Data != NULL) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Image hash is forbidden by DBX.\n"));
    Action = EFI_IMAGE_EXECUTION_AUTH_SIG_FOUND;
    CopyGuid (&RejectHashType, &Authority.SignatureType);
    goto Reject;
  }

  //
  // Step 2: For each WIN_CERTIFICATE in the image's security data directory, extract the auth data
  // and check if it authorizes the image per the `db` and `dbx`. Exit on the first authorization.
  //
  // Note: If the image is unsigned, the iterator is empty and this step is a no-op.
  //
  Status = WinCertIterInit (&CertIter, FileBuffer, FileSize, SecDataDir);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Failed to walk security data directory (%r).\n", Status));
    Action = EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED;
    goto Reject;
  }

  while ((Cert = WinCertIterNext (&CertIter)) != NULL) {
    Status = GetImageCertAuthority (Cert, &Cache, &Databases, &Authority);
    if ((Status == EFI_SUCCESS) && (Authority.Data != NULL)) {
      //
      // Measure the `db` trust anchor that authorized the image into PCR 7.
      //
      SecureBootHook (
        Measured,
        EFI_IMAGE_SECURITY_DATABASE,
        &gEfiImageSecurityDatabaseGuid,
        &Authority
        );
      Status = EFI_SUCCESS;
      goto Exit;
    }

    //
    // This auth data is rejected by the `dbx`; update the rejection information so that we can
    // properly record a rejection record if we end up rejecting the image.
    //
    if (Status == EFI_ACCESS_DENIED) {
      CopyGuid (&RejectHashType, &Authority.SignatureType);
      Action = EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED;
    }
  }

  //
  // Step 3: Authorize the image if the image authenticode hash is in the `db`.
  //
  Status = GetImageDigestAuthority (Databases.Db, Databases.DbSize, &Cache, &Authority);
  if (!EFI_ERROR (Status) && (Authority.Data != NULL)) {
    //
    // Measure the `db` image-hash entry that authorized the image into PCR 7.
    //
    SecureBootHook (
      Measured,
      EFI_IMAGE_SECURITY_DATABASE,
      &gEfiImageSecurityDatabaseGuid,
      &Authority
      );
    Status = EFI_SUCCESS;
    goto Exit;
  }

  DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Image is not authorized by DB.\n"));

Reject:
  //
  // Above logic assumed signed images. If the image is unsigned, it's rejection reason is always
  // EFI_IMAGE_EXECUTION_AUTH_UNTESTED and the table record does not contain any signature
  // information.
  //
  if (SecDataDir->Size == 0) {
    Action = EFI_IMAGE_EXECUTION_AUTH_UNTESTED;
    ZeroMem (&RejectHashType, sizeof (EFI_GUID));
  }

  //
  // Recover the memoized image digest for the rejection signature when a hash
  // algorithm was established (SIG_FOUND / SIG_FAILED). A failure simply records
  // no signature.
  //
  RejectDigest     = NULL;
  RejectDigestSize = 0;
  if (!IsZeroGuid (&RejectHashType)) {
    GetHash (&RejectHashType, &Cache, &RejectDigest, &RejectDigestSize);
  }

  RecordRejectedImage (File, Action, &RejectHashType, RejectDigest, RejectDigestSize);
  Status = EFI_ACCESS_DENIED;

Exit:
  if (Databases.Db != NULL) {
    FreePool (Databases.Db);
  }

  if (Databases.Dbx != NULL) {
    FreePool (Databases.Dbx);
  }

  return Status;
}

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
  EFI_STATUS                Status;
  UINT32                    Policy;
  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

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
  // Secure Boot is the gate for all remaining checks. When it is not
  // enabled, the platform has opted out of image authorization.
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
    return Status;
  }

  //
  // Run image verification. The unified path handles both signed and
  // unsigned images; SecDataDir->Size == 0 simply produces an empty
  // WIN_CERTIFICATE iteration inside ValidateImage.
  //
  return ValidateImage (File, FileBuffer, FileSize, &SecDataDir, GetMeasuredAuthorities ());
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
