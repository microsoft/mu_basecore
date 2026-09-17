/** @file
  Secureboot allow-list / revoke-list (EFI_SIGNATURE_LIST[]) helpers and structural iterators,
  including signed-image (certificate / Authenticode) validation, for the DXE
  Image Verification Library.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Database.h"

//
// Iterator state for walking EFI_SIGNATURE_LIST records inside a signature-database buffer.
//
typedef struct {
  CONST UINT8    *Cursor;
  UINTN          Remaining;
} SIG_DATABASE_ITER;

//
// Iterator state for walking EFI_SIGNATURE_DATA entries inside a single EFI_SIGNATURE_LIST.
//
typedef struct {
  CONST UINT8    *Cursor;
  UINTN          Stride;
  UINTN          Remaining;
} SIG_LIST_ITER;

/**
  Initialize an iterator over the EFI_SIGNATURE_LIST records contained in a signature database
  buffer.

  The initialization validates the list and will truncate the iteration range to the
  last valid entry if the list if malformed.

  @param[out]  Iter        Iterator state to initialize.
  @param[in]   Buffer      Raw database contents, or NULL for an empty database.
  @param[in]   BufferSize  Size of Buffer in bytes; 0 when Buffer is NULL.

  @retval TRUE   The iterator covers every entry in the list.
  @retval FALSE  The iterator was truncated due to invalid arguments or a malformed table.
**/
BOOLEAN
DatabaseIterInit (
  OUT SIG_DATABASE_ITER  *Iter,
  IN  CONST VOID         *Buffer,
  IN  UINTN              BufferSize
  )
{
  CONST UINT8               *Cursor;
  UINTN                     Remaining;
  CONST EFI_SIGNATURE_LIST  *List;

  if (Iter == NULL) {
    return FALSE;
  }

  Iter->Cursor    = (CONST UINT8 *)Buffer;
  Iter->Remaining = 0;

  if (Buffer == NULL) {
    return (BOOLEAN)(BufferSize == 0);
  }

  Cursor    = (CONST UINT8 *)Buffer;
  Remaining = BufferSize;

  while (Remaining > 0) {
    if (Remaining < sizeof (EFI_SIGNATURE_LIST)) {
      break;
    }

    List = (CONST EFI_SIGNATURE_LIST *)(CONST VOID *)Cursor;
    if ((List->SignatureListSize < sizeof (EFI_SIGNATURE_LIST)) ||
        (List->SignatureListSize > Remaining))
    {
      break;
    }

    Cursor    += List->SignatureListSize;
    Remaining -= List->SignatureListSize;
  }

  Iter->Cursor    = (CONST UINT8 *)Buffer;
  Iter->Remaining = BufferSize - Remaining;
  return (BOOLEAN)(Remaining == 0);
}

/**
  Return the next EFI_SIGNATURE_LIST from the buffer being iterated.

  Infallible over the range established by DatabaseIterInit.

  @param[in,out]  Iter  Iterator initialized by DatabaseIterInit.

  @retval non-NULL  Pointer to the next EFI_SIGNATURE_LIST.
  @retval NULL      Iteration is complete.
**/
CONST EFI_SIGNATURE_LIST *
DatabaseIterNext (
  IN OUT SIG_DATABASE_ITER  *Iter
  )
{
  CONST EFI_SIGNATURE_LIST  *List;

  if ((Iter == NULL) || (Iter->Remaining == 0)) {
    return NULL;
  }

  List             = (CONST EFI_SIGNATURE_LIST *)(CONST VOID *)Iter->Cursor;
  Iter->Cursor    += List->SignatureListSize;
  Iter->Remaining -= List->SignatureListSize;
  return List;
}

