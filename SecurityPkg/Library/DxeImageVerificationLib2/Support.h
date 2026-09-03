/** @file
  Utility functions for DxeImageVerificationLib.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include "DxeImageVerificationLib.h"

//
// A memoized digest owned by a `DIGEST_CACHE`, allocated on demand by GetHash () and released by
// FreeDigestCache (). The layout is private to Cache.c.
//
typedef struct DIGEST_CACHE_ENTRY DIGEST_CACHE_ENTRY;

//
// Caller-owned, generic digest cache bound to one buffer via Buffer / BufferSize. GetHash () hashes
// that buffer under a requested algorithm and memoizes the result, allocating one entry per distinct
// algorithm on demand (Entries is the head of that list).
//
// Zero-initialize before binding Buffer / BufferSize, and release the memoized entries with
// FreeDigestCache () when done.
//
typedef struct {
  CONST VOID            *Buffer;
  UINTN                 BufferSize;
  DIGEST_CACHE_ENTRY    *Entries;
} DIGEST_CACHE;

/**
  Get or compute the cached digest of the cache's buffer under a hash algorithm.

  On a cache hit the memoized digest is returned; on a miss the buffer is hashed with HashAllByGuid ()
  and the result is memoized in a newly allocated entry (keyed by HashAlgorithm) before being
  returned. The digest bytes remain valid until FreeDigestCache ().

  @param[in]      HashAlgorithm  Protocol/Hash.h algorithm GUID (EFI_HASH_ALGORITHM_*_GUID).
  @param[in,out]  Cache          Caller-owned cache bound to a buffer via Cache->Buffer /
                                 Cache->BufferSize.
  @param[out]     Digest         On success, a pointer to the cached digest bytes, valid until
                                 FreeDigestCache ().
  @param[out]     DigestSize     On success, the digest length in bytes.

  @retval EFI_SUCCESS            Digest / DigestSize describe a valid cached digest.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_OUT_OF_RESOURCES   A cache entry could not be allocated.
  @retval other                  A failure computing the digest, propagated from HashAllByGuid ().
**/
EFI_STATUS
GetHash (
  IN     CONST EFI_GUID  *HashAlgorithm,
  IN OUT DIGEST_CACHE    *Cache,
  OUT    CONST UINT8     **Digest,
  OUT    UINTN           *DigestSize
  );

/**
  Release the memoized digest entries owned by a DIGEST_CACHE.

  Frees every entry GetHash () allocated and resets the cache to empty. Buffer / BufferSize are left
  intact (the buffer is caller-owned). Safe to call on a zero-initialized or already-freed cache.

  @param[in,out]  Cache  Cache whose memoized entries are released, or NULL.
**/
VOID
FreeDigestCache (
  IN OUT DIGEST_CACHE  *Cache
  );

//
// The kind of subject an EFI_SIGNATURE_LIST enrolls, independent of the entry layout (V1 vs V2).
//
typedef enum {
  SignatureKindImageHash,      // raw image digest           (EFI_CERT_SHA*      / EFI_CERT_V2_SHA*)
  SignatureKindX509Cert,       // DER X.509 certificate       (EFI_CERT_X509      / EFI_CERT_V2_X509)
  SignatureKindX509TbsHash     // X.509 TBSCertificate digest (EFI_CERT_X509_SHA* / EFI_CERT_V2_X509_SHA*)
} SIGNATURE_KIND;

/**
  Populate Authority with a newly allocated V1 EFI_SIGNATURE_DATA that wraps a certificate payload.

  The allocation is a 16-byte SignatureOwner followed by a copy of Payload. It is owned by the
  caller and released with FreeImageAuthority (). Authority->SignatureType is left unchanged so the
  caller can record the authorizing list's signature type independently.

  @param[in]   Owner        SignatureOwner GUID to store, or NULL to store a zeroed GUID (used for a
                            matching V2 EFI_SIGNATURE_V2_DATA entry, which carries no owner).
  @param[in]   Payload      The certificate (or other signature payload) to copy.
  @param[in]   PayloadSize  Size of Payload in bytes.
  @param[out]  Authority    On success, Authority->Data references the allocated EFI_SIGNATURE_DATA
                            and Authority->Size is its total length.

  @retval EFI_SUCCESS            Authority was populated.
  @retval EFI_INVALID_PARAMETER  Payload or Authority is NULL, or PayloadSize is 0 or too large.
  @retval EFI_OUT_OF_RESOURCES   The allocation failed.
**/
EFI_STATUS
BuildImageAuthority (
  IN  CONST EFI_GUID   *Owner  OPTIONAL,
  IN  CONST UINT8      *Payload,
  IN  UINTN            PayloadSize,
  OUT IMAGE_AUTHORITY  *Authority
  );

/**
  Release the allocation owned by an IMAGE_AUTHORITY.

  Frees Authority->Data (if any) and clears Authority->Data / Authority->Size. Authority->SignatureType
  is left intact. Safe to call on an already-empty authority or a NULL pointer.

  @param[in,out]  Authority  Authority whose owned Data is released.
**/
VOID
FreeImageAuthority (
  IN OUT IMAGE_AUTHORITY  *Authority
  );
