/** @file
  Utility functions for DxeImageVerificationLib.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DXE_IMAGE_VERIFICATION_LIB_SUPPORT_H_
#define DXE_IMAGE_VERIFICATION_LIB_SUPPORT_H_

#include "DxeImageVerificationLib.h"

#include <Library/PeCoffLib.h>

//
// An enum to specify the type of data being cached in an instance of `DIGEST_CACHE`.
//
typedef enum {
  DigestCacheTypeImage,
  DigestCacheTypeX509
} DIGEST_CACHE_TYPE;

//
// A cached digest slot in `DIGEST_CACHE`. The slot's position within `DIGEST_CACHE::Entries`
// identifies the hash algorithm (ordering matches `mHashAlgorithms`).
//
// BufferSize == 0 marks an empty slot; any non-zero BufferSize identifies a valid cached digest
// for that particular hash algorithm (based on index compared to `mHashAlgorithms`).
//
typedef struct {
  UINT8    Bytes[MAX_DIGEST_SIZE];
  UINTN    BufferSize;
} DIGEST_CACHE_ENTRY;

//
// Caller-owned digest cache, one fixed slot per supported hash algorithm based on
// `mHashAlgorithms` order.
//
// The structure should be zero-initialized before use as the DigestSize in each entry is used to
// determine whether the slot contains a valid cached digest.
//
typedef struct {
  DIGEST_CACHE_TYPE     Type;
  CONST VOID            *Buffer;
  UINTN                 BufferSize;
  DIGEST_CACHE_ENTRY    Entries[ARRAY_SIZE (mHashAlgorithms)];
} DIGEST_CACHE;

/**
  Get or compute a cached digest for HashType.

  `Cache->Buffer` is the data to be hashed for cache miss while `Cache->Type` indicates how to
  compute the digest.

  Digest calculations are as follows:
  - DigestCacheTypeImage: compute using GetAuthenticodeHash() with the specified HashType.
  - DigestCacheTypeX509: compute using X509GetTbsCertHash() with the specified HashType.

  @param[in]      HashType     Signature-type GUID identifying the hash algorithm to use.
  @param[in,out]  Cache        Caller-owned digest cache bound to one buffer via Cache->Buffer /
                               Cache->BufferSize.
  @param[out]     Digest       On success, receives a pointer to the cached digest bytes. The
                               pointer is valid for the lifetime of Cache.
  @param[out]     DigestSize   On success, receives the digest length in bytes.

  @retval EFI_SUCCESS            Digest / DigestSize describe a valid cached digest.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_UNSUPPORTED        HashType does not map to an entry in mHashAlgorithms compatible
                                 with Cache->Type.
  @retval EFI_COMPROMISED_DATA   Cache already holds a digest for this slot but the stored size is invalid.
  @retval EFI_SECURITY_VIOLATION The hash operation failed.
  @retval other                  Forwarded from GetAuthenticodeHash or X509GetTbsCertHash.
**/
EFI_STATUS
GetHash (
  IN     CONST EFI_GUID  *HashType,
  IN OUT DIGEST_CACHE    *Cache,
  OUT    CONST UINT8     **Digest,
  OUT    UINTN           *DigestSize
  );

/**
  Locate the EFI_IMAGE_DIRECTORY_ENTRY_SECURITY data directory in the
  PE/COFF image contained in FileBuffer.

  Caution: This function may receive untrusted input. The PE/COFF image
  is external input and is bounds-checked by PeCoffLib before any field
  is dereferenced.

  @param[in]   FileBuffer  Pointer to the in-memory PE/COFF image.
  @param[in]   FileSize    BufferSize of FileBuffer in bytes.
  @param[out]  SecDataDir  On success, filled with a copy of the image's
                           security data directory entry. Zeroed when
                           the image declares no security directory.

  @retval EFI_SUCCESS            SecDataDir has been populated.
  @retval EFI_INVALID_PARAMETER  FileBuffer or SecDataDir is NULL.
  @retval EFI_LOAD_ERROR         FileBuffer does not contain a valid
                                 PE/COFF image, or PeCoffLib otherwise
                                 rejected the headers.
**/
EFI_STATUS
GetImageSecurityDataDirectory (
  IN  VOID                      *FileBuffer,
  IN  UINTN                     FileSize,
  OUT EFI_IMAGE_DATA_DIRECTORY  *SecDataDir
  );

/**
  Determine whether Guid matches any X509CertHashGuid entry in mHashAlgorithms.

  This identifies an `EFI_SIGNATURE_LIST` whose entries are TBS-cert hashes (any
  digest-flavored X.509-cert-hash list GUID, e.g. `EFI_CERT_X509_SHA256_GUID` or
  future digests such as SM3) rather than full X.509 certificates.

  @param[in]  Guid  Candidate signature-list type GUID; may be NULL.

  @retval TRUE   Guid matches one of the X509CertHashGuid entries in mHashAlgorithms.
  @retval FALSE  Guid is NULL or does not match any X509CertHashGuid entry.
**/
BOOLEAN
IsX509CertHashGuid (
  IN  CONST EFI_GUID  *Guid
  );

#endif // DXE_IMAGE_VERIFICATION_LIB_SUPPORT_H_
