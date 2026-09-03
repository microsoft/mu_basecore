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

//
// Context for MatchHashEntry visitor callback for the WalkDatabase function.
//
typedef struct {
  DIGEST_CACHE                *Cache;
  CONST SIGNATURE_TYPE_MAP    *Map;
  UINTN                       MapCount;
  BOOLEAN                     Truncated;
} MATCH_HASH_CONTEXT;

//
// Context for MatchCertEntry visitor callback for the WalkDatabase function.
//
typedef struct {
  CONST UINT8    *Cert;
  UINTN          CertSize;
} MATCH_CERT_CONTEXT;

/**
  WalkDatabase visitor: match the cache's digest against a hash-list entry.

  Uses the SignatureType to look up the hash algorithm to retrieve (or compute) the digest from
  the cache, then compare the digest against the entry.

  @param[in]      SignatureType  The list's SignatureType GUID.
  @param[in]      Entry          The current entry (raw bytes).
  @param[in]      EntrySize      The entry size (the list's SignatureSize).
  @param[in,out]  Context        A MATCH_HASH_CONTEXT.

  @retval WalkStop      The cache's digest matches this entry.
  @retval WalkSkipList  This list is not in the map, or its digest is uncomputable.
  @retval WalkContinue  This entry did not match; try the next.
**/
STATIC
WALK_ACTION
EFIAPI
MatchHashEntry (
  IN     CONST EFI_GUID  *SignatureType,
  IN     CONST VOID      *Entry,
  IN     UINTN           EntrySize,
  IN OUT VOID            *Context
  )
{
  MATCH_HASH_CONTEXT        *Ctx;
  CONST SIGNATURE_TYPE_MAP  *Match;
  CONST UINT8               *Target;
  UINTN                     TargetSize;
  UINTN                     PayloadSize;
  UINTN                     RevocationTimeSize;
  UINTN                     Index;
  EFI_STATUS                Status;

  Ctx   = (MATCH_HASH_CONTEXT *)Context;
  Match = NULL;

  //
  // Find the appropriate hash algorithm for this list's SignatureType or skip the list if it is
  // not in the map.
  //
  for (Index = 0; Index < Ctx->MapCount; Index++) {
    if (CompareGuid (SignatureType, Ctx->Map[Index].SignatureType)) {
      Match = &Ctx->Map[Index];
      break;
    }
  }

  if ((Match == NULL) || (EntrySize <= Match->OwnerSize)) {
    return WalkSkipList;
  }

  PayloadSize = EntrySize - Match->OwnerSize;

  Status = GetHash (Match->HashAlgorithm, Ctx->Cache, &Target, &TargetSize);
  if (EFI_ERROR (Status)) {
    Ctx->Truncated = TRUE;
    return WalkSkipList;
  }

  // Ensure the payload size matches the expected digest size. Must account for an appended
  // EFI_TIME for TBS-cert-hash V1 entries.
  RevocationTimeSize = 0;
  if ((Ctx->Map == mTbsHashSignatures) && (Match->OwnerSize == sizeof (EFI_GUID))) {
    RevocationTimeSize = sizeof (EFI_TIME);
  }

  if (PayloadSize != TargetSize + RevocationTimeSize) {
    return WalkSkipList;
  }

  if (CompareMem ((CONST UINT8 *)Entry + Match->OwnerSize, Target, TargetSize) == 0) {
    return WalkStop;
  }

  return WalkContinue;
}