/**
  Initialize an iterator over the EFI_SIGNATURE_DATA entries contained in a single
  EFI_SIGNATURE_LIST.

  The initialization validates the list and will truncate the iteration range to the
  last valid entry if the list if malformed.

  @param[out]  Iter  Iterator state to initialize.
  @param[in]   List  The signature list to walk.

  @retval TRUE   The iterator covers every entry in the list.
  @retval FALSE  The iterator was truncated due to invalid arguments or a malformed table.
**/
BOOLEAN
SigListIterInit (
  OUT SIG_LIST_ITER             *Iter,
  IN  CONST EFI_SIGNATURE_LIST  *List
  )
{
  UINTN  PayloadSize;

  if (Iter == NULL) {
    return FALSE;
  }

  Iter->Cursor    = NULL;
  Iter->Stride    = 0;
  Iter->Remaining = 0;

  if (List == NULL) {
    return FALSE;
  }

  if (List->SignatureListSize < sizeof (EFI_SIGNATURE_LIST)) {
    return FALSE;
  }

  if (List->SignatureSize < sizeof (EFI_GUID)) {
    return FALSE;
  }

  if (List->SignatureHeaderSize > List->SignatureListSize - sizeof (EFI_SIGNATURE_LIST)) {
    return FALSE;
  }

  PayloadSize = List->SignatureListSize
                - sizeof (EFI_SIGNATURE_LIST)
                - List->SignatureHeaderSize;

  Iter->Stride    = List->SignatureSize;
  Iter->Remaining = PayloadSize / List->SignatureSize;
  Iter->Cursor    = (CONST UINT8 *)List
                    + sizeof (EFI_SIGNATURE_LIST)
                    + List->SignatureHeaderSize;

  return (BOOLEAN)((PayloadSize % List->SignatureSize) == 0);
}

/**
  Return the next EFI_SIGNATURE_DATA entry from the list being iterated.

  Infallible over the range established by SigListIterInit.

  @param[in,out]  Iter  Iterator initialized by SigListIterInit.

  @retval non-NULL  Pointer to the next EFI_SIGNATURE_DATA entry.
  @retval NULL      Iteration is complete.
**/
CONST EFI_SIGNATURE_DATA *
SigListIterNext (
  IN OUT SIG_LIST_ITER  *Iter
  )
{
  CONST EFI_SIGNATURE_DATA  *Entry;

  if ((Iter == NULL) || (Iter->Remaining == 0)) {
    return NULL;
  }

  Entry         = (CONST EFI_SIGNATURE_DATA *)(CONST VOID *)Iter->Cursor;
  Iter->Cursor += Iter->Stride;
  Iter->Remaining--;
  return Entry;
}

/**
  Initialize an iterator over a packed table of WIN_CERTIFICATE records.

  The initialization validates the table and will truncate the iteration range to the last valid
  entry if the table is malformed.

  @param[out]  Iter             Iterator state to initialize.
  @param[in]   WinCertificates  The WIN_CERTIFICATE table, or NULL when there are none.
  @param[in]   Length           Length of the table in bytes; 0 when WinCertificates is NULL.

  @retval TRUE   The iterator covers every entry in the table.
  @retval FALSE  The iterator was truncated due to invalid arguments or a malformed table.
**/
BOOLEAN
WinCertIterInit (
  OUT WIN_CERT_ITER          *Iter,
  IN  CONST WIN_CERTIFICATE  *WinCertificates,
  IN  UINTN                  Length
  )
{
  CONST UINT8            *Cursor;
  UINTN                  Remaining;
  CONST WIN_CERTIFICATE  *Cert;
  UINTN                  EntrySize;

  if (Iter == NULL) {
    return FALSE;
  }

  Iter->Cursor    = NULL;
  Iter->Remaining = 0;

  if (WinCertificates == NULL) {
    return (BOOLEAN)(Length == 0);
  }

  Cursor    = (CONST UINT8 *)WinCertificates;
  Remaining = Length;

  while (Remaining > 0) {
    if (Remaining < sizeof (WIN_CERTIFICATE)) {
      break;
    }

    Cert = (CONST WIN_CERTIFICATE *)(CONST VOID *)Cursor;

    if ((Cert->dwLength < sizeof (WIN_CERTIFICATE)) ||
        (Cert->dwLength > Remaining))
    {
      break;
    }

    // Each entry is padded to an 8-byte boundary.
    EntrySize = ALIGN_VALUE (Cert->dwLength, 8);
    if (EntrySize > Remaining) {
      EntrySize = Remaining;
    }

    Cursor    += EntrySize;
    Remaining -= EntrySize;
  }

  Iter->Cursor    = (CONST UINT8 *)WinCertificates;
  Iter->Remaining = Length - Remaining;
  return (BOOLEAN)(Remaining == 0);
}

