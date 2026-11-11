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
#include "Ecit.h"
#include "Support.h"

#include <Library/DevicePathLib.h>

STATIC CONST EFI_DEVICE_PATH_PROTOCOL  mUnknownDevicePath = {
  END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
};

/**
  Append one evaluated signature, abandoning an incomplete record on reporting failure.
**/
STATIC
VOID
AppendVerificationSignature (
  IN OUT IMAGE_VERIFICATION_INFO        *Info OPTIONAL,
  IN UINT32                             SignatureIndex,
  IN CONST SIGNATURE_VERIFICATION_INFO  *Verification
  )
{
  EFI_STATUS  Status;

  if ((Info == NULL) || (Info->Image == NULL)) {
    return;
  }

  Status = ImageVerificationResultAppendSignature (
             &Info->Image,
             SignatureIndex,
             Verification->Status,
             (Verification->ThumbprintSize == 0) ? NULL : &Verification->ThumbprintAlgorithm,
             (Verification->ThumbprintSize == 0) ? NULL : Verification->Thumbprint,
             Verification->ThumbprintSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to append SBRT signature (%r).\n", Status));
    FreePool (Info->Image);
    Info->Image = NULL;
  }
}

/**
  Publish a completed image record without changing its authorization decision.
**/
STATIC
VOID
PublishVerificationResult (
  IN CONST IMAGE_VERIFICATION_INFO  *Info
  )
{
  EFI_STATUS                                       Status;
  VOID                                             *OldTable;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *NewTable;

  if (Info->Image == NULL) {
    return;
  }

  OldTable = NULL;
  Status   = EfiGetSystemConfigurationTable (&gEfiImageSecureBootVerificationResultTableGuid, &OldTable);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to locate SBRT (%r).\n", Status));
    return;
  }

  Status = ImageVerificationResultAppendImage (
             OldTable,
             Info->Image,
             Info->Status,
             IsZeroGuid (&Info->DigestAlgorithm) ? NULL : &Info->DigestAlgorithm,
             &NewTable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to append SBRT image (%r).\n", Status));
    return;
  }

  Status = gBS->InstallConfigurationTable (&gEfiImageSecureBootVerificationResultTableGuid, NewTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to publish SBRT (%r).\n", Status));
    FreePool (NewTable);
  } else if (OldTable != NULL) {
    FreePool (OldTable);
  }
}

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
  @param[in]   WinCertificates        Raw bytes of the image's embedded WIN_CERTIFICATE table, or
                                      NULL when the image is unsigned.
  @param[in]   WinCertificatesLength  Length of WinCertificates in bytes; 0 when unsigned.
  @param[in,out] Measured             Authority measurement state used to record the `db`
                                      certificate that authorized the image into PCR 7
                                      (de-duplicated across images). Only certificate authorities
                                      are measured; image-hash authorizations are not.
  @param[in,out] Info                 Optional SBRT image record and decision metadata.

  @retval EFI_SUCCESS        The image is authorized.
  @retval EFI_ACCESS_DENIED  The image is revoked, not authorized, or the
                             databases could not be loaded.