/**
  WalkDatabase visitor: match a raw DER certificate against a full-certificate list entry.

  Considers only EFI_CERT_X509 (full-certificate) lists whose payload size equals the certificate,
  comparing the bytes exactly. All other lists are skipped.

  @param[in]      SignatureType  The list's SignatureType GUID.
  @param[in]      Entry          The current entry (raw bytes).
  @param[in]      EntrySize      The entry size (the list's SignatureSize).
  @param[in,out]  Context        A MATCH_CERT_CONTEXT.

  @retval WalkStop      The certificate matches this entry.
  @retval WalkSkipList  This list is not a matching full-certificate list.
  @retval WalkContinue  This entry did not match; try the next.
**/
STATIC
WALK_ACTION
EFIAPI
MatchCertEntry (
  IN     CONST EFI_GUID  *SignatureType,
  IN     CONST VOID      *Entry,
  IN     UINTN           EntrySize,
  IN OUT VOID            *Context
  )
{
  MATCH_CERT_CONTEXT        *Ctx;
  CONST SIGNATURE_TYPE_MAP  *Match;
  UINTN                     Index;

  Ctx   = (MATCH_CERT_CONTEXT *)Context;
  Match = NULL;

  //
  // Ensure the signature type is one that represents a full DER certificate or skip the list.
  //
  for (Index = 0; Index < ARRAY_SIZE (mX509CertSignatures); Index++) {
    if (CompareGuid (SignatureType, mX509CertSignatures[Index].SignatureType)) {
      Match = &mX509CertSignatures[Index];
      break;
    }
  }

  if ((Match == NULL) ||
      (EntrySize <= Match->OwnerSize) ||
      ((EntrySize - Match->OwnerSize) != Ctx->CertSize))
  {
    return WalkSkipList;
  }

  if (CompareMem ((CONST UINT8 *)Entry + Match->OwnerSize, Ctx->Cert, Ctx->CertSize) == 0) {
    return WalkStop;
  }

  return WalkContinue;
}

/**
  Determine whether the image digest bound to Cache is present in a `db`-style allow-list.

  @param[in,out]  Cache   Digest cache bound to the Authenticode image bytes.
  @param[in]      Db      Raw `db` contents, or NULL for an empty database.
  @param[in]      DbSize  Size of Db in bytes; 0 when Db is NULL.

  @retval TRUE   The image digest matches an image-hash entry in the valid prefix of Db.
  @retval FALSE  It is absent, or Cache is unusable.
**/
BOOLEAN
IsImageHashInDb (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *Db,
  IN     UINTN         DbSize
  )
{
  MATCH_HASH_CONTEXT  Ctx;

  //
  // An allow-list reports absent when the cache cannot be searched.
  //
  if ((Cache == NULL) || (Cache->Buffer == NULL) || (Cache->BufferSize == 0)) {
    return FALSE;
  }

  Ctx.Cache     = Cache;
  Ctx.Map       = mImageHashSignatures;
  Ctx.MapCount  = ARRAY_SIZE (mImageHashSignatures);
  Ctx.Truncated = FALSE;

  //
  // Honor the valid prefix and ignore truncation: a trailing malformed entry can only drop a
  // potential authorizer, never add one.
  //
  return WalkDatabase (Db, DbSize, MatchHashEntry, &Ctx, NULL);
}

/**
  Determine whether the image digest bound to Cache is present in a `dbx`-style deny-list.

  Fails closed: a malformed `dbx` or an uncomputable digest reports the image present.

  @param[in,out]  Cache    Digest cache bound to the Authenticode image bytes.
  @param[in]      Dbx      Raw `dbx` contents, or NULL for an empty database.
  @param[in]      DbxSize  Size of Dbx in bytes; 0 when Dbx is NULL.

  @retval TRUE   The image digest matches an image-hash entry, or the `dbx` could not be fully parsed.
  @retval FALSE  It is definitively absent (including an absent/empty Dbx).
**/
BOOLEAN
IsImageHashInDbx (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *Dbx,
  IN     UINTN         DbxSize
  )
{
  MATCH_HASH_CONTEXT  Ctx;
  BOOLEAN             Found;
  BOOLEAN             Truncated;

  //
  // A deny-list fails closed: an unsearchable cache reports the image present.
  //
  if ((Cache == NULL) || (Cache->Buffer == NULL) || (Cache->BufferSize == 0)) {
    return TRUE;
  }

  Ctx.Cache     = Cache;
  Ctx.Map       = mImageHashSignatures;
  Ctx.MapCount  = ARRAY_SIZE (mImageHashSignatures);
  Ctx.Truncated = FALSE;

  Found = WalkDatabase (Dbx, DbxSize, MatchHashEntry, &Ctx, &Truncated);

  //
  // Fail closed on any truncation: a structural break, or a supported list whose digest could not
  // be computed, might have hidden a match.
  //
  return (BOOLEAN)(Found || Truncated || Ctx.Truncated);
}

