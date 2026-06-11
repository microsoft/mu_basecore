/** @file
  Secureboot DB/DBX/DBT (EFI_SIGNATURE_LIST) helpers, including signed-image
  (certificate / Authenticode) validation, for the DXE Image Verification Library.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DXE_IMAGE_VERIFICATION_LIB_DATABASE_H_
#define DXE_IMAGE_VERIFICATION_LIB_DATABASE_H_

#include "DxeImageVerificationLib.h"
#include "Iterator.h"
#include "Support.h"
#include <Library/UefiLib.h>
#include <Guid/WinCertificate.h>

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
  );

/**
  Determine whether a TBS Certificate hash is present in the `dbx`.

  Iterates over the EFI_SIGNATURE_LISTs in the `dbx` database and checks if any of them contain
  the hash of the TBS Certificate.

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
  );

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
  );

#endif // DXE_IMAGE_VERIFICATION_LIB_DATABASE_H_