**/
EFI_STATUS
ValidateImage (
  IN     CONST VOID               *AuthenticodeImage,
  IN     UINTN                    AuthenticodeImageSize,
  IN     CONST UINT8              *WinCertificates,
  IN     UINTN                    WinCertificatesLength,
  IN OUT MEASURED_AUTHORITIES     *Measured,
  IN OUT IMAGE_VERIFICATION_INFO  *Info OPTIONAL
  )
{
  EFI_STATUS                  Status;
  DIGEST_CACHE                Cache;
  SIGNATURE_LISTS             Lists;
  WIN_CERT_ITER               CertIter;
  CONST UINT8                 *Cert;
  UINTN                       CertSize;
  WIN_CERTIFICATE             CertHeader;
  CONST UINT8                 *SignatureData;
  UINTN                       SignatureDataSize;
  IMAGE_SIGNATURE_EVALUATION  Evaluation;
  EFI_GUID                    HashAlgorithm;
  UINT32                      SignatureIndex;
  BOOLEAN                     CertificatesValid;

  ZeroMem (&Cache, sizeof (Cache));
  ZeroMem (&Evaluation, sizeof (Evaluation));
  if (Info != NULL) {
    Info->Status = EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY;
    ZeroMem (&Info->DigestAlgorithm, sizeof (Info->DigestAlgorithm));
  }

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
  if (IsImageHashInRevokeList (&Cache, Lists.RevokeList, Lists.RevokeListSize, &HashAlgorithm)) {
    if ((Info != NULL) && !IsZeroGuid (&HashAlgorithm)) {
      Info->Status = EFI_IMAGE_VERIFICATION_STATUS_REJECTED_BY_DIGEST;
      CopyGuid (&Info->DigestAlgorithm, &HashAlgorithm);
    }

    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: image hash is forbidden by dbx.\n"));
    goto Reject;
  }

  //
  // Step 2: Authorize the image if its Authenticode hash is found in db, passed to the generic
  // allow-list search.
  //
  if (IsImageHashInAllowList (&Cache, Lists.AllowList, Lists.AllowListSize, &HashAlgorithm)) {
    if (Info != NULL) {
      Info->Status = EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_DIGEST;
      CopyGuid (&Info->DigestAlgorithm, &HashAlgorithm);
    }

    Status = EFI_SUCCESS;
    goto Exit;
  }

  //
  // Step 3: For each WIN_CERTIFICATE, extract its signature data and evaluate it against db and
  // dbx. Exit on the first authorization.
  //
  CertificatesValid = WinCertIterInit (&CertIter, WinCertificates, WinCertificatesLength);
  if (!CertificatesValid) {
    DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: certificate table truncated at a malformed entry; evaluating the valid prefix.\n"));
  }

  SignatureIndex = 0;
  while ((Cert = WinCertIterNext (&CertIter, &CertSize)) != NULL) {
    ZeroMem (&Evaluation.Verification, sizeof (Evaluation.Verification));
    Evaluation.Verification.Status = EFI_SIGNATURE_VERIFICATION_MALFORMED;
    CopyMem (&CertHeader, Cert, sizeof (CertHeader));
    Status = ExtractSignatureData (Cert, CertSize, &SignatureData, &SignatureDataSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: WIN_CERTIFICATE not usable (type=0x%04x, %r).\n", CertHeader.wCertificateType, Status));
      AppendVerificationSignature (Info, SignatureIndex++, &Evaluation.Verification);
      continue;
    }

    Status = EvaluateSignature (
               SignatureData,
               SignatureDataSize,
               &Cache,
               &Lists,
               &Evaluation
               );
    AppendVerificationSignature (Info, SignatureIndex++, &Evaluation.Verification);
    if (!EFI_ERROR (Status) && (Evaluation.Verdict == ImageSignatureAllowed)) {
      if (Info != NULL) {
        Info->Status = EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY;
      }

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

  if (!CertificatesValid) {
    ZeroMem (&Evaluation.Verification, sizeof (Evaluation.Verification));
    Evaluation.Verification.Status = EFI_SIGNATURE_VERIFICATION_MALFORMED;
    AppendVerificationSignature (Info, SignatureIndex, &Evaluation.Verification);
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
  @param[in]    File       Device path of the image, or NULL if its origin is unknown.
  @param[in]    FileBuffer Buffer containing the image to verify.
  @param[in]    FileSize   Size of FileBuffer in bytes.
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
  @retval EFI_INVALID_PARAMETER  File and FileBuffer are both NULL.

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
  EFI_STATUS               Status;
  UINT32                   Policy;
  UINT8                    *AuthImage;
  UINTN                    AuthImageSize;
  CONST UINT8              *WinCertificates;
  UINTN                    WinCertificatesLength;
  IMAGE_VERIFICATION_INFO  Info;
  CHAR16                   *Name;
  EFI_STATUS               ReportStatus;

  if ((File == NULL) && (FileBuffer == NULL)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: image buffer and device path are both NULL.\n"));
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

  ZeroMem (&Info, sizeof (Info));
  Info.Status = EFI_IMAGE_VERIFICATION_STATUS_REJECTED_MALFORMED;
  if (File != NULL) {
    Name = ConvertDevicePathToText (File, FALSE, TRUE);
    if (Name == NULL) {
      DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to convert the SBRT image device path to text.\n"));
    }

    ReportStatus = ImageVerificationResultCreateImage (Name, File, GetDevicePathSize (File), &Info.Image);
    if (Name != NULL) {
      FreePool (Name);
    }
  } else {
    ReportStatus = ImageVerificationResultCreateImage (NULL, &mUnknownDevicePath, sizeof (mUnknownDevicePath), &Info.Image);
  }

  if (EFI_ERROR (ReportStatus)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to create SBRT image (%r).\n", ReportStatus));
  }

  Status = BuildAuthenticodeImage (FileBuffer, FileSize, &AuthImage, &AuthImageSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Failed to assemble the Authenticode image (%r).\n", Status));
    Status = EFI_ACCESS_DENIED;
    goto Report;
  }

  Status = GetWinCertificates (FileBuffer, FileSize, &WinCertificates, &WinCertificatesLength);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: Failed to locate the certificate table (%r).\n", Status));
    FreePool (AuthImage);
    Status = EFI_ACCESS_DENIED;
    goto Report;
  }

  Status = ValidateImage (AuthImage, AuthImageSize, WinCertificates, WinCertificatesLength, GetMeasuredAuthorities (), &Info);

  FreePool (AuthImage);

Report:
  PublishVerificationResult (&Info);
  if (Info.Image != NULL) {
    FreePool (Info.Image);
  }

  return Status;
}

/**
  Register the image security handler and report its ECIT capabilities.

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
  EFI_STATUS  Status;

  Status = RegisterSecurity2Handler (
             DxeImageVerificationHandler,
             EFI_AUTH_OPERATION_VERIFY_IMAGE | EFI_AUTH_OPERATION_IMAGE_REQUIRED
             );
  if (!EFI_ERROR (Status)) {
    ReportImageVerificationCapabilities ();
  }

  return Status;
}