/**
  Determine whether the TBSCertificate digest bound to Cache is present in a `db`-style allow-list.

  @param[in,out]  Cache   Digest cache bound to a certificate's TBSCertificate bytes.
  @param[in]      Db      Raw `db` contents, or NULL for an empty database.
  @param[in]      DbSize  Size of Db in bytes; 0 when Db is NULL.

  @retval TRUE   The TBS digest matches a cert-hash entry in the valid prefix of Db.
  @retval FALSE  It is absent, or Cache is unusable.
**/
BOOLEAN
IsTbsHashInDb (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *Db,
  IN     UINTN         DbSize
  )
{
  MATCH_HASH_CONTEXT  Ctx;

  //
  // An allow-list reports absent when the cache cannot be searched.
  //
  if ((Cache == NULL) || (Cache->Buffer == NULL) || (Cache->BufferSize == 0)) {
    return FALSE;
  }

  Ctx.Cache     = Cache;
  Ctx.Map       = mTbsHashSignatures;
  Ctx.MapCount  = ARRAY_SIZE (mTbsHashSignatures);
  Ctx.Truncated = FALSE;

  //
  // Honor the valid prefix and ignore truncation: a trailing malformed entry can only drop a
  // potential authorizer, never add one.
  //
  return WalkDatabase (Db, DbSize, MatchHashEntry, &Ctx, NULL);
}

/**
  Determine whether the TBSCertificate digest bound to Cache is present in a `dbx`-style deny-list.

  Fails closed: a malformed `dbx` or an uncomputable digest reports the certificate present.

  @param[in,out]  Cache    Digest cache bound to a certificate's TBSCertificate bytes.
  @param[in]      Dbx      Raw `dbx` contents, or NULL for an empty database.
  @param[in]      DbxSize  Size of Dbx in bytes; 0 when Dbx is NULL.

  @retval TRUE   The TBS digest matches a cert-hash entry, or the `dbx` could not be fully parsed.
  @retval FALSE  It is definitively absent (including an absent/empty Dbx).
**/
BOOLEAN
IsTbsHashInDbx (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *Dbx,
  IN     UINTN         DbxSize
  )
{
  MATCH_HASH_CONTEXT  Ctx;
  BOOLEAN             Found;
  BOOLEAN             Truncated;

  //
  // A deny-list fails closed: an unsearchable cache reports the certificate present.
  //
  if ((Cache == NULL) || (Cache->Buffer == NULL) || (Cache->BufferSize == 0)) {
    return TRUE;
  }

  Ctx.Cache     = Cache;
  Ctx.Map       = mTbsHashSignatures;
  Ctx.MapCount  = ARRAY_SIZE (mTbsHashSignatures);
  Ctx.Truncated = FALSE;

  Found = WalkDatabase (Dbx, DbxSize, MatchHashEntry, &Ctx, &Truncated);

  //
  // Fail closed on any truncation: a structural break, or a supported list whose digest could not
  // be computed, might have hidden a match.
  //
  return (BOOLEAN)(Found || Truncated || Ctx.Truncated);
}