/**
  Return the next WIN_CERTIFICATE from the directory being iterated.

  Infallible over the range established by WinCertIterInit.

  @param[in,out]  Iter  Iterator initialized by WinCertIterInit.

  @retval non-NULL  Pointer to the next WIN_CERTIFICATE.
  @retval NULL      Iteration is complete.
**/
CONST WIN_CERTIFICATE *
WinCertIterNext (
  IN OUT WIN_CERT_ITER  *Iter
  )
{
  CONST WIN_CERTIFICATE  *Cert;
  UINTN                  EntrySize;

  if ((Iter == NULL) || (Iter->Remaining == 0)) {
    return NULL;
  }

  Cert      = (CONST WIN_CERTIFICATE *)(CONST VOID *)Iter->Cursor;
  EntrySize = ALIGN_VALUE (Cert->dwLength, 8);
  if (EntrySize > Iter->Remaining) {
    EntrySize = Iter->Remaining;
  }

  Iter->Cursor    += EntrySize;
  Iter->Remaining -= EntrySize;
  return Cert;
}

//
// WalkDatabase callback return options to control the iteration.
//
typedef enum {
  WalkContinue,   // Continue to the next entry in the current list.
  WalkSkipList,   // Skip the remaining entries of the current list; continue with the next list.
  WalkStop        // Stop the walk; WalkDatabase returns TRUE.
} WALK_ACTION;

/**
  Per-entry callback invoked by WalkDatabase for each entry of every EFI_SIGNATURE_LIST in a
  database's valid prefix.

  Each entry is passed to the visitor as an untyped pointer. The walker interprets it according
  according to the SignatureType GUID. After processing, the callback will return a WALK_ACTION
  value to control the iteration.

  @param[in]      SignatureType  The list's EFI_SIGNATURE_LIST SignatureType GUID.
  @param[in]      Entry          The current entry
  @param[in]      EntrySize      The entry size in bytes
  @param[in,out]  Context        Caller state threaded through the walk.

  @retval WalkContinue  Continue to the next entry in the current list.
  @retval WalkSkipList  Skip the rest of the current list and continue with the next list.
  @retval WalkStop      Stop the walk.
**/
typedef
WALK_ACTION
(EFIAPI *SIG_ENTRY_VISITOR)(
  IN     CONST EFI_GUID  *SignatureType,
  IN     CONST VOID      *Entry,
  IN     UINTN           EntrySize,
  IN OUT VOID            *Context
  );

/**
  Walk each entry of every EFI_SIGNATURE_LIST in a signature database, invoking the Visit callback for each entry.

  Visit has control of iteration based on the returned WALK_ACTION. It may continue to the next entry, skip the
  rest of the current list, or stop the walk entirely.

  @param[in]      Database      Raw database contents, or NULL for an empty database.
  @param[in]      DatabaseSize  Size of Database in bytes; 0 when Database is NULL.
  @param[in]      Visit         Per-entry callback. Required.
  @param[in,out]  Context       State threaded to Visit.
  @param[out]     Truncated     Optional. Set TRUE if a malformed database or list clamped the range.

  @retval TRUE   Visit returned WalkStop for some entry.
  @retval FALSE  The walk completed without Visit stopping it.
**/
STATIC
BOOLEAN
WalkDatabase (
  IN     CONST VOID         *Database,
  IN     UINTN              DatabaseSize,
  IN     SIG_ENTRY_VISITOR  Visit,
  IN OUT VOID               *Context,
  OUT    BOOLEAN            *Truncated   OPTIONAL
  )
{
  SIG_DATABASE_ITER         DbIter;
  SIG_LIST_ITER             ListIter;
  CONST EFI_SIGNATURE_LIST  *List;
  CONST EFI_SIGNATURE_DATA  *Entry;
  WALK_ACTION               Action;

  if (Truncated != NULL) {
    *Truncated = FALSE;
  }

  if ((Database == NULL) || (DatabaseSize == 0)) {
    return FALSE;
  }

  if (!DatabaseIterInit (&DbIter, Database, DatabaseSize) && (Truncated != NULL)) {
    *Truncated = TRUE;
  }

  while ((List = DatabaseIterNext (&DbIter)) != NULL) {
    if (!SigListIterInit (&ListIter, List) && (Truncated != NULL)) {
      *Truncated = TRUE;
    }

    while ((Entry = SigListIterNext (&ListIter)) != NULL) {
      Action = Visit (&List->SignatureType, Entry, List->SignatureSize, Context);
      if (Action == WalkStop) {
        return TRUE;
      }

      if (Action == WalkSkipList) {
        break;
      }
    }
  }

  return FALSE;
}

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
  Load the platform's db and dbx signature databases into a generic signature-list pair.

  The db and dbx buffers are allocated using AllocatePool(). The caller is responsible for freeing
  Lists->AllowList and Lists->RevokeList with FreePool().

  @param[out]  Lists  On success, receives db as the allow-list and dbx as the revoke-list. Either
                      list may be NULL (with a 0 size) if the corresponding variable is absent.

  @retval EFI_SUCCESS            db and dbx were loaded. Either list may still be NULL if the
                                 corresponding variable was absent.
  @retval EFI_INVALID_PARAMETER  Lists is NULL.
  @retval Other                  Failure status from gRT->GetVariable.
