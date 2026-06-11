/** @file
  Secureboot DB/DBX/DBT (EFI_SIGNATURE_LIST) helpers, including signed-image
  (certificate / Authenticode) validation, for the DXE Image Verification Library.

  Caution: This file consumes external input (the PE/COFF image and the
  Secure Boot signature databases). All inputs must be treated as
  attacker-controlled.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Database.h"

/**
  Load a Secure Boot Signature Database into a pool-allocated buffer.

  The returned buffer is allocated using AllocatePool(). The caller is responsible for freeing
  this buffer with FreePool().

  @param[in]   DatabaseName  Variable name (e.g. EFI_IMAGE_SECURITY_DATABASE,
                             EFI_IMAGE_SECURITY_DATABASE1).
  @param[out]  Buffer        Pool-allocated copy of the variable contents,
                             or NULL if the variable does not exist.
                             Caller is responsible for freeing this buffer with
                             FreePool when non-NULL.
  @param[out]  BufferSize    BufferSize of *Buffer in bytes, or 0 if the
                             variable does not exist.

  @retval EFI_SUCCESS            The variable was loaded successfully, or it was absent.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval Other                  Status from gRT->GetVariable.
**/
EFI_STATUS
LoadSignatureDatabase (
  IN  CONST CHAR16  *DatabaseName,
  OUT VOID          **Buffer,
  OUT UINTN         *BufferSize
  )
{
  EFI_STATUS  Status;

  if ((DatabaseName == NULL) || (Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Buffer     = NULL;
  *BufferSize = 0;

  Status = GetVariable2 (DatabaseName, &gEfiImageSecurityDatabaseGuid, Buffer, BufferSize);
  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  return Status;
}

/**
  Load the platform's db and dbx signature databases.

  The Db / Dbx buffers in the returned structure are allocated using AllocatePool(). The caller is
  responsible for freeing them with FreePool().

  @param[out]  Databases  On success, receives pool-allocated copies of the `db` and `dbx`
                          variable contents. Either Db or Dbx may be NULL (with a 0 size) if the
                          corresponding variable is absent.

  @retval EFI_SUCCESS            Databases loaded. Db / Dbx may still be NULL if the
                                 corresponding variable was absent.
  @retval EFI_INVALID_PARAMETER  Databases is NULL.
  @retval Other                  Failure status from gRT->GetVariable.
**/
EFI_STATUS
LoadSignatureDatabases (
  OUT SIGNATURE_DATABASES  *Databases
  )
{
  EFI_STATUS  Status;

  if (Databases == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Databases->Db      = NULL;
  Databases->DbSize  = 0;
  Databases->Dbx     = NULL;
  Databases->DbxSize = 0;

  Status = LoadSignatureDatabase (EFI_IMAGE_SECURITY_DATABASE, &Databases->Db, &Databases->DbSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to load db - %r\n", Status));
    goto Error;
  }

  Status = LoadSignatureDatabase (EFI_IMAGE_SECURITY_DATABASE1, &Databases->Dbx, &Databases->DbxSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to load dbx - %r\n", Status));
    goto Error;
  }

  return EFI_SUCCESS;

Error:
  if (Databases->Db != NULL) {
    FreePool (Databases->Db);
    Databases->Db     = NULL;
    Databases->DbSize = 0;
  }

  if (Databases->Dbx != NULL) {
    FreePool (Databases->Dbx);
    Databases->Dbx     = NULL;
    Databases->DbxSize = 0;
  }

  return Status;
}

/**
  Search the database for a digest authority that matches the image.

  A match is defined as a hash in the database that matches the hash of the image being searched.
  The hash algorithm used is determined by the SignatureType of each EFI_SIGNATURE_LIST in the
  database.

  @param[in]      Database       The raw database contents.
  @param[in]      DatabaseSize   The size of the Database in bytes.
  @param[in, out] Cache          DIGEST_CACHE pointer bound to the image being searched. The cache
                                 may be updated during the search.
  @param[out]     Authority      Only valid on EFI_SUCCESS; reflects if a matching entry was found.
                                 NULL if no match; otherwise, it contains the matching entry.

  @retval EFI_SUCCESS            Search completed; Authority reflects if a matching entry was found.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL, or Cache is not bound to an image.
  @retval EFI_VOLUME_CORRUPTED   Database is structurally malformed.
  @retval other                  Propagated from GetHash.
**/
EFI_STATUS
GetImageDigestAuthority (
  IN     CONST VOID       *Database,
  IN     UINTN            DatabaseSize,
  IN OUT DIGEST_CACHE     *Cache,
  OUT    IMAGE_AUTHORITY  *Authority
  )
{
  EFI_STATUS                Status;
  SIG_DATABASE_ITER         DbIter;
  SIG_LIST_ITER             ListIter;
  CONST EFI_SIGNATURE_LIST  *List;
  CONST EFI_SIGNATURE_DATA  *Entry;
  CONST UINT8               *Digest;
  UINTN                     DigestSize;

  if ((Cache == NULL) || (Authority == NULL) ||
      (Cache->Buffer == NULL) || (Cache->BufferSize == 0) ||
      (Cache->Type != DigestCacheTypeImage))
  {
    return EFI_INVALID_PARAMETER;
  }

  Authority->Data = NULL;
  Authority->Size = 0;
  ZeroMem (&Authority->SignatureType, sizeof (EFI_GUID));

  if ((DatabaseSize == 0) || (Database == NULL)) {
    return EFI_SUCCESS;
  }

  Status = DatabaseIterInit (&DbIter, Database, DatabaseSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Iterate over each EFI_SIGNATURE_LIST in the database.
  //
  while ((List = DatabaseIterNext (&DbIter)) != NULL) {
    Status = GetHash (
               &List->SignatureType,
               Cache,
               &Digest,
               &DigestSize
               );

    //
    // Unsupported hash type; skip this list.
    //
    if (Status == EFI_UNSUPPORTED) {
      continue;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to get image hash - %r\n", Status));
      return Status;
    }

    if (EFI_ERROR (SigListIterInit (&ListIter, List))) {
      continue;
    }

    //
    // Iterate over each Entry in the current EFI_SIGNATURE_LIST.
    //
    while ((Entry = SigListIterNext (&ListIter)) != NULL) {
      if (CompareMem (Entry->SignatureData, Digest, DigestSize) == 0) {
        Authority->Data = Entry;
        Authority->Size = List->SignatureSize;
        CopyGuid (&Authority->SignatureType, &List->SignatureType);
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Determine whether a TBS Certificate hash is present in the `dbx`.

  Iterates over the EFI_SIGNATURE_LISTs in the `dbx` database and checks if any of them contain
  the hash of the TBS Certificate. Any failure to iterate will assume the certificate is in the DBX.

  @param[in]  Cert      DER-encoded X.509 certificate.
  @param[in]  CertSize  BufferSize of Cert in bytes.
  @param[in]  Dbx       Raw dbx contents, or NULL.
  @param[in]  DbxSize   BufferSize of Dbx in bytes; 0 when Dbx is NULL.

  @retval TRUE   The certificate hash was located in dbx, or an error
                 prevented a definitive answer.
  @retval FALSE  The certificate hash is not present in dbx.
**/
BOOLEAN
IsTBSCertHashInDbx (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST VOID   *Dbx,
  IN  UINTN        DbxSize
  )
{
  EFI_STATUS                Status;
  UINTN                     DigestSize;
  DIGEST_CACHE              HashCache;
  SIG_DATABASE_ITER         Iter;
  SIG_LIST_ITER             ListIter;
  CONST EFI_SIGNATURE_LIST  *List;
  CONST EFI_SIGNATURE_DATA  *Entry;
  CONST UINT8               *CertDigest;

  //
  // There is no DBX, so it's definitely not in the DBX.
  //
  if ((Dbx == NULL) || (DbxSize == 0)) {
    return FALSE;
  }

  if (EFI_ERROR (DatabaseIterInit (&Iter, Dbx, DbxSize))) {
    DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: dbx is malformed; treating cert as revoked.\n"));
    return TRUE;
  }

  ZeroMem (&HashCache, sizeof (HashCache));
  HashCache.Type       = DigestCacheTypeX509;
  HashCache.Buffer     = Cert;
  HashCache.BufferSize = CertSize;

  while ((List = DatabaseIterNext (&Iter)) != NULL) {
    Status = GetHash (&List->SignatureType, &HashCache, &CertDigest, &DigestSize);

    //
    // This EFI_SIGNATURE_LIST in the DBX is not applicable to X509 certicicates.
    //
    if (Status == EFI_UNSUPPORTED) {
      continue;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: failed to compute X509 hash (%r).\n", Status));
      return TRUE;
    }

    if (List->SignatureSize < sizeof (EFI_GUID) + DigestSize) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: malformed dbx signature list size.\n"));
      return TRUE;
    }

    if (EFI_ERROR (SigListIterInit (&ListIter, List))) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: malformed dbx signature list.\n"));
      return TRUE;
    }

    while ((Entry = SigListIterNext (&ListIter)) != NULL) {
      if (CompareMem (Entry->SignatureData, CertDigest, DigestSize) == 0) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
  Extract the DER-encoded PKCS#7 SignedData payload from a single WIN_CERTIFICATE entry.

  @param[in]   Cert          The certificate to inspect.
  @param[out]  AuthData      On success, set to point at the PKCS#7 payload inside Cert.
  @param[out]  AuthDataSize  On success, set to the PKCS#7 payload length in bytes.

  @retval EFI_SUCCESS            AuthData/AuthDataSize were populated.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_UNSUPPORTED        Unsupported WIN_CERTIFICATE type.
  @retval EFI_VOLUME_CORRUPTED   dwLength is too small to contain the required header for the
                                 declared type.
**/
EFI_STATUS
GetWinCertificatePkcs7AuthData (
  IN  CONST WIN_CERTIFICATE  *Cert,
  OUT CONST UINT8            **AuthData,
  OUT UINTN                  *AuthDataSize
  )
{
  CONST WIN_CERTIFICATE_UEFI_GUID  *UefiGuidCert;

  if ((Cert == NULL) || (AuthData == NULL) || (AuthDataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Cert->wCertificateType) {
    case WIN_CERT_TYPE_PKCS_SIGNED_DATA:
      //
      // The certificate is a bare DER-encoded PKCS#7 SignedData prefixed
      // by the WIN_CERTIFICATE header.
      //
      if (Cert->dwLength <= sizeof (WIN_CERTIFICATE)) {
        return EFI_VOLUME_CORRUPTED;
      }

      *AuthData     = (CONST UINT8 *)Cert + sizeof (WIN_CERTIFICATE);
      *AuthDataSize = Cert->dwLength - sizeof (WIN_CERTIFICATE);
      return EFI_SUCCESS;

    case WIN_CERT_TYPE_EFI_GUID:
      //
      // The certificate is a WIN_CERTIFICATE_UEFI_GUID; the embedded
      // payload format is identified by CertType. Only the PKCS#7
      // SignedData GUID is supported.
      //
      if (Cert->dwLength <= OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)) {
        return EFI_VOLUME_CORRUPTED;
      }

      UefiGuidCert = (CONST WIN_CERTIFICATE_UEFI_GUID *)Cert;
      if (!CompareGuid (&UefiGuidCert->CertType, &gEfiCertPkcs7Guid)) {
        return EFI_UNSUPPORTED;
      }

      *AuthData     = UefiGuidCert->CertData;
      *AuthDataSize = Cert->dwLength - OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
      return EFI_SUCCESS;

    default:
      return EFI_UNSUPPORTED;
  }
}

/**
  Determine whether a PKCS#7 signature is authorized by an `EFI_CERT_X509_GUID`
  `EFI_SIGNATURE_LIST`.

  Iterates over the X.509 trust anchors carried in `List`, asks `AuthenticodeVerify` whether any
  one of them verifies the signature, and rejects an otherwise verifying anchor whose TBS hash is
  enrolled in `dbx`.

  The caller is responsible for confirming `List->SignatureType == gEfiCertX509Guid` before
  invocation.

  @param[in]  List           Candidate list of X.509 certificates (an EFI_SIGNATURE_LIST).
  @param[in]  AuthData       DER-encoded PKCS#7 SignedData.
  @param[in]  AuthDataSize   BufferSize of AuthData in bytes.
  @param[in]  ImageHash      Authenticode digest of the image.
  @param[in]  ImageHashSize  BufferSize of ImageHash in bytes.
  @param[in]  Dbx            Raw dbx contents, or NULL.
  @param[in]  DbxSize        BufferSize of Dbx in bytes; 0 when Dbx is NULL.
  @param[out] Authority      On a match, Authority->Data is the verifying EFI_SIGNATURE_DATA entry
                             and Authority->Size is List->SignatureSize.

  @retval TRUE   At least one entry verifies AuthData and is not revoked.
  @retval FALSE  No entry verifies AuthData (or all that do are revoked).
**/
STATIC
BOOLEAN
IsPkcs7AuthDataAuthorizedByX509List (
  IN  CONST EFI_SIGNATURE_LIST  *List,
  IN  CONST UINT8               *AuthData,
  IN  UINTN                     AuthDataSize,
  IN  CONST UINT8               *ImageHash,
  IN  UINTN                     ImageHashSize,
  IN  CONST VOID                *Dbx,
  IN  UINTN                     DbxSize,
  OUT IMAGE_AUTHORITY           *Authority
  )
{
  SIG_LIST_ITER             Iter;
  CONST EFI_SIGNATURE_DATA  *Entry;
  CONST UINT8               *TrustedCert;
  UINTN                     TrustedCertSize;
  UINT8                     *TBSCert;
  UINTN                     TBSCertSize;

  //
  // If the signature size is less than or equal to an EFI_GUID there
  // is no cert payload to inspect.
  //
  if (List->SignatureSize <= sizeof (EFI_GUID)) {
    return FALSE;
  }

  if (EFI_ERROR (SigListIterInit (&Iter, List))) {
    return FALSE;
  }

  TrustedCertSize = List->SignatureSize - sizeof (EFI_GUID);

  while ((Entry = SigListIterNext (&Iter)) != NULL) {
    TrustedCert = Entry->SignatureData;

    if (!AuthenticodeVerify (
           AuthData,
           AuthDataSize,
           TrustedCert,
           TrustedCertSize,
           ImageHash,
           ImageHashSize
           ))
    {
      continue;
    }

    if (!X509GetTBSCert (TrustedCert, TrustedCertSize, &TBSCert, &TBSCertSize)) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: X509GetTBSCert failed; treating cert as revoked.\n"));
      return FALSE;
    }

    //
    // The Authenticode signature is a valid trust anchor; make sure its not revoked
    //
    if (IsTBSCertHashInDbx (TrustedCert, TrustedCertSize, Dbx, DbxSize)) {
      DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: signing cert hash present in dbx; rejected.\n"));
      continue;
    }

    Authority->Data = Entry;
    Authority->Size = List->SignatureSize;

    return TRUE;
  }

  return FALSE;
}

/**
  Determine whether a PKCS#7 signature is authorized by an `EFI_CERT_X509_<HASH>_GUID`
  `EFI_SIGNATURE_LIST`.

  Each entry in `List` is a precomputed TBS-cert hash. For each entry, attempt to locate a signer
  in `AuthData` whose TBS hash (under the algorithm identified by `List->SignatureType`) matches
  the entry, then confirm the matched TBS hash is not enrolled in `dbx`.

  The caller is responsible for confirming `List->SignatureType` is one of the
  `EFI_CERT_X509_SHA{256,384,512}_GUID` values (see `IsX509CertHashGuid`) before invocation.

  @param[in]  List           Candidate list of TBS-cert hashes (an EFI_SIGNATURE_LIST).
  @param[in]  AuthData       DER-encoded PKCS#7 SignedData.
  @param[in]  AuthDataSize   BufferSize of AuthData in bytes.
  @param[in]  ImageHash      Authenticode digest of the image.
  @param[in]  ImageHashSize  BufferSize of ImageHash in bytes.
  @param[in]  Dbx            Raw dbx contents, or NULL.
  @param[in]  DbxSize        BufferSize of Dbx in bytes; 0 when Dbx is NULL.
  @param[out] Authority      On a match, Authority->Data is the matching EFI_SIGNATURE_DATA entry
                             and Authority->Size is List->SignatureSize.
  @param[in,out] TrustAnchorCacheHandle  Caller-owned cache handle pointer passed through to
                                         GetTrustAnchorX509FromAuthData for reuse across list entries.

  @retval TRUE   At least one entry matches a non-revoked signer TBS hash.
  @retval FALSE  No entry matches, all matches are revoked, or the list could not be parsed.
**/
STATIC
BOOLEAN
IsPkcs7AuthDataAuthorizedByX509HashList (
  IN  CONST EFI_SIGNATURE_LIST  *List,
  IN  CONST UINT8               *AuthData,
  IN  UINTN                     AuthDataSize,
  IN  CONST UINT8               *ImageHash,
  IN  UINTN                     ImageHashSize,
  IN  CONST VOID                *Dbx,
  IN  UINTN                     DbxSize,
  OUT IMAGE_AUTHORITY           *Authority,
  IN OUT VOID                   **TrustAnchorCacheHandle
  )
{
  EFI_STATUS                Status;
  SIG_LIST_ITER             Iter;
  CONST EFI_SIGNATURE_DATA  *Entry;
  UINTN                     TbsCertHashSize;
  UINT8                     *TrustAnchorX509;
  UINTN                     TrustAnchorX509Size;
  BOOLEAN                   Authorized;

  (VOID)ImageHash;
  (VOID)ImageHashSize;

  if ((List == NULL) || (AuthData == NULL) || (AuthDataSize == 0) ||
      (Authority == NULL) || (TrustAnchorCacheHandle == NULL))
  {
    return FALSE;
  }

  if (!IsX509CertHashGuid (&List->SignatureType)) {
    return FALSE;
  }

  if (List->SignatureSize <= sizeof (EFI_GUID)) {
    return FALSE;
  }

  if (EFI_ERROR (SigListIterInit (&Iter, List))) {
    return FALSE;
  }

  TbsCertHashSize = List->SignatureSize - sizeof (EFI_GUID);
  TrustAnchorX509 = NULL;
  Authorized      = FALSE;

  while ((Entry = SigListIterNext (&Iter)) != NULL) {
    TrustAnchorX509Size = 0;

    Status = GetTrustAnchorX509FromAuthData (
               TrustAnchorCacheHandle,
               Entry->SignatureData,
               TbsCertHashSize,
               AuthData,
               AuthDataSize,
               &TrustAnchorX509,
               &TrustAnchorX509Size
               );

    if (Status == EFI_NOT_FOUND) {
      continue;
    }

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: Trust Anchor found, checking TBS cert hash in dbx.\n"));
    if (IsTBSCertHashInDbx (TrustAnchorX509, TrustAnchorX509Size, Dbx, DbxSize)) {
      DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: Trust Anchor TBS cert hash found in dbx, skipping.\n"));
      FreePool (TrustAnchorX509);
      TrustAnchorX509 = NULL;
      continue;
    }

    Authority->Data = Entry;
    Authority->Size = List->SignatureSize;
    Authorized      = TRUE;
    goto Done;
  }

Done:
  if (TrustAnchorX509 != NULL) {
    FreePool (TrustAnchorX509);
  }

  return Authorized;
}

/**
  Determine whether the auth data (PKCS#7 SignedData) is revoked by the `dbx`.

  Runs two checks against the auth data (PKCS#7 SignedData):
  1. If a X.509 trust anchor enrolled in the `dbx` authenticates the signature, the certificate is
     revoked.
  2. if a signer in the signing chain (as reported by `Pkcs7GetSigners`) has a TBS hash that is
     enrolled in the `dbx`, the certificate is revoked.

  @param[in]  AuthData       DER-encoded PKCS#7 SignedData payload.
  @param[in]  AuthDataSize   Size of AuthData in bytes; must be non-zero.
  @param[in]  ImageHash      Authenticode digest of the image under the algorithm
                             AuthData uses.
  @param[in]  ImageHashSize  Size of ImageHash in bytes; must be non-zero.
  @param[in]  Dbx            Raw `dbx` contents, or NULL.
  @param[in]  DbxSize        Size of Dbx in bytes; 0 when Dbx is NULL.

  @retval TRUE   The signature is revoked by `dbx`, or a missing/empty `AuthData`
                 or `ImageHash` prevented a safe determination (fail closed).
  @retval FALSE  The signature is not revoked by `dbx`, including when `dbx` is
                 absent or empty.
**/
BOOLEAN
IsCertRevoked (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        AuthDataSize,
  IN  CONST UINT8  *ImageHash,
  IN  UINTN        ImageHashSize,
  IN  CONST VOID   *Dbx,
  IN  UINTN        DbxSize
  )
{
  SIG_DATABASE_ITER         DbIter;
  CONST EFI_SIGNATURE_LIST  *List;
  SIG_LIST_ITER             ListIter;
  CONST EFI_SIGNATURE_DATA  *Entry;
  UINTN                     TrustedCertSize;
  UINT8                     *CertStack;
  UINTN                     CertStackSize;
  UINT8                     *TrustedCert;
  UINTN                     TrustedCertOutSize;
  UINT8                     CertNumber;
  CONST UINT8               *Walker;
  CONST UINT8               *StackEnd;
  UINTN                     Index;
  UINT32                    CertLen;
  UINT8                     *TBSCert;
  UINTN                     TBSCertSize;
  BOOLEAN                   Revoked;

  if ((AuthData == NULL) || (AuthDataSize == 0) ||
      (ImageHash == NULL) || (ImageHashSize == 0))
  {
    return TRUE;
  }

  if ((Dbx == NULL) || (DbxSize == 0)) {
    return FALSE;
  }

  //
  // Step 1: walk dbx and try AuthenticodeVerify against each X.509 trust anchor. Failure to walk
  // the `dbx` is fail-closed since we cannot make a safe determination.
  //
  if (EFI_ERROR (DatabaseIterInit (&DbIter, Dbx, DbxSize))) {
    DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: `dbx` malformed.\n"));
    return TRUE;
  }

  while ((List = DatabaseIterNext (&DbIter)) != NULL) {
    if (!CompareGuid (&List->SignatureType, &gEfiCertX509Guid)) {
      continue;
    }

    if (List->SignatureSize <= sizeof (EFI_GUID)) {
      continue;
    }

    if (EFI_ERROR (SigListIterInit (&ListIter, List))) {
      continue;
    }

    TrustedCertSize = List->SignatureSize - sizeof (EFI_GUID);

    while ((Entry = SigListIterNext (&ListIter)) != NULL) {
      if (AuthenticodeVerify (
            AuthData,
            AuthDataSize,
            Entry->SignatureData,
            TrustedCertSize,
            ImageHash,
            ImageHashSize
            ))
      {
        DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: image verifies against `dbx` X.509 entry.\n"));
        return TRUE;
      }
    }
  }

  //
  // Step 2: walk the PKCS#7 signer chain. If any signer's TBS hash is in the `dbx`, the certificate
  // is revoked.
  //
  CertStack          = NULL;
  CertStackSize      = 0;
  TrustedCert        = NULL;
  TrustedCertOutSize = 0;

  if (!Pkcs7GetSigners (
         AuthData,
         AuthDataSize,
         &CertStack,
         &CertStackSize,
         &TrustedCert,
         &TrustedCertOutSize
         ))
  {
    return FALSE;
  }

  Revoked = FALSE;

  if ((CertStack != NULL) && (CertStackSize > 0)) {
    StackEnd   = CertStack + CertStackSize;
    CertNumber = *CertStack;
    Walker     = CertStack + 1;

    //
    // Fail-closed for malformed signer stacks.
    //
    for (Index = 0; Index < CertNumber; Index++) {
      if ((UINTN)(StackEnd - Walker) < sizeof (UINT32)) {
        DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: IsCertRevoked: malformed signer stack length prefix.\n"));
        Revoked = TRUE;
        break;
      }

      CertLen = ReadUnaligned32 ((CONST UINT32 *)Walker);
      Walker += sizeof (UINT32);

      if ((CertLen == 0) || ((UINTN)(StackEnd - Walker) < CertLen)) {
        DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: IsCertRevoked: malformed signer cert payload.\n"));
        Revoked = TRUE;
        break;
      }

      if (!X509GetTBSCert (Walker, CertLen, &TBSCert, &TBSCertSize)) {
        DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: IsCertRevoked: Failed to get TBS certificate.\n"));
        Revoked = TRUE;
        break;
      }

      if (IsTBSCertHashInDbx (Walker, CertLen, Dbx, DbxSize)) {
        DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: IsCertRevoked: signer TBS hash present in dbx.\n"));
        Revoked = TRUE;
        break;
      }

      Walker += CertLen;
    }
  }

  if (CertStack != NULL) {
    Pkcs7FreeSigners (CertStack);
  }

  if (TrustedCert != NULL) {
    Pkcs7FreeSigners (TrustedCert);
  }

  return Revoked;
}

/**
  Determine whether the auth data (PKCS#7 SignedData) is allowed by the `db`.

  Iterates over the the EFI_SIGNATURE_LISTs in the `db` and validates the auth data one of two ways
  depending on the associated GUID of the list:

  1. `EFI_CERT_X509_GUID`: Checks each X.509 trust anchor in the list to see if it verifies the
  auth data with `AuthenticodeVerify`. If it does verify, The TBS certificate is extracted, hashed
  and checked against the `dbx`.
  2. EFI_CERT_X509_<HASH_ALGORITHM>_GUID: Uses each TBS cert hash in the list to attempt to extract
  the TBS certificate from the auth data. If found, the TBS certificate is hashed and checked
  against the `dbx`.

  @param[in]  AuthData       DER-encoded PKCS#7 SignedData payload.
  @param[in]  AuthDataSize   Size of AuthData in bytes; must be non-zero.
  @param[in]  ImageHash      Authenticode digest of the image under the algorithm
                             AuthData uses.
  @param[in]  ImageHashSize  Size of ImageHash in bytes; must be non-zero.
  @param[in]  Databases      The `db` / `dbx` signature databases to evaluate against.
  @param[out] Authority      On authorization, Authority->Data is the EFI_SIGNATURE_DATA trust
                             anchor in `db` that authorized the image and Authority->Size is its
                             SignatureSize. On no authorization, Authority->Data is NULL and
                             Authority->Size is 0.

  @retval TRUE   The signature authorizes the image.
  @retval FALSE  The signature does not authorize the image, a required pointer was NULL, or an
                 error prevented a definitive answer.
**/
BOOLEAN
IsCertAuthorized (
  IN  CONST UINT8                *AuthData,
  IN  UINTN                      AuthDataSize,
  IN  CONST UINT8                *ImageHash,
  IN  UINTN                      ImageHashSize,
  IN  CONST SIGNATURE_DATABASES  *Databases,
  OUT IMAGE_AUTHORITY            *Authority
  )
{
  SIG_DATABASE_ITER         Iter;
  CONST EFI_SIGNATURE_LIST  *List;
  VOID                      *TrustAnchorCacheHandle;
  BOOLEAN                   Authorized;

  if ((AuthData == NULL) || (AuthDataSize == 0) ||
      (ImageHash == NULL) || (ImageHashSize == 0) ||
      (Databases == NULL) || (Authority == NULL))
  {
    return FALSE;
  }

  Authority->Data        = NULL;
  Authority->Size        = 0;
  TrustAnchorCacheHandle = NULL;
  Authorized             = FALSE;

  if (EFI_ERROR (DatabaseIterInit (&Iter, Databases->Db, Databases->DbSize))) {
    return FALSE;
  }

  //
  // Iterate over each signature list in the database, dispatching to the helper
  // that matches the list's signature-type GUID.
  //
  while ((List = DatabaseIterNext (&Iter)) != NULL) {
    if (CompareGuid (&List->SignatureType, &gEfiCertX509Guid) &&
        IsPkcs7AuthDataAuthorizedByX509List (
          List,
          AuthData,
          AuthDataSize,
          ImageHash,
          ImageHashSize,
          Databases->Dbx,
          Databases->DbxSize,
          Authority
          ))
    {
      Authorized = TRUE;
      break;
    }

    if (IsX509CertHashGuid (&List->SignatureType) &&
        IsPkcs7AuthDataAuthorizedByX509HashList (
          List,
          AuthData,
          AuthDataSize,
          ImageHash,
          ImageHashSize,
          Databases->Dbx,
          Databases->DbxSize,
          Authority,
          &TrustAnchorCacheHandle
          ))
    {
      Authorized = TRUE;
      break;
    }
  }

  if (TrustAnchorCacheHandle != NULL) {
    FreeTrustAnchorX509Cache (TrustAnchorCacheHandle);
  }

  return Authorized;
}

/**
  Search the `db` for a trust anchor that authorizes the image via a single WIN_CERTIFICATE.

  A match is defined as a X.509 certificate in the `db` which:
  1. Verifies the auth data found in the WIN_CERTIFICATE against the hashed image digest.
  2. Whose TBS (To Be Signed) hash is not present in the `dbx`.

  @param[in]      Cert       The certificate to evaluate.
  @param[in,out]  Cache      Image digest cache bound to the image buffer; the cache may
                             memoize one digest per algorithm across calls.
  @param[in]      Databases  The `db` / `dbx` signature databases to evaluate against.
  @param[out]     Authority  Authority->SignatureType is the authenticode hash algorithm. On
                             EFI_SUCCESS, Authority->Data references the EFI_SIGNATURE_DATA
                             trust anchor in the `db` that authorized the image.

  @retval EFI_SUCCESS            The certificate authorizes the image; inspect Authority.
  @retval EFI_NOT_FOUND          The certificate is valid but no `db` trust anchor authorizes it.
  @retval EFI_ACCESS_DENIED      The certificate is revoked by `dbx`, or a prelude failure
                                 (PKCS#7 extraction, hash-algorithm lookup, or image-hash
                                 computation) prevented evaluation.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
**/
EFI_STATUS
GetImageCertAuthority (
  IN     CONST WIN_CERTIFICATE      *Cert,
  IN OUT DIGEST_CACHE               *Cache,
  IN     CONST SIGNATURE_DATABASES  *Databases,
  OUT    IMAGE_AUTHORITY            *Authority
  )
{
  EFI_STATUS   Status;
  CONST UINT8  *AuthData;
  UINTN        AuthDataSize;
  EFI_GUID     HashType;
  CONST UINT8  *ImageHash;
  UINTN        ImageHashSize;

  if ((Cert == NULL) || (Cache == NULL) || (Databases == NULL) || (Authority == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Authority->Data = NULL;
  Authority->Size = 0;
  ZeroMem (&Authority->SignatureType, sizeof (EFI_GUID));

  Status = GetWinCertificatePkcs7AuthData (Cert, &AuthData, &AuthDataSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "DxeImageVerificationLib: Unable to extract PKCS#7 authentication data from WIN_CERTIFICATE (type=0x%04x, status=%r).\n",
      Cert->wCertificateType,
      Status
      ));
    return EFI_ACCESS_DENIED;
  }

  Status = GetAuthenticodeHashAlgorithm (AuthData, AuthDataSize, &HashType);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "DxeImageVerificationLib: Unable to determine the hash algorithm from the PKCS#7 authentication data (%r).\n",
      Status
      ));
    return EFI_ACCESS_DENIED;
  }

  Status = GetHash (&HashType, Cache, &ImageHash, &ImageHashSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "DxeImageVerificationLib: Unable to compute the image hash (type=%g, status=%r).\n",
      &HashType,
      Status
      ));
    return EFI_ACCESS_DENIED;
  }

  CopyGuid (&Authority->SignatureType, &HashType);

  if (IsCertRevoked (AuthData, AuthDataSize, ImageHash, ImageHashSize, Databases->Dbx, Databases->DbxSize)) {
    DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: certificate revoked by dbx.\n"));
    return EFI_ACCESS_DENIED;
  }

  //
  // IsCertAuthorized populates Authority->Data only when it authorizes the image; on a
  // non-authorizing result it leaves Data NULL while preserving SignatureType set above.
  //
  if (!IsCertAuthorized (
         AuthData,
         AuthDataSize,
         ImageHash,
         ImageHashSize,
         Databases,
         Authority
         ))
  {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
