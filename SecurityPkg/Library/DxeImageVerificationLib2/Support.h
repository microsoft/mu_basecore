/** @file
  Support function declarations for DxeImageVerificationLib.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include "DxeImageVerificationLib.h"
#include <Guid/WinCertificate.h>

//
// A memoized digest owned by a `DIGEST_CACHE`, allocated on demand by GetHash () and released by
// FreeDigestCache (). The layout is private to Support.c.
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

/**
  Determine whether the given device path resolves to a Firmware Volume.

  @param[in]   File       Device path describing the image origin.

  @retval EFI_SUCCESS            The image is from a Firmware Volume.
  @retval EFI_NOT_FOUND          The image is not from a Firmware Volume.
  @retval EFI_INVALID_PARAMETER  File is NULL.
**/
EFI_STATUS
IsFromFv (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File
  );

/**
  Determines the Image type classification.

  @param[in]   File       Device path describing the image origin.
  @param[out]  ImageType  The classification of the image source.

  @retval EFI_SUCCESS            ImageType contains a valid value.
  @retval EFI_INVALID_PARAMETER  File or ImageType is NULL.
**/
EFI_STATUS
GetImageType (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File,
  OUT UINT32                          *ImageType
  );

/**
  Look up the configured authorization policy for the given image source.

  IMAGE_FROM_FV is mapped to ALWAYS_EXECUTE; all other image types map to
  DENY_EXECUTE_ON_SECURITY_VIOLATION.

  @param[in]  ImageType  An IMAGE_* image source classification value.

  @return  ALWAYS_EXECUTE or DENY_EXECUTE_ON_SECURITY_VIOLATION.
**/
UINT32
GetPolicyForImageType (
  IN UINT32  ImageType
  );

/**
  Resolve an image's authorization policy.

  @param[in]   File    Device path describing the image origin.
  @param[out]  Policy  On success, filled with the resolved policy value.

  @retval EFI_SUCCESS            Policy contains a valid policy value.
  @retval EFI_INVALID_PARAMETER  File or Policy is NULL.
**/
EFI_STATUS
GetExecutionPolicy (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File,
  OUT UINT32                          *Policy
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
  Extract the Authenticode signature from a single WIN_CERTIFICATE entry.

  @param[in]   Cert          The certificate to inspect.
  @param[out]  SignatureData      On success, set to point at the Authenticode signature inside Cert.
  @param[out]  SignatureDataSize  On success, set to the Authenticode signature length in bytes.

  @retval EFI_SUCCESS            SignatureData/SignatureDataSize were populated.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_UNSUPPORTED        Unsupported WIN_CERTIFICATE type.
  @retval EFI_VOLUME_CORRUPTED   dwLength is too small to contain the required header for the
                                 declared type.
**/
EFI_STATUS
ExtractSignatureData (
  IN  CONST WIN_CERTIFICATE  *Cert,
  OUT CONST UINT8            **SignatureData,
  OUT UINTN                  *SignatureDataSize
  );

/**
  Populate Authority with a newly allocated V1 EFI_SIGNATURE_DATA that wraps a certificate payload.

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
