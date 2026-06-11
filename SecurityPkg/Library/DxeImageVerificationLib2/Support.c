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
        if ((mHashAlgorithms[I].ImageHashGuid != NULL) &&
            CompareGuid (Guid, mHashAlgorithms[I].ImageHashGuid))
        {
          *Index = I;
          return TRUE;
        }

        break;

      case DigestCacheTypeX509:
        if ((mHashAlgorithms[I].X509CertHashGuid != NULL) &&
            CompareGuid (Guid, mHashAlgorithms[I].X509CertHashGuid))
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
        Status = GetAuthenticodeHash ((VOID *)Cache->Buffer, Cache->BufferSize, HashType, Slot->Bytes, &Slot->BufferSize);
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
  Determine whether Guid matches any X509CertHashGuid entry in mHashAlgorithms.

  This identifies an `EFI_SIGNATURE_LIST` whose entries are TBS-cert hashes (any
  digest-flavored X.509-cert-hash list GUID) rather than full X.509 certificates.

  @param[in]  Guid  Candidate signature-list type GUID; may be NULL.

  @retval TRUE   Guid matches one of the X509CertHashGuid entries in mHashAlgorithms.
  @retval FALSE  Guid is NULL or does not match any X509CertHashGuid entry.
**/
BOOLEAN
IsX509CertHashGuid (
  IN  CONST EFI_GUID  *Guid
  )
{
  UINTN  Index;

  if (Guid == NULL) {
    return FALSE;
  }

  for (Index = 0; Index < ARRAY_SIZE (mHashAlgorithms); Index++) {
    if ((mHashAlgorithms[Index].X509CertHashGuid != NULL) &&
        CompareGuid (Guid, mHashAlgorithms[Index].X509CertHashGuid))
    {
      return TRUE;
    }
  }

  return FALSE;
}