/**
  Determine whether a raw DER certificate is present in a `dbx`-style deny-list by exact match.

  Compares the certificate byte-for-byte against the EFI_CERT_X509 (full-certificate) lists. This
  covers identity revocation only; TBS-cert-hash revocation is a hash match handled by IsTbsHashInDbx.
  Fails closed: an unusable certificate or an un-parseable `dbx` reports the certificate present.

  @param[in]  Cert       DER-encoded certificate to search for.
  @param[in]  CertSize   Size of Cert in bytes.
  @param[in]  Dbx        Raw `dbx` contents, or NULL for an empty database.
  @param[in]  DbxSize    Size of Dbx in bytes; 0 when Dbx is NULL.

  @retval TRUE   The certificate matches an EFI_CERT_X509 entry, or the database could not be fully
                 parsed.
  @retval FALSE  The certificate is definitively absent (including an absent/empty Dbx).
**/
BOOLEAN
IsCertInDbx (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST VOID   *Dbx,
  IN  UINTN        DbxSize
  )
{
  MATCH_CERT_CONTEXT  Ctx;
  BOOLEAN             Found;
  BOOLEAN             Truncated;

  //
  // Deny-list: fail closed. An unusable certificate cannot be searched.
  //
  if ((Cert == NULL) || (CertSize == 0)) {
    return TRUE;
  }

  Ctx.Cert     = Cert;
  Ctx.CertSize = CertSize;

  Found = WalkDatabase (Dbx, DbxSize, MatchCertEntry, &Ctx, &Truncated);

  return (BOOLEAN)(Found || Truncated);
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
ExtractAuthData (
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
  Determine whether the verified certificate chain that authorizes an image is revoked by the
  `dbx`.

  Reports the chain revoked if any certificate in it - signer, intermediates, or the anchor - is
  enrolled in the `dbx` (by exact DER via IsCertInDbx or by TBS-cert hash via IsTbsHashInDbx).

  @param[in]  CertChain          EFI_CERT_STACK ordered signer..anchor.
  @param[in]  CertChainSize      Size of CertChain in bytes.
  @param[in]  Dbx                Raw dbx contents, or NULL.
  @param[in]  DbxSize            Size of Dbx in bytes; 0 when Dbx is NULL.

  @retval TRUE   A certificate in the chain is revoked, or the chain could not be parsed (fail
                 closed).
  @retval FALSE  No certificate in the chain is revoked, including when dbx is absent or empty.
**/
BOOLEAN
IsChainRevoked (
  IN  CONST UINT8  *CertChain,
  IN  UINTN        CertChainSize,
  IN  CONST VOID   *Dbx,
  IN  UINTN        DbxSize
  )
{
  CONST UINT8   *Walker;
  CONST UINT8   *StackEnd;
  UINT8         CertNumber;
  UINTN         Index;
  UINT32        CertLen;
  BOOLEAN       Revoked;
  DIGEST_CACHE  CertCache;
  UINT8         *Tbs;
  UINTN         TbsSize;

  //
  // With no dbx there is nothing to revoke against.
  //
  if ((Dbx == NULL) || (DbxSize == 0)) {
    return FALSE;
  }

  if ((CertChain == NULL) || (CertChainSize == 0)) {
    return TRUE;
  }

  Revoked    = FALSE;
  StackEnd   = CertChain + CertChainSize;
  CertNumber = *CertChain;
  Walker     = CertChain + 1;

  //
  // Walk the EFI_CERT_STACK (ordered signer..anchor). Any parse inconsistency is fail-closed.
  //
  for (Index = 0; Index < CertNumber; Index++) {
    if ((UINTN)(StackEnd - Walker) < sizeof (UINT32)) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: malformed certificate chain length prefix.\n"));
      Revoked = TRUE;
      break;
    }

    CertLen = ReadUnaligned32 ((CONST UINT32 *)Walker);
    Walker += sizeof (UINT32);

    if ((CertLen == 0) || ((UINTN)(StackEnd - Walker) < CertLen)) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: malformed certificate chain payload.\n"));
      Revoked = TRUE;
      break;
    }

    //
    // The cert-hash `dbx` entries hash the certificate's TBSCertificate, so pre-extract it and bind
    // the cache to those bytes. A fresh cache per certificate keeps memoized digests independent; a
    // certificate whose TBS cannot be extracted is fail-closed as revoked.
    //
    if (!X509GetTBSCert (Walker, CertLen, &Tbs, &TbsSize)) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: could not extract chain certificate TBS; treating as revoked.\n"));
      Revoked = TRUE;
      break;
    }

    ZeroMem (&CertCache, sizeof (CertCache));
    CertCache.Buffer     = Tbs;
    CertCache.BufferSize = TbsSize;

    //
    // A certificate is revoked either by identity (exact DER in an EFI_CERT_X509 list, matched
    // against the raw certificate) or by its TBSCertificate digest (a cert-hash list, matched via
    // the cache). Both are fail-closed. The per-certificate cache is released before the next cert.
    //
    Revoked = (BOOLEAN)(IsCertInDbx (Walker, CertLen, Dbx, DbxSize) ||
                        IsTbsHashInDbx (&CertCache, Dbx, DbxSize));

    FreeDigestCache (&CertCache);

    if (Revoked) {
      DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: chain certificate revoked by dbx.\n"));
      break;
    }

    Walker += CertLen;
  }

  return Revoked;
}