**/
EFI_STATUS
LoadDbAndDbx (
  OUT SIGNATURE_LISTS  *Lists
  )
{
  EFI_STATUS  Status;

  if (Lists == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Lists->AllowList      = NULL;
  Lists->AllowListSize  = 0;
  Lists->RevokeList     = NULL;
  Lists->RevokeListSize = 0;

  Status = LoadSignatureDatabase (
             EFI_IMAGE_SECURITY_DATABASE,
             &Lists->AllowList,
             &Lists->AllowListSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to load db - %r\n", Status));
    goto Error;
  }

  Status = LoadSignatureDatabase (
             EFI_IMAGE_SECURITY_DATABASE1,
             &Lists->RevokeList,
             &Lists->RevokeListSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to load dbx - %r\n", Status));
    goto Error;
  }

  return EFI_SUCCESS;

Error:
  if (Lists->AllowList != NULL) {
    FreePool (Lists->AllowList);
    Lists->AllowList     = NULL;
    Lists->AllowListSize = 0;
  }

  if (Lists->RevokeList != NULL) {
    FreePool (Lists->RevokeList);
    Lists->RevokeList     = NULL;
    Lists->RevokeListSize = 0;
  }

  return Status;
}

//
// Context for MatchHashEntry visitor callback for the WalkDatabase function.
//
typedef struct {
  DIGEST_CACHE                *Cache;      // The digest cache containing precomputed digests for comparison.
  CONST SIGNATURE_TYPE_MAP    *Map;        // An array of signature type mappings to search for in the database.
  UINTN                       MapCount;    // The number of elements in the Map array.
  BOOLEAN                     DigestError; // Set TRUE if the required digest could not be obtained.
} MATCH_HASH_CONTEXT;

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
  UINTN                     TrailingDataSize;
  UINTN                     Index;
  EFI_STATUS                Status;

  Ctx   = (MATCH_HASH_CONTEXT *)Context;
  Match = NULL;

  // Find the appropriate hash algorithm for this list's SignatureType or skip the list
  for (Index = 0; Index < Ctx->MapCount; Index++) {
    if (CompareGuid (SignatureType, Ctx->Map[Index].SignatureType)) {
      Match = &Ctx->Map[Index];
      break;
    }
  }

  if ((Match == NULL) || (EntrySize <= Match->OwnerSize)) {
    return WalkSkipList;
  }

  Status = GetHash (Match->HashAlgorithm, Ctx->Cache, &Target, &TargetSize);
  if (EFI_ERROR (Status)) {
    Ctx->DigestError = TRUE;
    return WalkSkipList;
  }

  // If this is the TBS-cert-hash V1 entry, we must account for the appended EFI_TIME in the payload size.
  TrailingDataSize = 0;
  if ((Ctx->Map == mTbsHashSignatures) && (Match->OwnerSize == sizeof (EFI_GUID))) {
    TrailingDataSize = sizeof (EFI_TIME);
  }

  if ((EntrySize - Match->OwnerSize) != TargetSize + TrailingDataSize) {
    return WalkSkipList;
  }

  if (CompareMem ((CONST UINT8 *)Entry + Match->OwnerSize, Target, TargetSize) == 0) {
    return WalkStop;
  }

  return WalkContinue;
}

