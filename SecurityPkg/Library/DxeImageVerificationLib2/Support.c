/** @file
  Utility functions for DxeImageVerificationLib.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Support.h"

/**
  Resolve the hash-algorithm table index for Guid according to cache type.

  @param[in]   CacheType  Digest cache type selecting the compatible GUID namespace.
  @param[in]   Guid       Candidate signature-type GUID.
  @param[out]  Index      On TRUE return, receives Guid's position in mHashAlgorithms.

  @retval TRUE   Guid matched an entry compatible with CacheType.
  @retval FALSE  CacheType is unsupported, Guid is NULL, Index is NULL, or Guid is not compatible
                 with CacheType.
**/
STATIC
BOOLEAN
GetIndex (
  IN  DIGEST_CACHE_TYPE  CacheType,
  IN  CONST EFI_GUID     *Guid,
  OUT UINTN              *Index
  )
{
  UINTN  I;

  if ((Guid == NULL) || (Index == NULL)) {
    return FALSE;
  }

  for (I = 0; I < ARRAY_SIZE (mHashAlgorithms); I++) {
    switch (CacheType) {
      case DigestCacheTypeImage:
        if (((mHashAlgorithms[I].ImageHashGuid != NULL) && CompareGuid (Guid, mHashAlgorithms[I].ImageHashGuid)) ||
            ((mHashAlgorithms[I].ImageHashGuidV2 != NULL) && CompareGuid (Guid, mHashAlgorithms[I].ImageHashGuidV2)))
        {
          *Index = I;
          return TRUE;
        }

        break;

      case DigestCacheTypeX509:
        if (((mHashAlgorithms[I].X509CertHashGuid != NULL) && CompareGuid (Guid, mHashAlgorithms[I].X509CertHashGuid)) ||
            ((mHashAlgorithms[I].X509CertHashGuidV2 != NULL) && CompareGuid (Guid, mHashAlgorithms[I].X509CertHashGuidV2)))
        {
          *Index = I;
          return TRUE;
        }

        break;

      default:
        return FALSE;
    }
  }

  return FALSE;
}

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
  )
{
  EFI_STATUS          Status;
  UINTN               SlotIndex;
  DIGEST_CACHE_ENTRY  *Slot;

  if ((HashType == NULL) || (Cache == NULL) || (Cache->Buffer == NULL) ||
      (Digest == NULL) || (DigestSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (!GetIndex (Cache->Type, HashType, &SlotIndex)) {
    return EFI_UNSUPPORTED;
  }

  Slot = &Cache->Entries[SlotIndex];

  //
  // Populate the cache entry if it is not already populated.
  //
  if (Slot->BufferSize == 0) {
    switch (Cache->Type) {
      case DigestCacheTypeImage:
        Status = GetAuthenticodeHash (
                   (VOID *)Cache->Buffer,
                   Cache->BufferSize,
                   mHashAlgorithms[SlotIndex].ImageHashGuid, // Map possible V2 GUID to the base GUID
                   Slot->Bytes,
                   &Slot->BufferSize
                   );
        if (EFI_ERROR (Status)) {
          Slot->BufferSize = 0;
          return EFI_SECURITY_VIOLATION;
        }

        break;

      case DigestCacheTypeX509:
        Status = X509GetTbsCertHash (
                   (VOID *)Cache->Buffer,
                   Cache->BufferSize,
                   mHashAlgorithms[SlotIndex].ImageHashGuid, // Map the X509 hash type GUID to the base hash type GUID
                   Slot->Bytes,
                   &Slot->BufferSize
                   );
        if (EFI_ERROR (Status)) {
          Slot->BufferSize = 0;
          return EFI_SECURITY_VIOLATION;
        }

        break;

      default:
        Slot->BufferSize = 0;
        return EFI_UNSUPPORTED;
    }
  }

  *Digest     = Slot->Bytes;
  *DigestSize = Slot->BufferSize;
  return EFI_SUCCESS;
}

//
// Image Handle that contains the image bytes and size.
//
typedef struct {
  CONST UINT8    *Base;
  UINTN          BufferSize;
} PE_COFF_IMAGE_HANDLE;

/**
  Buffer size aware implementation of PE_COFF_LOADER_READ_FILE.

  Truncates reads for requests that start within the image bounds, but extends beyond the image
  bounds. Sets read size to 0 for requests that start beyond the image bounds.

  @param[in]      FileHandle  Pointer to a PE_COFF_IMAGE_HANDLE describing
                              the image bytes available to PeCoffLib.
  @param[in]      FileOffset  Byte offset within the image to read from.
  @param[in,out]  ReadSize    On input, bytes requested. On output, bytes
                              actually copied (may be 0 or less than
                              requested if the range extends past EOF).
  @param[out]     Buffer      Destination buffer of at least the input
                              *ReadSize bytes.

  @retval RETURN_SUCCESS            Read succeeded; *ReadSize is the
                                    number of bytes copied.
  @retval RETURN_INVALID_PARAMETER  FileHandle, ReadSize, or Buffer was
                                    NULL.
**/
STATIC
RETURN_STATUS
EFIAPI
BoundedImageRead (
  IN     VOID   *FileHandle,
  IN     UINTN  FileOffset,
  IN OUT UINTN  *ReadSize,
  OUT    VOID   *Buffer
  )
{
  CONST PE_COFF_IMAGE_HANDLE  *Handle;
  UINTN                       Available;

  if ((FileHandle == NULL) || (ReadSize == NULL) || (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  Handle = (CONST PE_COFF_IMAGE_HANDLE *)FileHandle;

  if (FileOffset >= Handle->BufferSize) {
    *ReadSize = 0;
    return RETURN_SUCCESS;
  }

  Available = Handle->BufferSize - FileOffset;
  if (*ReadSize > Available) {
    *ReadSize = Available;
  }

  CopyMem (Buffer, Handle->Base + FileOffset, *ReadSize);
  return RETURN_SUCCESS;
}

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
  )
{
  RETURN_STATUS                 PeCoffStatus;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  PE_COFF_IMAGE_HANDLE          Handle;

  if ((FileBuffer == NULL) || (SecDataDir == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (SecDataDir, sizeof (*SecDataDir));

  Handle.Base       = (CONST UINT8 *)FileBuffer;
  Handle.BufferSize = FileSize;

  //
  // Delegate header parsing, signature validation, optional-header
  // magic checks, and security-directory bounds checking to PeCoffLib.
  // PeCoffLib records the validated security data directory entry in the
  // image context, leaving it zeroed when the image declares no security
  // directory.
  //
  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle    = &Handle;
  ImageContext.ImageRead = BoundedImageRead;

  PeCoffStatus = PeCoffLoaderGetImageInfo (&ImageContext);
  if (RETURN_ERROR (PeCoffStatus)) {
    DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: PeImage invalid (0x%lx).\n", (UINT64)PeCoffStatus));
    return EFI_LOAD_ERROR;
  }

  *SecDataDir = ImageContext.SecurityDataDirectory;
  return EFI_SUCCESS;
}

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
  )
{
  UINTN  Index;

  if ((SignatureType == NULL) || (Kind == NULL) || (OwnerSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Full X.509 certificate lists carry a DER certificate and are not tracked in mHashAlgorithms.
  //
  if (CompareGuid (SignatureType, &gEfiCertX509Guid)) {
    *Kind      = SignatureKindX509Cert;
    *OwnerSize = sizeof (EFI_GUID);
    return EFI_SUCCESS;
  }

  if (CompareGuid (SignatureType, &gEfiCertV2X509Guid)) {
    *Kind      = SignatureKindX509Cert;
    *OwnerSize = 0;
    return EFI_SUCCESS;
  }

  //
  // Image-hash and X.509 TBS-cert-hash lists carry a digest. One pass over the table checks both
  // roles across both layouts; a match in a V2 column reports a zero owner size.
  //
  for (Index = 0; Index < ARRAY_SIZE (mHashAlgorithms); Index++) {
    if ((mHashAlgorithms[Index].ImageHashGuid != NULL) &&
        CompareGuid (SignatureType, mHashAlgorithms[Index].ImageHashGuid))
    {
      *Kind      = SignatureKindImageHash;
      *OwnerSize = sizeof (EFI_GUID);
      return EFI_SUCCESS;
    }

    if ((mHashAlgorithms[Index].ImageHashGuidV2 != NULL) &&
        CompareGuid (SignatureType, mHashAlgorithms[Index].ImageHashGuidV2))
    {
      *Kind      = SignatureKindImageHash;
      *OwnerSize = 0;
      return EFI_SUCCESS;
    }

    if ((mHashAlgorithms[Index].X509CertHashGuid != NULL) &&
        CompareGuid (SignatureType, mHashAlgorithms[Index].X509CertHashGuid))
    {
      *Kind      = SignatureKindX509TbsHash;
      *OwnerSize = sizeof (EFI_GUID);
      return EFI_SUCCESS;
    }

    if ((mHashAlgorithms[Index].X509CertHashGuidV2 != NULL) &&
        CompareGuid (SignatureType, mHashAlgorithms[Index].X509CertHashGuidV2))
    {
      *Kind      = SignatureKindX509TbsHash;
      *OwnerSize = 0;
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

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
  )
{
  EFI_SIGNATURE_DATA  *SigData;
  UINTN               TotalSize;

  if ((Payload == NULL) || (PayloadSize == 0) || (Authority == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Reject a payload large enough to overflow the header addition. PayloadSize is derived from
  // attacker-controlled db/dbx and image data, so the bound is checked before allocation.
  //
  if (PayloadSize > MAX_UINTN - OFFSET_OF (EFI_SIGNATURE_DATA, SignatureData)) {
    return EFI_INVALID_PARAMETER;
  }

  TotalSize = OFFSET_OF (EFI_SIGNATURE_DATA, SignatureData) + PayloadSize;

  SigData = AllocateZeroPool (TotalSize);
  if (SigData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // A NULL Owner leaves the zeroed SignatureOwner from AllocateZeroPool in place (V2 source entry).
  //
  if (Owner != NULL) {
    CopyGuid (&SigData->SignatureOwner, Owner);
  }

  CopyMem (SigData->SignatureData, Payload, PayloadSize);

  Authority->Data = SigData;
  Authority->Size = TotalSize;

  return EFI_SUCCESS;
}

/**
  Release the allocation owned by an IMAGE_AUTHORITY.

  Frees Authority->Data (if any) and clears Authority->Data / Authority->Size. Authority->SignatureType
  is left intact. Safe to call on an already-empty authority or a NULL pointer.

  @param[in,out]  Authority  Authority whose owned Data is released.
**/
VOID
FreeImageAuthority (
  IN OUT IMAGE_AUTHORITY  *Authority
  )
{
  if (Authority == NULL) {
    return;
  }

  if (Authority->Data != NULL) {
    FreePool (Authority->Data);
    Authority->Data = NULL;
  }

  Authority->Size = 0;
}