//
// Context for EvaluateAnchorEntry: the image's signature and hash, the databases (for `dbx`
// revocation), the lazily-allocated trust-anchor recovery cache handle (freed by the caller), and
// the evaluation record updated in place.
//
typedef struct {
  CONST UINT8                  *AuthData;
  UINTN                        AuthDataSize;
  CONST UINT8                  *ImageHash;
  UINTN                        ImageHashSize;
  CONST SIGNATURE_DATABASES    *Databases;
  VOID                         *CacheHandle;
  IMAGE_CERT_EVALUATION        *Evaluation;
} EVALUATE_ANCHOR_CONTEXT;

/**
  WalkDatabase visitor: walk the `db` for a non-revoked trust anchor that authorizes the image.

  For each certificate entry in the `db` (a full certificate, or a TBS-cert-hash entry whose anchor
  is recovered from the signature), attempt to verify the image. If it verifies, check if any certificate
  in the available chain (from signer to trust anchor) is revoked by the `dbx`. If none are revoked,
  record the authorizing certificate in the evaluation record and stop the walk. Otherwise continue
  the walk.

  @param[in]      SignatureType  The list's SignatureType GUID.
  @param[in]      Entry          The current entry (raw bytes).
  @param[in]      EntrySize      The entry size (the list's SignatureSize).
  @param[in,out]  Context        An EVALUATE_ANCHOR_CONTEXT.

  @retval WalkStop      The image was authorized (ImageCertApproved).
  @retval WalkSkipList  This list cannot authorize the image (unsupported, or an image-hash list).
  @retval WalkContinue  This entry did not authorize the image; try the next.
**/
STATIC
WALK_ACTION
EFIAPI
EvaluateAnchorEntry (
  IN     CONST EFI_GUID  *SignatureType,
  IN     CONST VOID      *Entry,
  IN     UINTN           EntrySize,
  IN OUT VOID            *Context
  )
{
  EVALUATE_ANCHOR_CONTEXT   *Ctx;
  EFI_STATUS                Status;
  UINT8                     *Anchor;
  UINTN                     AnchorSize;
  UINTN                     TbsHashSize;
  UINT8                     *CertChain;
  UINTN                     CertChainSize;
  UINTN                     OwnerSize;
  UINTN                     PayloadSize;
  UINTN                     Index;
  CONST SIGNATURE_TYPE_MAP  *Match;

  Ctx        = (EVALUATE_ANCHOR_CONTEXT *)Context;
  Anchor     = NULL;
  AnchorSize = 0;

  //
  // A trust anchor is either a full X.509 certificate or a TBS-cert-hash list (whose anchor is
  // recovered from the signature). Any other list is skipped.
  //
  Match = NULL;
  for (Index = 0; Index < ARRAY_SIZE (mX509CertSignatures); Index++) {
    if (CompareGuid (SignatureType, mX509CertSignatures[Index].SignatureType)) {
      Match = &mX509CertSignatures[Index];
      break;
    }
  }

  if (Match == NULL) {
    for (Index = 0; Index < ARRAY_SIZE (mTbsHashSignatures); Index++) {
      if (CompareGuid (SignatureType, mTbsHashSignatures[Index].SignatureType)) {
        Match = &mTbsHashSignatures[Index];
        break;
      }
    }
  }

  if ((Match == NULL) || (EntrySize <= Match->OwnerSize)) {
    return WalkSkipList;
  }

  OwnerSize   = Match->OwnerSize;
  PayloadSize = EntrySize - OwnerSize;

  if (Match->HashAlgorithm == NULL) {
    //
    // A NULL hash algorithm marks a full X.509 certificate list
    //
    Anchor     = (UINT8 *)Entry + OwnerSize;
    AnchorSize = PayloadSize;
  } else {
    //
    // Recover the trust anchor from the TBS-cert-hash entry.
    //
    TbsHashSize = PayloadSize;
    if (OwnerSize == sizeof (EFI_GUID)) {
      if (TbsHashSize <= sizeof (EFI_TIME)) {
        return WalkSkipList;
      }

      TbsHashSize -= sizeof (EFI_TIME);
    }

    Status = GetTrustAnchorX509FromAuthData (
               &Ctx->CacheHandle,
               (CONST UINT8 *)Entry + OwnerSize,
               TbsHashSize,
               Ctx->AuthData,
               Ctx->AuthDataSize,
               &Anchor,
               &AnchorSize
               );
    if (EFI_ERROR (Status)) {
      return WalkContinue;
    }
  }

  //
  // The candidate anchor authorizes the image only if it verifies the signature and none of the
  // certificates in its verified chain are revoked by the `dbx`.
  //
  CertChain     = NULL;
  CertChainSize = 0;
  Status        = AuthenticodeVerifyEx (
                    Ctx->AuthData,
                    Ctx->AuthDataSize,
                    Anchor,
                    AnchorSize,
                    Ctx->ImageHash,
                    Ctx->ImageHashSize,
                    &CertChain,
                    &CertChainSize
                    );
  if (!EFI_ERROR (Status)) {
    if (IsChainRevoked (CertChain, CertChainSize, Ctx->Databases->Dbx, Ctx->Databases->DbxSize)) {
      //
      // Verified but revoked: record only the verdict. A later anchor may still authorize the image
      // cleanly and replace this verdict.
      //
      Ctx->Evaluation->Verdict = ImageCertRevokedByDbx;
    } else {
      //
      // Authorized: record the authorizing certificate. BuildImageAuthority copies Anchor before it
      // is released below; the owner GUID comes from a V1 entry or is zeroed for a V2 entry.
      //
      Ctx->Evaluation->Verdict = ImageCertApproved;
      BuildImageAuthority (
        (OwnerSize == sizeof (EFI_GUID)) ? (CONST EFI_GUID *)Entry : NULL,
        Anchor,
        AnchorSize,
        &Ctx->Evaluation->Authority
        );
      CopyGuid (&Ctx->Evaluation->Authority.SignatureType, SignatureType);
    }
  }

  if (CertChain != NULL) {
    FreePool (CertChain);
  }

  //
  // Only the TBS-cert-hash path (a non-NULL hash algorithm) allocated the anchor; release it.
  //
  if (Match->HashAlgorithm != NULL) {
    FreePool (Anchor);
  }

  return (Ctx->Evaluation->Verdict == ImageCertApproved) ? WalkStop : WalkContinue;
}

