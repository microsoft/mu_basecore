/** @file
  Utility functions for DxeImageVerificationLib.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

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

//
// The kind of subject an EFI_SIGNATURE_LIST enrolls, independent of the entry layout (V1 vs V2)
// that GetSignatureTypeInfo reports separately.
//
typedef enum {
  SignatureKindImageHash,      // raw image digest           (EFI_CERT_SHA*      / EFI_CERT_V2_SHA*)
  SignatureKindX509Cert,       // DER X.509 certificate       (EFI_CERT_X509      / EFI_CERT_V2_X509)
  SignatureKindX509TbsHash     // X.509 TBSCertificate digest (EFI_CERT_X509_SHA* / EFI_CERT_V2_X509_SHA*)
} SIGNATURE_KIND;

/**
  Classify an EFI_SIGNATURE_LIST SignatureType GUID for the database walkers.

  In a single table lookup, reports what a list's entries enroll (Kind) and how they are laid out
  (OwnerSize): entries of a V1 signature type begin with a 16-byte SignatureOwner
  (EFI_SIGNATURE_DATA), while V2 entries omit it (EFI_SIGNATURE_V2_DATA). The payload therefore
  begins OwnerSize bytes into each entry.

  @param[in]   SignatureType  The EFI_SIGNATURE_LIST SignatureType GUID.
  @param[out]  Kind           On EFI_SUCCESS, the signature kind.
  @param[out]  OwnerSize      On EFI_SUCCESS, the per-entry SignatureOwner size: 0 for a V2
                              (EFI_SIGNATURE_V2_DATA) type, sizeof (EFI_GUID) for a V1
                              (EFI_SIGNATURE_DATA) type.

  @retval EFI_SUCCESS            SignatureType is recognized; Kind and OwnerSize are set.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_UNSUPPORTED        SignatureType is not a supported signature type.
**/
EFI_STATUS
GetSignatureTypeInfo (
  IN  CONST EFI_GUID  *SignatureType,
  OUT SIGNATURE_KIND  *Kind,
  OUT UINTN           *OwnerSize
  );

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