//
// Context for MatchCertEntry visitor callback for the WalkDatabase function.
//
typedef struct {
  CONST UINT8    *Cert;     // Pointer to the raw DER certificate bytes.
  UINTN          CertSize;  // Size of the raw DER certificate in bytes.
} MATCH_CERT_CONTEXT;

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

  // Find the appropriate X509 signature type for this list's SignatureType.
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
  Determine whether the image digest bound to Cache is present in the allow-list

  @param[in,out]  Cache          Digest cache bound to the Authenticode image bytes.
  @param[in]      AllowList      Raw allow-list contents, or NULL for an empty list.
  @param[in]      AllowListSize  Size of AllowList in bytes; 0 when AllowList is NULL.

  @retval TRUE   The image digest matches an image-hash entry in the valid prefix of AllowList.
  @retval FALSE  It is absent, or Cache is unusable.
**/
BOOLEAN
IsImageHashInAllowList (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *AllowList,
  IN     UINTN         AllowListSize
  )
{
  MATCH_HASH_CONTEXT  Ctx;

  if ((Cache == NULL) || (Cache->Buffer == NULL) || (Cache->BufferSize == 0)) {
    return FALSE;
  }

  Ctx.Cache       = Cache;
  Ctx.Map         = mImageHashSignatures;
  Ctx.MapCount    = ARRAY_SIZE (mImageHashSignatures);
  Ctx.DigestError = FALSE;

  return WalkDatabase (AllowList, AllowListSize, MatchHashEntry, &Ctx, NULL);
}

/**
  Determine whether the image digest bound to Cache is present in the revoke-list

  A malformed revoke-list or an uncomputable digest reports the image present.

  @param[in,out]  Cache           Digest cache bound to the Authenticode image bytes.
  @param[in]      RevokeList      Raw revoke-list contents, or NULL for an empty list.
  @param[in]      RevokeListSize  Size of RevokeList in bytes; 0 when RevokeList is NULL.

  @retval TRUE   The image digest matches an image-hash entry, or the revoke-list could not be fully parsed.
  @retval FALSE  It is definitively absent (including an absent/empty RevokeList).
**/
BOOLEAN
IsImageHashInRevokeList (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *RevokeList,
  IN     UINTN         RevokeListSize
  )
{
  MATCH_HASH_CONTEXT  Ctx;
  BOOLEAN             Found;
  BOOLEAN             Truncated;

  if ((Cache == NULL) || (Cache->Buffer == NULL) || (Cache->BufferSize == 0)) {
    return TRUE;
  }

  Ctx.Cache       = Cache;
  Ctx.Map         = mImageHashSignatures;
  Ctx.MapCount    = ARRAY_SIZE (mImageHashSignatures);
  Ctx.DigestError = FALSE;

  Found = WalkDatabase (RevokeList, RevokeListSize, MatchHashEntry, &Ctx, &Truncated);

  // If the revoke-list was truncated or there was an error calculating a digest, treat the image as present.
  return (BOOLEAN)(Found || Truncated || Ctx.DigestError);
}

/**
  Determine whether the TBSCertificate digest bound to Cache is present in the allow-list.

  @param[in,out]  Cache          Digest cache bound to a certificate's TBSCertificate bytes.
  @param[in]      AllowList      Raw allow-list contents, or NULL for an empty list.
  @param[in]      AllowListSize  Size of AllowList in bytes; 0 when AllowList is NULL.

  @retval TRUE   The TBS digest matches a cert-hash entry in the valid prefix of AllowList.
  @retval FALSE  It is absent, or Cache is unusable.
**/
BOOLEAN
IsTbsHashInAllowList (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *AllowList,
  IN     UINTN         AllowListSize
  )
{
  MATCH_HASH_CONTEXT  Ctx;

  if ((Cache == NULL) || (Cache->Buffer == NULL) || (Cache->BufferSize == 0)) {
    return FALSE;
  }

  Ctx.Cache       = Cache;
  Ctx.Map         = mTbsHashSignatures;
  Ctx.MapCount    = ARRAY_SIZE (mTbsHashSignatures);
  Ctx.DigestError = FALSE;

  return WalkDatabase (AllowList, AllowListSize, MatchHashEntry, &Ctx, NULL);
}