/**
  Evaluate a single image WIN_CERTIFICATE against the `db` / `dbx` databases.

  Consolidates PKCS#7 extraction, image-hash computation, `db` authorization, and chain-relative
  `dbx` revocation into a single pass. The certificate authorizes the image when some `db` trust
  anchor verifies the image's signature (via AuthenticodeVerifyEx) and no certificate in the
  verified signer->anchor chain is enrolled in the `dbx` (see IsChainRevoked). Trust anchors are taken
  directly from `EFI_CERT_X509_GUID` `db` lists, or recovered from the signature for
  `EFI_CERT_X509_<HASH>_GUID` (TBS-cert-hash) lists via GetTrustAnchorX509FromAuthData.

  The EFI_STATUS return reports whether evaluation could be performed, not the security outcome:
  the outcome is reported in Evaluation->Verdict. An error return means evaluation could not be
  completed for a reason unrelated to the certificate's content.

  @param[in]      Cert        The WIN_CERTIFICATE to evaluate.
  @param[in,out]  Cache       Image digest cache bound to the image buffer; the cache may memoize
                              one digest per algorithm across calls.
  @param[in]      Databases   The `db` / `dbx` signature databases to evaluate against.
  @param[out]     Evaluation  On EFI_SUCCESS, receives the verdict and, for ImageCertApproved, the
                              authorizing certificate in Evaluation->Authority (for measurement).
                              Evaluation->Authority.Data is non-NULL only for ImageCertApproved; a
                              revoked or unauthorized image records no authority.
                              Evaluation->Authority.SignatureType is the authorizing `db` list's
                              signature type, set only for ImageCertApproved.

  @retval EFI_SUCCESS            Evaluation completed; inspect Evaluation->Verdict.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval other                  The image's Authenticode hash could not be computed (propagated
                                 from GetHash); no verdict was produced.
**/
EFI_STATUS
EvaluateImageCertificate (
  IN     CONST WIN_CERTIFICATE      *Cert,
  IN OUT DIGEST_CACHE               *Cache,
  IN     CONST SIGNATURE_DATABASES  *Databases,
  OUT    IMAGE_CERT_EVALUATION      *Evaluation
  )
{
  EFI_STATUS               Status;
  CONST UINT8              *AuthData;
  UINTN                    AuthDataSize;
  EFI_GUID                 HashAlgorithm;
  CONST UINT8              *ImageHash;
  UINTN                    ImageHashSize;
  EVALUATE_ANCHOR_CONTEXT  AnchorCtx;

  if ((Cert == NULL) || (Cache == NULL) || (Databases == NULL) || (Evaluation == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set default verdict to unusable so if we fail any parsing step we can simply return early.
  //
  Evaluation->Verdict        = ImageCertUnusable;
  Evaluation->Authority.Data = NULL;
  Evaluation->Authority.Size = 0;
  ZeroMem (&Evaluation->Authority.SignatureType, sizeof (EFI_GUID));

  Status = ExtractAuthData (Cert, &AuthData, &AuthDataSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: WIN_CERTIFICATE not usable (type=0x%04x, %r).\n", Cert->wCertificateType, Status));
    return EFI_SUCCESS;
  }

  Status = GetAuthenticodeHashAlgorithm (AuthData, AuthDataSize, &HashAlgorithm);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: unrecognized Authenticode hash algorithm (%r).\n", Status));
    return EFI_SUCCESS;
  }

  //
  // Compute (or reuse) the image's Authenticode hash. The EFI_HASH_ALGORITHM_* GUID from
  // GetAuthenticodeHashAlgorithm is passed straight to GetHash.
  //
  Status = GetHash (&HashAlgorithm, Cache, &ImageHash, &ImageHashSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to compute image hash (type=%g, %r).\n", &HashAlgorithm, Status));
    return Status;
  }

  //
  // Parsing passed, Update the default verdict.
  //
  Evaluation->Verdict = ImageCertNotInDb;

  //
  // Walk the `db` specifically for trust anchors (full cert or TBS-cert-hash entries). For each anchor, see if it verifies
  // the image. If verified, check the chain from signer to anchor against the `dbx` for revocation.
  //
  ZeroMem (&AnchorCtx, sizeof (AnchorCtx));
  AnchorCtx.AuthData      = AuthData;
  AnchorCtx.AuthDataSize  = AuthDataSize;
  AnchorCtx.ImageHash     = ImageHash;
  AnchorCtx.ImageHashSize = ImageHashSize;
  AnchorCtx.Databases     = Databases;
  AnchorCtx.Evaluation    = Evaluation;

  WalkDatabase (Databases->Db, Databases->DbSize, EvaluateAnchorEntry, &AnchorCtx, NULL);

  if (AnchorCtx.CacheHandle != NULL) {
    FreeTrustAnchorX509Cache (AnchorCtx.CacheHandle);
  }

  return EFI_SUCCESS;
}
