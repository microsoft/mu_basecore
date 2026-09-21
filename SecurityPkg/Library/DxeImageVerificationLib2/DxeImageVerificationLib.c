/** @file
  Image verification services for secure boot service

  Caution: This file requires additional review when modified.
  This library will have external input - PE/COFF image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  DxeImageVerificationHandler() passes the untrusted PE/COFF image to
  AuthenticodeLib for bounds-checked parsing. AuthenticodeLib returns the
  prepared Authenticode byte stream and the image's WIN_CERTIFICATE table;
  the handler passes both directly to ValidateImage().

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeImageVerificationLib.h"
#include "Database.h"
#include "Support.h"

/**
  Validate a prepared Authenticode image and WIN_CERTIFICATE table against the platform signature
  databases.

    1. Reject immediately if the image's Authenticode hash is enrolled in `dbx`.
    2. Authorize the image if its Authenticode hash is enrolled in `db`.
    3. Walk each WIN_CERTIFICATE to determine if its signature is not revoked by `dbx` and is
       authorized by `db`. Only one WIN_CERTIFICATE needs to authorize the image for validation.

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
  EFI_STATUS                  Status;
  DIGEST_CACHE                Cache;
  SIGNATURE_LISTS             Lists;
  WIN_CERT_ITER               CertIter;
  CONST WIN_CERTIFICATE       *Cert;
  CONST UINT8                 *SignatureData;
  UINTN                       SignatureDataSize;
  IMAGE_SIGNATURE_EVALUATION  Evaluation;

  ZeroMem (&Cache, sizeof (Cache));
  ZeroMem (&Evaluation, sizeof (Evaluation));

  Status = LoadDbAndDbx (&Lists);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to load db and dbx (%r).\n", Status));
    goto Reject;
  }

  // The digest cache prevents redundant Authenticode hash computations against db and dbx.
  Cache.Buffer     = AuthenticodeImage;
  Cache.BufferSize = AuthenticodeImageSize;

  //
  // Step 1: Reject the image if its Authenticode hash is found in dbx, passed to the generic
  // revoke-list search. An unparseable dbx fails closed.
  //
  if (IsImageHashInRevokeList (&Cache, Lists.RevokeList, Lists.RevokeListSize)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: image hash is forbidden by dbx.\n"));
    goto Reject;
  }

  //
  // Step 2: Authorize the image if its Authenticode hash is found in db, passed to the generic
  // allow-list search.
  //
  if (IsImageHashInAllowList (&Cache, Lists.AllowList, Lists.AllowListSize)) {
    Status = EFI_SUCCESS;
    goto Exit;
  }

  //
  // Step 3: For each WIN_CERTIFICATE, extract its signature data and evaluate it against db and
  // dbx. Exit on the first authorization.
  //
  if (!WinCertIterInit (&CertIter, WinCertificates, WinCertificatesLength)) {
    DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: certificate table truncated at a malformed entry; evaluating the valid prefix.\n"));
  }

  while ((Cert = WinCertIterNext (&CertIter)) != NULL) {
    Status = ExtractSignatureData (Cert, &SignatureData, &SignatureDataSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: WIN_CERTIFICATE not usable (type=0x%04x, %r).\n", Cert->wCertificateType, Status));
      continue;
    }

    Status = EvaluateSignature (
               SignatureData,
               SignatureDataSize,
               &Cache,
               &Lists,
               &Evaluation
               );
    if (!EFI_ERROR (Status) && (Evaluation.Verdict == ImageSignatureAllowed)) {
      SecureBootHook (
        Measured,
        EFI_IMAGE_SECURITY_DATABASE,
        &gEfiImageSecurityDatabaseGuid,
        &Evaluation.Authority
        );
      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

  DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: image is not authorized by db.\n"));

Reject:
  Status = EFI_ACCESS_DENIED;

Exit:
  FreeImageAuthority (&Evaluation.Authority);

  FreeDigestCache (&Cache);

  if (Lists.AllowList != NULL) {
    FreePool (Lists.AllowList);
  }

  if (Lists.RevokeList != NULL) {
    FreePool (Lists.RevokeList);
  }

  return Status;
}

/**
  Provide verification service for signed images.

  If platform policy enforces image validation, this handler attempts to authorize
  the image based on its image authenticode digest, or any signatures present in the
  image's security data directory.

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

  if (Policy == ALWAYS_EXECUTE) {
    return EFI_SUCCESS;
  }

  if (!IsSecureBootEnabled ()) {
    return EFI_SUCCESS;
  }

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