/**
  Determine whether the TBSCertificate digest bound to Cache is present in the revoke-list.

  Fails closed: a malformed revoke-list or an uncomputable digest reports the certificate present.

  @param[in,out]  Cache           Digest cache bound to a certificate's TBSCertificate bytes.
  @param[in]      RevokeList      Raw revoke-list contents, or NULL for an empty list.
  @param[in]      RevokeListSize  Size of RevokeList in bytes; 0 when RevokeList is NULL.

  @retval TRUE   The TBS digest matches a cert-hash entry, or the revoke-list could not be fully parsed.
  @retval FALSE  It is definitively absent (including an absent/empty RevokeList).
**/
BOOLEAN
IsTbsHashInRevokeList (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *RevokeList,
  IN     UINTN         RevokeListSize
  )
{
  MATCH_HASH_CONTEXT  Ctx;
  BOOLEAN             Found;
  BOOLEAN             Truncated;

  if ((Cache == NULL) || (Cache->Buffer == NULL) || (Cache->BufferSize == 0)) {
    return TRUE;
  }

  Ctx.Cache       = Cache;
  Ctx.Map         = mTbsHashSignatures;
  Ctx.MapCount    = ARRAY_SIZE (mTbsHashSignatures);
  Ctx.DigestError = FALSE;

  Found = WalkDatabase (RevokeList, RevokeListSize, MatchHashEntry, &Ctx, &Truncated);

  // If the revoke-list was truncated or there was an error calculating a digest, treat the hash as present.
  return (BOOLEAN)(Found || Truncated || Ctx.DigestError);
}

/**
  Determine whether a raw DER certificate is present in a revoke-list by exact match.

  Compares the certificate byte-for-byte against the EFI_CERT_X509 (full-certificate) lists. This
  covers identity revocation only; TBS-cert-hash revocation is a hash match handled by
  IsTbsHashInRevokeList.
  Fails closed: an unusable certificate or an un-parseable revoke-list reports the certificate present.

  @param[in]  Cert            DER-encoded certificate to search for.
  @param[in]  CertSize        Size of Cert in bytes.
  @param[in]  RevokeList      Raw revoke-list contents, or NULL for an empty list.
  @param[in]  RevokeListSize  Size of RevokeList in bytes; 0 when RevokeList is NULL.

  @retval TRUE   The certificate matches an EFI_CERT_X509 entry, or the revoke-list could not be fully
                 parsed.
  @retval FALSE  The certificate is definitively absent (including an absent/empty RevokeList).
**/
BOOLEAN
IsCertInRevokeList (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST VOID   *RevokeList,
  IN  UINTN        RevokeListSize
  )
{
  MATCH_CERT_CONTEXT  Ctx;
  BOOLEAN             Found;
  BOOLEAN             Truncated;

  if ((Cert == NULL) || (CertSize == 0)) {
    return TRUE;
  }

  Ctx.Cert     = Cert;
  Ctx.CertSize = CertSize;

  Found = WalkDatabase (RevokeList, RevokeListSize, MatchCertEntry, &Ctx, &Truncated);

  return (BOOLEAN)(Found || Truncated);
}

