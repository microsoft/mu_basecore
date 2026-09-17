/** @file
  Secureboot allow-list / revoke-list (EFI_SIGNATURE_LIST[]) helpers and structural iterators,
  including signed-image (certificate / Authenticode) validation, for the DXE
  Image Verification Library.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include "DxeImageVerificationLib.h"
#include "Support.h"
#include <Library/UefiLib.h>
#include <Guid/WinCertificate.h>

/**
  Iterator state for walking WIN_CERTIFICATE records inside a PE/COFF image's security data
  directory.

  Fields are owned by the iterator implementation; callers should treat them as opaque.
**/
typedef struct {
  CONST UINT8    *Cursor;     // Next byte to consume.
  UINTN          Remaining;   // Bytes left in the certificate table.
} WIN_CERT_ITER;

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
  );

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
  );

//
// A generic allow-list and revoke-list pair. Both buffers are owned by the
// caller; either may be NULL when the corresponding list is absent, in which
// case its size is 0.
//
typedef struct {
  VOID     *AllowList;
  UINTN    AllowListSize;
  VOID     *RevokeList;
  UINTN    RevokeListSize;
} SIGNATURE_LISTS;

//
// The verdict from evaluating an Authenticode image signature against the
// allow-list and revoke-list.
//
typedef enum {
  //
  // An allow-list trust anchor authorized the image and no certificate in its
  // verified signer->anchor chain is revoked.
  //
  ImageSignatureAllowed,
  //
  // An allow-list trust anchor verified the image, but a certificate in its
  // verified chain is enrolled in the revoke-list. No un-revoked anchor
  // authorized the image.
  //
  ImageSignatureRevoked,
  //
  // No allow-list trust anchor verifies the image's signature.
  //
  ImageSignatureNotAuthorized,
  //
  // The Authenticode signature could not be evaluated: the payload is malformed, its
  // Authenticode hash algorithm is unrecognized, or another failure occurred
  // before trust-anchor evaluation.
  //
  ImageSignatureUnusable
} IMAGE_SIGNATURE_VERDICT;

//
// The result of EvaluateSignature: the evaluation verdict plus, for
// ImageSignatureAllowed, the `db` certificate that authorized the image (for
// measurement). A revoked or unauthorized image records no authority.
//
typedef struct {
  IMAGE_SIGNATURE_VERDICT    Verdict;
  IMAGE_AUTHORITY            Authority;
} IMAGE_SIGNATURE_EVALUATION;

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );
