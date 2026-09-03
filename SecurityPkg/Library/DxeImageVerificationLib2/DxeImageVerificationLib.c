/** @file
  Implement image verification services for secure boot service

  Caution: This file requires additional review when modified.
  This library will have external input - PE/COFF image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  DxeImageVerificationHandler() passes the untrusted PE/COFF image to
  AuthenticodeLib for bounds-checked parsing. AuthenticodeLib returns the
  prepared Authenticode byte stream and the image's WIN_CERTIFICATE table;
  the handler passes both directly to ValidateImage().

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
  Validate a prepared Authenticode image and WIN_CERTIFICATE table against
  the platform signature databases.

    1. Reject immediately if the image's Authenticode hash is enrolled in the `dbx`.
    2. Walk each WIN_CERTIFICATE to determine if the Auth Data from it is not revoked by the `dbx`
       and is authorized by the `db`. Only one WIN_CERTIFICATE needs to authorize the image for it
       to be validated.
    3. Authorize the image if the image's Authenticode hash is enrolled in the `db`.

  @param[in]   AuthenticodeImage      The assembled Authenticode image (the exact bytes the
                                      image-hash checks hash).
  @param[in]   AuthenticodeImageSize  Size of AuthenticodeImage in bytes.
  @param[in]   WinCertificates        The image's embedded WIN_CERTIFICATE table, or NULL when the
                                      image is unsigned.
  @param[in]   WinCertificatesLength  Length of WinCertificates in bytes; 0 when unsigned.
  @param[in,out] Measured             Authority measurement state used to record the `db`
                                      certificate that authorized the image into PCR 7
                                      (de-duplicated across images). Only certificate authorities
                                      are measured; image-hash authorizations are not.

  @retval EFI_SUCCESS        The image is authorized.
  @retval EFI_ACCESS_DENIED  The image is revoked, not authorized, or the
                             databases could not be loaded.
**/
EFI_STATUS
ValidateImage (
  IN     CONST VOID             *AuthenticodeImage,
  IN     UINTN                  AuthenticodeImageSize,
  IN     CONST WIN_CERTIFICATE  *WinCertificates,
  IN     UINTN                  WinCertificatesLength,
  IN OUT MEASURED_AUTHORITIES   *Measured
  )
{
  EFI_STATUS             Status;
  DIGEST_CACHE           Cache;
  SIGNATURE_DATABASES    Databases;
  WIN_CERT_ITER          CertIter;
  CONST WIN_CERTIFICATE  *Cert;
  IMAGE_CERT_EVALUATION  CertEval;

  //
  // Setup digest cache for the image. This prevents redundant authenticode hash computations
  // across the image-hash revocation check, per-cert authorization, and the image-hash fallback.
  //
  ZeroMem (&Cache, sizeof (Cache));

  //
  // Zeroed up front so every Exit path can safely release CertEval.Authority, even if the
  // certificate walk never runs (unsigned image) or an earlier step rejects.
  //
  ZeroMem (&CertEval, sizeof (CertEval));

  Status = LoadSignatureDatabases (&Databases);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Failed to load signature databases (%r).\n", Status));
    goto Reject;
  }

  //
  // Bind the digest cache to the caller-supplied Authenticode image (the exact bytes the image-hash
  // checks hash).
  //
  Cache.Buffer     = AuthenticodeImage;
  Cache.BufferSize = AuthenticodeImageSize;

  //
  // Step 1: Reject the image if its Authenticode hash is found in the `dbx`. A `dbx` that cannot
  // be fully parsed fails closed (IsImageHashInDbx returns TRUE), rejecting the image.
  //
  if (IsImageHashInDbx (&Cache, Databases.Dbx, Databases.DbxSize)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Image hash is forbidden by DBX.\n"));
    goto Reject;
  }

  //
  // Step 2: For each WIN_CERTIFICATE, extract the auth data and check if it authorizes the image
  // per the `db` and `dbx`. Exit on the first authorization.
  //
  // Note: If the image is unsigned, the iterator is empty and this step is a no-op. A malformed
  // certificate table only truncates the walk to its valid prefix (best-effort); the certificates
  // that parse cleanly are still evaluated, and the image can still be authorized by Step 3.
  //
  if (!WinCertIterInit (&CertIter, WinCertificates, WinCertificatesLength)) {
    DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: certificate table truncated at a malformed entry; evaluating the valid prefix.\n"));
  }

  while ((Cert = WinCertIterNext (&CertIter)) != NULL) {
    Status = EvaluateImageCertificate (Cert, &Cache, &Databases, &CertEval);
    if (!EFI_ERROR (Status) && (CertEval.Verdict == ImageCertApproved)) {
      //
      // Measure the `db` certificate that authorized the image into PCR 7.
      //
      SecureBootHook (
        Measured,
        EFI_IMAGE_SECURITY_DATABASE,
        &gEfiImageSecurityDatabaseGuid,
        &CertEval.Authority
        );
      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

  //
  // Step 3: Authorize the image if the image authenticode hash is in the `db`. Image-hash
  // authorization is intentionally not measured into PCR 7 - only certificate authorities are.
  //
  if (IsImageHashInDb (&Cache, Databases.Db, Databases.DbSize)) {
    Status = EFI_SUCCESS;
    goto Exit;
  }

  DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Image is not authorized by DB.\n"));

Reject:
  Status = EFI_ACCESS_DENIED;

Exit:
  FreeImageAuthority (&CertEval.Authority);

  FreeDigestCache (&Cache);

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
  EFI_STATUS             Status;
  UINT32                 Policy;
  UINT8                  *AuthImage;
  UINTN                  AuthImageSize;
  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  WinCertificatesLength;

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
  // Assemble the Authenticode image and locate the embedded WIN_CERTIFICATE table. Any failure to
  // parse the image is treated as a verification failure.
  //
  Status = BuildAuthenticodeImage (FileBuffer, FileSize, &AuthImage, &AuthImageSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Failed to assemble the Authenticode image (%r).\n", Status));
    return EFI_ACCESS_DENIED;
  }

  Status = GetWinCertificates (FileBuffer, FileSize, &WinCertificates, &WinCertificatesLength);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Failed to locate the certificate table (%r).\n", Status));
    FreePool (AuthImage);
    return EFI_ACCESS_DENIED;
  }

  //
  // Run image verification. The unified path handles both signed and unsigned images; an unsigned
  // image simply produces an empty WIN_CERTIFICATE iteration inside ValidateImage.
  //
  Status = ValidateImage (AuthImage, AuthImageSize, WinCertificates, WinCertificatesLength, GetMeasuredAuthorities ());

  FreePool (AuthImage);
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
