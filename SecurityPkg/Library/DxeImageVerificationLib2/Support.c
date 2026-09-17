/** @file
  Support functions for DxeImageVerificationLib.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Support.h"

//
// A linked list entry containing one computed digest associated with an algorithm.
//
struct DIGEST_CACHE_ENTRY {
  DIGEST_CACHE_ENTRY    *Next;                     // The next entry in the linked list.
  CONST EFI_GUID        *Algorithm;                // The hash algorithm GUID.
  UINT8                 Digest[MAX_DIGEST_SIZE];   // The computed hash digest.
  UINTN                 DigestSize;                // The size of the computed hash digest.
};

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
  )
{
  EFI_STATUS          Status;
  DIGEST_CACHE_ENTRY  *Entry;

  if ((HashAlgorithm == NULL) || (Cache == NULL) || (Cache->Buffer == NULL) ||
      (Digest == NULL) || (DigestSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Return a cached digest if available
  for (Entry = Cache->Entries; Entry != NULL; Entry = Entry->Next) {
    if (CompareGuid (Entry->Algorithm, HashAlgorithm)) {
      *Digest     = Entry->Digest;
      *DigestSize = Entry->DigestSize;
      return EFI_SUCCESS;
    }
  }

  Entry = AllocatePool (sizeof (*Entry));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HashAllByGuid (HashAlgorithm, Cache->Buffer, Cache->BufferSize, Entry->Digest, &Entry->DigestSize);
  if (EFI_ERROR (Status)) {
    FreePool (Entry);
    return Status;
  }

  Entry->Algorithm = HashAlgorithm;
  Entry->Next      = Cache->Entries;
  Cache->Entries   = Entry;

  *Digest     = Entry->Digest;
  *DigestSize = Entry->DigestSize;
  return EFI_SUCCESS;
}

/**
  Release the memoized digest entries owned by a DIGEST_CACHE.

  Frees every entry GetHash () allocated and resets the cache to empty. Buffer / BufferSize are left
  intact (the buffer is caller-owned). Safe to call on a zero-initialized or already-freed cache.

  @param[in,out]  Cache  Cache whose memoized entries are released, or NULL.
**/
VOID
FreeDigestCache (
  IN OUT DIGEST_CACHE  *Cache
  )
{
  DIGEST_CACHE_ENTRY  *Entry;
  DIGEST_CACHE_ENTRY  *Next;

  if (Cache == NULL) {
    return;
  }

  for (Entry = Cache->Entries; Entry != NULL; Entry = Next) {
    Next = Entry->Next;
    FreePool (Entry);
  }

  Cache->Entries = NULL;
}

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
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;

  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)File;
  Status         = gBS->LocateDevicePath (
                          &gEfiFirmwareVolume2ProtocolGuid,
                          &TempDevicePath,
                          &DeviceHandle
                          );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  // Confirm the protocol is actually present on the resolved handle.
  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

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
  )
{
  if ((File == NULL) || (ImageType == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!EFI_ERROR (IsFromFv (File))) {
    *ImageType = IMAGE_FROM_FV;
    return EFI_SUCCESS;
  }

  *ImageType = IMAGE_UNKNOWN;
  return EFI_SUCCESS;
}

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
  )
{
  if (ImageType == IMAGE_FROM_FV) {
    return ALWAYS_EXECUTE;
  }

  return DENY_EXECUTE_ON_SECURITY_VIOLATION;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      ImageType;

  if ((File == NULL) || (Policy == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetImageType (File, &ImageType);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Policy = GetPolicyForImageType (ImageType);

  return EFI_SUCCESS;
}

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
  )
{
  CONST WIN_CERTIFICATE_UEFI_GUID  *UefiGuidCert;

  if ((Cert == NULL) || (SignatureData == NULL) || (SignatureDataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Cert->wCertificateType) {
    case WIN_CERT_TYPE_PKCS_SIGNED_DATA:
      //
      // The Authenticode signature is prefixed by the WIN_CERTIFICATE header.
      //
      if (Cert->dwLength <= sizeof (WIN_CERTIFICATE)) {
        return EFI_VOLUME_CORRUPTED;
      }

      *SignatureData     = (CONST UINT8 *)Cert + sizeof (WIN_CERTIFICATE);
      *SignatureDataSize = Cert->dwLength - sizeof (WIN_CERTIFICATE);
      return EFI_SUCCESS;

    case WIN_CERT_TYPE_EFI_GUID:
      //
      // The certificate is a WIN_CERTIFICATE_UEFI_GUID; the embedded
      // payload format is identified by CertType. Only the Authenticode
      // signature GUID is supported.
      //
      if (Cert->dwLength <= OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)) {
        return EFI_VOLUME_CORRUPTED;
      }

      UefiGuidCert = (CONST WIN_CERTIFICATE_UEFI_GUID *)Cert;
      if (!CompareGuid (&UefiGuidCert->CertType, &gEfiCertPkcs7Guid)) {
        return EFI_UNSUPPORTED;
      }

      *SignatureData     = UefiGuidCert->CertData;
      *SignatureDataSize = Cert->dwLength - OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
      return EFI_SUCCESS;

    default:
      return EFI_UNSUPPORTED;
  }
}

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
  )
{
  EFI_SIGNATURE_DATA  *SigData;
  UINTN               TotalSize;

  if ((Payload == NULL) || (PayloadSize == 0) || (Authority == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (PayloadSize > MAX_UINTN - OFFSET_OF (EFI_SIGNATURE_DATA, SignatureData)) {
    return EFI_INVALID_PARAMETER;
  }

  TotalSize = OFFSET_OF (EFI_SIGNATURE_DATA, SignatureData) + PayloadSize;

  SigData = AllocateZeroPool (TotalSize);
  if (SigData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

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