/**
  Determine whether the verified certificate chain that authorizes an image is revoked.

  Reports the chain revoked if any certificate in it is enrolled in the revoke-list (by
  exact DER via IsCertInRevokeList or by TBS-cert hash via IsTbsHashInRevokeList).

  @param[in]  CertChain          EFI_CERT_STACK ordered signer..anchor.
  @param[in]  CertChainSize      Size of CertChain in bytes.
  @param[in]  RevokeList         Raw revoke-list contents, or NULL.
  @param[in]  RevokeListSize     Size of RevokeList in bytes; 0 when RevokeList is NULL.

  @retval TRUE   A certificate in the chain is revoked, or the chain could not be parsed (fail
                 closed).
  @retval FALSE  No certificate in the chain is revoked, including when the revoke-list is absent or empty.
**/
BOOLEAN
IsChainRevoked (
  IN  CONST UINT8  *CertChain,
  IN  UINTN        CertChainSize,
  IN  CONST VOID   *RevokeList,
  IN  UINTN        RevokeListSize
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

  if ((RevokeList == NULL) || (RevokeListSize == 0)) {
    return FALSE;
  }

  if ((CertChain == NULL) || (CertChainSize == 0)) {
    return TRUE;
  }

  Revoked    = FALSE;
  StackEnd   = CertChain + CertChainSize;
  CertNumber = *CertChain;
  Walker     = CertChain + 1;

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

    if (!X509GetTBSCert (Walker, CertLen, &Tbs, &TbsSize)) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: could not extract chain certificate TBS; treating as revoked.\n"));
      Revoked = TRUE;
      break;
    }

    ZeroMem (&CertCache, sizeof (CertCache));
    CertCache.Buffer     = Tbs;
    CertCache.BufferSize = TbsSize;

    Revoked = (BOOLEAN)(IsCertInRevokeList (Walker, CertLen, RevokeList, RevokeListSize) ||
                        IsTbsHashInRevokeList (&CertCache, RevokeList, RevokeListSize));

    FreeDigestCache (&CertCache);

    if (Revoked) {
      DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: chain certificate revoked by revoke-list.\n"));
      break;
    }

    Walker += CertLen;
  }

  return Revoked;
}

//
// Context for EvaluateAnchorEntry visitor callback for the WalkDatabase function.
//
typedef struct {
  CONST UINT8                   *SignatureData;    // Pointer to the Authenticode signature data
  UINTN                         SignatureDataSize; // Size of the Authenticode signature data
  CONST UINT8                   *ImageHash;        // Pointer to the Image's hash, matching the hash type
                                                   // specified in the certificate.
  UINTN                         ImageHashSize;     // Size of the image's hash
  CONST SIGNATURE_LISTS         *Lists;            // Pointer to the allow-list and revoke-list.
  VOID                          *CacheHandle;      // Pointer to an opaque cache structure used by GetTrustAnchorX509FromAuthData.
  IMAGE_SIGNATURE_EVALUATION    *Evaluation;       // Pointer to the structure containing the signature evaluation results.
} EVALUATE_ANCHOR_CONTEXT;

