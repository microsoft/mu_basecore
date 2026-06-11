/** @file
  Image Execution Information Table helpers for the DXE Image Verification
  Library.

  The UEFI specification defines an EFI_IMAGE_EXECUTION_INFO_TABLE that is
  published in the EFI System Configuration Table under
  gEfiImageSecurityDatabaseGuid. The DXE Image Verification Library appends an
  entry to this table every time it rejects an image so that the OS (or other
  consumers) can discover which images failed authentication and why.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DXE_IMAGE_VERIFICATION_LIB_IMAGE_EXECUTION_H_
#define DXE_IMAGE_VERIFICATION_LIB_IMAGE_EXECUTION_H_

#include "DxeImageVerificationLib.h"
#include "Support.h"

/**
  Compute the size, in bytes, of an EFI_IMAGE_EXECUTION_INFO_TABLE.

  The size is the fixed table header plus the InfoSize of every
  EFI_IMAGE_EXECUTION_INFO entry that follows it. The InfoSize fields are read
  with ReadUnaligned32 because table entries are not guaranteed to be naturally
  aligned.

  @param[in]  ImageExeInfoTable  Table to measure, or NULL.

  @retval 0       ImageExeInfoTable is NULL.
  @retval Others  Size of the table in bytes.
**/
UINTN
GetImageExeInfoTableSize (
  IN EFI_IMAGE_EXECUTION_INFO_TABLE  *ImageExeInfoTable
  );

/**
  Build an EFI_SIGNATURE_LIST that records a single image digest.

  The resulting list has one EFI_SIGNATURE_DATA entry whose SignatureData is a
  copy of Digest and whose SignatureOwner is the zero GUID. The list's
  SignatureType is HashType (the image-hash algorithm GUID). This matches the
  layout the UEFI specification expects for an unsigned-style image-hash
  signature recorded into the Image Execution Information Table.

  @param[in]   HashType           Image-hash algorithm GUID (e.g. gEfiCertSha256Guid).
  @param[in]   Digest             Image digest bytes.
  @param[in]   DigestSize         Size of Digest in bytes; must be non-zero.
  @param[out]  SignatureList      On success, receives a pool-allocated
                                  EFI_SIGNATURE_LIST. Caller frees with FreePool.
  @param[out]  SignatureListSize  On success, receives the size of *SignatureList.

  @retval EFI_SUCCESS            The signature list was built.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL or DigestSize is 0.
  @retval EFI_OUT_OF_RESOURCES   The allocation failed.
**/
EFI_STATUS
BuildImageDigestSignatureList (
  IN  CONST EFI_GUID      *HashType,
  IN  CONST UINT8         *Digest,
  IN  UINTN               DigestSize,
  OUT EFI_SIGNATURE_LIST  **SignatureList,
  OUT UINTN               *SignatureListSize
  );

/**
  Create or update the Image Execution Information Table with a new entry.

  Locates the existing EFI_IMAGE_EXECUTION_INFO_TABLE in the EFI System
  Configuration Table (creating a new one if absent), appends a single
  EFI_IMAGE_EXECUTION_INFO entry describing Action / Name / DevicePath /
  Signature, and re-installs the (re)allocated table under
  gEfiImageSecurityDatabaseGuid. The previous table allocation, if any, is
  freed after the new one is installed.

  The table is allocated from EfiRuntimeServicesData so it remains valid for
  consumption after ExitBootServices.

  @param[in]  Action         Action taken by the firmware regarding the image.
  @param[in]  Name           Optional NULL-terminated, user-friendly image name.
                             When NULL, a single NULL CHAR16 is recorded.
  @param[in]  DevicePath     Device path of the image being recorded.
  @param[in]  Signature      Optional EFI_SIGNATURE_LIST describing the image
                             signature(s). May be NULL.
  @param[in]  SignatureSize  Size of Signature in bytes. Must be 0 when
                             Signature is NULL.

  @retval EFI_SUCCESS            The entry was recorded and the table installed.
  @retval EFI_INVALID_PARAMETER  DevicePath is NULL, or Signature is NULL while
                                 SignatureSize is non-zero (or vice versa).
  @retval EFI_OUT_OF_RESOURCES   A required allocation failed.
  @retval Other                  Status from gBS->InstallConfigurationTable.
**/
EFI_STATUS
AddImageExeInfo (
  IN       EFI_IMAGE_EXECUTION_ACTION  Action,
  IN       CHAR16                      *Name OPTIONAL,
  IN CONST EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN       EFI_SIGNATURE_LIST          *Signature OPTIONAL,
  IN       UINTN                       SignatureSize
  );

/**
  Record a rejected image into the Image Execution Information Table.

  Builds the table entry describing the rejection. When HashType is a non-zero
  GUID, the memoized image digest for that algorithm is retrieved from Cache and
  recorded as the entry's signature (matching the legacy SIG_FOUND / SIG_FAILED
  behavior); otherwise no signature is recorded. The image name is derived from
  File via ConvertDevicePathToText. All transient allocations are freed before
  return.

  @param[in]      File      Device path of the rejected image. Must be non-NULL.
  @param[in]      Action    The EFI_IMAGE_EXECUTION_ACTION describing why the
                            image was rejected.
  @param[in,out]  Cache     Image digest cache bound to the image buffer; used to
                            recover the memoized digest when HashType is non-zero.
  @param[in]      HashType  Image-hash algorithm GUID to record the digest under,
                            or the zero GUID to record no signature.
**/
VOID
RecordRejectedImage (
  IN     CONST EFI_DEVICE_PATH_PROTOCOL  *File,
  IN     EFI_IMAGE_EXECUTION_ACTION      Action,
  IN OUT DIGEST_CACHE                    *Cache,
  IN     CONST EFI_GUID                  *HashType
  );

#endif // DXE_IMAGE_VERIFICATION_LIB_IMAGE_EXECUTION_H_
