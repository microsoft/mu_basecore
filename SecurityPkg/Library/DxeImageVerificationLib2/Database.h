/** @file
  Secureboot DB/DBX/DBT (EFI_SIGNATURE_LIST) helpers, including signed-image
  (certificate / Authenticode) validation, for the DXE Image Verification Library.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
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
// The platform's Secure Boot signature databases (`db` and `dbx`). Db / Dbx
// are pool-allocated copies owned by the caller; either may be NULL when the
// corresponding variable is absent, in which case its size is 0.
//
typedef struct {
  VOID     *Db;
  UINTN    DbSize;
  VOID     *Dbx;
  UINTN    DbxSize;
} SIGNATURE_DATABASES;

//
// The verdict from evaluating a single image WIN_CERTIFICATE against the `db`
// and `dbx` databases.
//
typedef enum {
  //
  // A `db` trust anchor authorized the image and no certificate in its verified
  // signer->anchor chain is revoked by the `dbx`.
  //
  ImageCertApproved,
  //
  // A `db` trust anchor verified the image, but a certificate in its verified
  // chain is enrolled in the `dbx`. No un-revoked anchor authorized the image.
  //
  ImageCertRevokedByDbx,
  //
  // No `db` trust anchor verifies the image's signature.
  //
  ImageCertNotInDb,
  //
  // The WIN_CERTIFICATE could not be evaluated: an unsupported certificate
  // type, a malformed PKCS#7 payload, an unrecognized Authenticode hash
  // algorithm, or another failure before trust-anchor evaluation.
  //
  ImageCertUnusable
} IMAGE_CERT_VERDICT;

//
// The result of EvaluateImageCertificate: the evaluation verdict plus, for
// ImageCertApproved, the `db` certificate that authorized the image (for
// measurement). A revoked or unauthorized image records no authority.
//
typedef struct {
  IMAGE_CERT_VERDICT    Verdict;
  IMAGE_AUTHORITY       Authority;
} IMAGE_CERT_EVALUATION;

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
  );

/**
  Determine whether the image digest bound to Cache is present in the allow-list

  @param[in,out]  Cache   Digest cache bound to the Authenticode image bytes.
  @param[in]      Db      Raw allow-list contents, or NULL for an empty database.
  @param[in]      DbSize  Size of Db in bytes; 0 when Db is NULL.

  @retval TRUE   The image digest matches an image-hash entry in the valid prefix of Db.
  @retval FALSE  It is absent, or Cache is unusable.
**/
BOOLEAN
IsImageHashInDb (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *Db,
  IN     UINTN         DbSize
  );

/**
  Determine whether the image digest bound to Cache is present in the revoke-list

  A malformed revoke-list or an uncomputable digest reports the image present.

  @param[in,out]  Cache    Digest cache bound to the Authenticode image bytes.
  @param[in]      Dbx      Raw revoke-list contents, or NULL for an empty database.
  @param[in]      DbxSize  Size of Dbx in bytes; 0 when Dbx is NULL.

  @retval TRUE   The image digest matches an image-hash entry, or the revoke-list could not be fully parsed.
  @retval FALSE  It is definitively absent (including an absent/empty Dbx).
**/
BOOLEAN
IsImageHashInDbx (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *Dbx,
  IN     UINTN         DbxSize
  );

/**
  Determine whether the TBSCertificate digest bound to Cache is present in the allow-list.

  @param[in,out]  Cache   Digest cache bound to a certificate's TBSCertificate bytes.
  @param[in]      Db      Raw allow-list contents, or NULL for an empty database.
  @param[in]      DbSize  Size of Db in bytes; 0 when Db is NULL.

  @retval TRUE   The TBS digest matches a cert-hash entry in the valid prefix of Db.
  @retval FALSE  It is absent, or Cache is unusable.
**/
BOOLEAN
IsTbsHashInDb (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *Db,
  IN     UINTN         DbSize
  );

/**
  Determine whether the TBSCertificate digest bound to Cache is present in the revoke-list.

  Fails closed: a malformed revoke-list or an uncomputable digest reports the certificate present.

  @param[in,out]  Cache    Digest cache bound to a certificate's TBSCertificate bytes.
  @param[in]      Dbx      Raw revoke-list contents, or NULL for an empty database.
  @param[in]      DbxSize  Size of Dbx in bytes; 0 when Dbx is NULL.

  @retval TRUE   The TBS digest matches a cert-hash entry, or the revoke-list could not be fully parsed.
  @retval FALSE  It is definitively absent (including an absent/empty Dbx).
**/
BOOLEAN
IsTbsHashInDbx (
  IN OUT DIGEST_CACHE  *Cache,
  IN     CONST VOID    *Dbx,
  IN     UINTN         DbxSize
  );

/**
  Determine whether a raw DER certificate is present in a revoke-list by exact match.

  Compares the certificate byte-for-byte against the EFI_CERT_X509 (full-certificate) lists. This
  covers identity revocation only; TBS-cert-hash revocation is a hash match handled by IsTbsHashInDbx.
  Fails closed: an unusable certificate or an un-parseable revoke-list reports the certificate present.

  @param[in]  Cert       DER-encoded certificate to search for.
  @param[in]  CertSize   Size of Cert in bytes.
  @param[in]  Dbx        Raw revoke-list contents, or NULL for an empty database.
  @param[in]  DbxSize    Size of Dbx in bytes; 0 when Dbx is NULL.

  @retval TRUE   The certificate matches an EFI_CERT_X509 entry, or the revoke-list could not be fully
                 parsed.
  @retval FALSE  The certificate is definitively absent (including an absent/empty Dbx).
**/
BOOLEAN
IsCertInDbx (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST VOID   *Dbx,
  IN  UINTN        DbxSize
  );

/**
  Evaluate a single WIN_CERTIFICATE against an allow-list and a revoke-list..

  @param[in]      Cert        The WIN_CERTIFICATE to evaluate.
  @param[in,out]  Cache       Image digest cache bound to the image buffer; the cache may memoize
                              one digest per algorithm across calls.
  @param[in]      Databases   The allow-list / revoke-list databases to evaluate against.
  @param[out]     Evaluation  Evaluation data when EFI_SUCCESS is returned.

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
  );