/**
  WalkDatabase visitor: walk the allow-list for a non-revoked trust anchor that authorizes the image.

  For each certificate entry in the allow-list (a full certificate, or a TBS-cert-hash entry whose anchor
  is recovered from the signature), attempt to verify the image. If it verifies, check if any certificate
  in the available chain (from signer to trust anchor) is revoked by the revoke-list. If none are revoked,
  record the authorizing certificate in the evaluation record and stop the walk. Otherwise continue
  the walk.

  @param[in]      SignatureType  The list's SignatureType GUID.
  @param[in]      Entry          The current entry (raw bytes).
  @param[in]      EntrySize      The entry size (the list's SignatureSize).
  @param[in,out]  Context        An EVALUATE_ANCHOR_CONTEXT.

  @retval WalkStop      The image was authorized (ImageSignatureAllowed).
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

  // Find a supported list type or skip the list. Supports a full X.509 certificate or a TBS-cert-hash list.
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
    // A NULL hash algorithm marks a full X.509 certificate list
    Anchor     = (UINT8 *)Entry + OwnerSize;
    AnchorSize = PayloadSize;
  } else {
    // Recover the trust anchor from the TBS-cert-hash entry.
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
               Ctx->SignatureData,
               Ctx->SignatureDataSize,
               &Anchor,
               &AnchorSize
               );
    if (EFI_ERROR (Status)) {
      return WalkContinue;
    }
  }

  CertChain     = NULL;
  CertChainSize = 0;
  Status        = AuthenticodeVerifyEx (
                    Ctx->SignatureData,
                    Ctx->SignatureDataSize,
                    Anchor,
                    AnchorSize,
                    Ctx->ImageHash,
                    Ctx->ImageHashSize,
                    &CertChain,
                    &CertChainSize
                    );
  if (!EFI_ERROR (Status)) {
    if (IsChainRevoked (
          CertChain,
          CertChainSize,
          Ctx->Lists->RevokeList,
          Ctx->Lists->RevokeListSize
          ))
    {
      Ctx->Evaluation->Verdict = ImageSignatureRevoked;
    } else {
      Ctx->Evaluation->Verdict = ImageSignatureAllowed;

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

  // Only the TBS-cert-hash path allocated the anchor; release it.
  if (Match->HashAlgorithm != NULL) {
    FreePool (Anchor);
  }

  return (Ctx->Evaluation->Verdict == ImageSignatureAllowed) ? WalkStop : WalkContinue;
}

/**
  Evaluate an Authenticode signature against an allow-list and a revoke-list.

  @param[in]      SignatureData      Authenticode signature data.
  @param[in]      SignatureDataSize  Size of SignatureData in bytes.
  @param[in,out]  Cache       Image digest cache bound to the image buffer; the cache may memoize
                              one digest per algorithm across calls.
  @param[in]      Lists       The allow-list and revoke-list to evaluate against.
  @param[out]     Evaluation  Evaluation data when EFI_SUCCESS is returned.

  @retval EFI_SUCCESS            Evaluation completed; inspect Evaluation->Verdict.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval other                  The image's Authenticode hash could not be computed (propagated
                                 from GetHash); no verdict was produced.
**/
EFI_STATUS
EvaluateSignature (
  IN     CONST UINT8                 *SignatureData,
  IN     UINTN                       SignatureDataSize,
  IN OUT DIGEST_CACHE                *Cache,
  IN     CONST SIGNATURE_LISTS       *Lists,
  OUT    IMAGE_SIGNATURE_EVALUATION  *Evaluation
  )
{
  EFI_STATUS               Status;
  EFI_GUID                 HashAlgorithm;
  CONST UINT8              *ImageHash;
  UINTN                    ImageHashSize;
  EVALUATE_ANCHOR_CONTEXT  AnchorCtx;

  if ((SignatureData == NULL) || (SignatureDataSize == 0) || (Cache == NULL) ||
      (Lists == NULL) || (Evaluation == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Set default verdict to unusable so if we fail any parsing step we can simply return early.
  Evaluation->Verdict        = ImageSignatureUnusable;
  Evaluation->Authority.Data = NULL;
  Evaluation->Authority.Size = 0;
  ZeroMem (&Evaluation->Authority.SignatureType, sizeof (EFI_GUID));

  Status = GetAuthenticodeHashAlgorithm (SignatureData, SignatureDataSize, &HashAlgorithm);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: unrecognized Authenticode hash algorithm (%r).\n", Status));
    return EFI_SUCCESS;
  }

  Status = GetHash (&HashAlgorithm, Cache, &ImageHash, &ImageHashSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to compute image hash (type=%g, %r).\n", &HashAlgorithm, Status));
    return Status;
  }

  Evaluation->Verdict = ImageSignatureNotAuthorized;

  ZeroMem (&AnchorCtx, sizeof (AnchorCtx));
  AnchorCtx.SignatureData     = SignatureData;
  AnchorCtx.SignatureDataSize = SignatureDataSize;
  AnchorCtx.ImageHash         = ImageHash;
  AnchorCtx.ImageHashSize     = ImageHashSize;
  AnchorCtx.Lists             = Lists;
  AnchorCtx.Evaluation        = Evaluation;

  // Walk the allow-list for trust anchors that verify the image and are not revoked.
  WalkDatabase (
    Lists->AllowList,
    Lists->AllowListSize,
    EvaluateAnchorEntry,
    &AnchorCtx,
    NULL
    );

  if (AnchorCtx.CacheHandle != NULL) {
    FreeTrustAnchorX509Cache (AnchorCtx.CacheHandle);
  }

  return EFI_SUCCESS;
}
