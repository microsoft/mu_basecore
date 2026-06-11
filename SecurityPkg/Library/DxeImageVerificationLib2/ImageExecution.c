/** @file
  Image Execution Information Table helpers for the DXE Image Verification
  Library.

  Every time the verification handler rejects an image it records the outcome
  in the EFI_IMAGE_EXECUTION_INFO_TABLE published under
  gEfiImageSecurityDatabaseGuid. This file owns the (re)allocation,
  serialization, and installation of that configuration table.

  Caution: This file consumes external input (the device path of the rejected
  image and the signature data describing it). All inputs must be treated as
  attacker-controlled.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeImageVerificationLib.h"

#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>

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
  )
{
  UINTN                     Index;
  EFI_IMAGE_EXECUTION_INFO  *ImageExeInfoItem;
  UINTN                     TotalSize;

  if (ImageExeInfoTable == NULL) {
    return 0;
  }

  ImageExeInfoItem = (EFI_IMAGE_EXECUTION_INFO *)((UINT8 *)ImageExeInfoTable + sizeof (EFI_IMAGE_EXECUTION_INFO_TABLE));
  TotalSize        = sizeof (EFI_IMAGE_EXECUTION_INFO_TABLE);

  for (Index = 0; Index < ImageExeInfoTable->NumberOfImages; Index++) {
    TotalSize       += ReadUnaligned32 ((UINT32 *)&ImageExeInfoItem->InfoSize);
    ImageExeInfoItem = (EFI_IMAGE_EXECUTION_INFO *)((UINT8 *)ImageExeInfoItem + ReadUnaligned32 ((UINT32 *)&ImageExeInfoItem->InfoSize));
  }

  return TotalSize;
}

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
  )
{
  EFI_SIGNATURE_LIST  *List;
  EFI_SIGNATURE_DATA  *Data;
  UINTN               ListSize;

  if ((HashType == NULL) || (Digest == NULL) || (DigestSize == 0) ||
      (SignatureList == NULL) || (SignatureListSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *SignatureList     = NULL;
  *SignatureListSize = 0;

  //
  // EFI_SIGNATURE_DATA is a flexible structure ending in SignatureData[1];
  // subtract that one byte already accounted for in the struct size.
  //
  ListSize = sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + DigestSize;

  List = (EFI_SIGNATURE_LIST *)AllocateZeroPool (ListSize);
  if (List == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyGuid (&List->SignatureType, HashType);
  List->SignatureListSize   = (UINT32)ListSize;
  List->SignatureHeaderSize = 0;
  List->SignatureSize       = (UINT32)(sizeof (EFI_SIGNATURE_DATA) - 1 + DigestSize);

  Data = (EFI_SIGNATURE_DATA *)((UINT8 *)List + sizeof (EFI_SIGNATURE_LIST));
  CopyMem (Data->SignatureData, Digest, DigestSize);

  *SignatureList     = List;
  *SignatureListSize = ListSize;

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                      Status;
  EFI_IMAGE_EXECUTION_INFO_TABLE  *ImageExeInfoTable;
  EFI_IMAGE_EXECUTION_INFO_TABLE  *NewImageExeInfoTable;
  EFI_IMAGE_EXECUTION_INFO        *ImageExeInfoEntry;
  UINTN                           ImageExeInfoTableSize;
  UINTN                           NewImageExeInfoEntrySize;
  UINTN                           NameStringLen;
  UINTN                           DevicePathSize;
  CHAR16                          *NameStr;

  //
  // A device path is mandatory; the signature pointer and size must agree.
  //
  if ((DevicePath == NULL) ||
      ((Signature == NULL) && (SignatureSize != 0)) ||
      ((Signature != NULL) && (SignatureSize == 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Name != NULL) {
    NameStringLen = StrSize (Name);
  } else {
    NameStringLen = sizeof (CHAR16);
  }

  //
  // Locate the existing table (if any) to size the new allocation. When no
  // table exists yet, the base size is just the fixed header.
  //
  ImageExeInfoTable = NULL;
  EfiGetSystemConfigurationTable (&gEfiImageSecurityDatabaseGuid, (VOID **)&ImageExeInfoTable);
  if (ImageExeInfoTable != NULL) {
    ImageExeInfoTableSize = GetImageExeInfoTableSize (ImageExeInfoTable);
  } else {
    ImageExeInfoTableSize = sizeof (EFI_IMAGE_EXECUTION_INFO_TABLE);
  }

  DevicePathSize           = GetDevicePathSize (DevicePath);
  NewImageExeInfoEntrySize = sizeof (EFI_IMAGE_EXECUTION_INFO) + NameStringLen + DevicePathSize + SignatureSize;

  NewImageExeInfoTable = (EFI_IMAGE_EXECUTION_INFO_TABLE *)AllocateRuntimePool (ImageExeInfoTableSize + NewImageExeInfoEntrySize);
  if (NewImageExeInfoTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the existing table contents, or start a fresh, empty table.
  //
  if (ImageExeInfoTable != NULL) {
    CopyMem (NewImageExeInfoTable, ImageExeInfoTable, ImageExeInfoTableSize);
  } else {
    NewImageExeInfoTable->NumberOfImages = 0;
  }

  NewImageExeInfoTable->NumberOfImages++;

  //
  // The new entry begins immediately after the previously serialized data.
  // Action and InfoSize are written unaligned because the entry start is not
  // guaranteed to be naturally aligned.
  //
  ImageExeInfoEntry = (EFI_IMAGE_EXECUTION_INFO *)((UINT8 *)NewImageExeInfoTable + ImageExeInfoTableSize);
  WriteUnaligned32 ((UINT32 *)ImageExeInfoEntry, Action);
  WriteUnaligned32 ((UINT32 *)((UINT8 *)ImageExeInfoEntry + sizeof (EFI_IMAGE_EXECUTION_ACTION)), (UINT32)NewImageExeInfoEntrySize);

  //
  // Name immediately follows the fixed entry header, then the device path,
  // then any signature data.
  //
  NameStr = (CHAR16 *)(ImageExeInfoEntry + 1);
  if (Name != NULL) {
    CopyMem ((UINT8 *)NameStr, Name, NameStringLen);
  } else {
    ZeroMem ((UINT8 *)NameStr, sizeof (CHAR16));
  }

  CopyMem (
    (UINT8 *)NameStr + NameStringLen,
    DevicePath,
    DevicePathSize
    );

  if (Signature != NULL) {
    CopyMem (
      (UINT8 *)NameStr + NameStringLen + DevicePathSize,
      Signature,
      SignatureSize
      );
  }

  //
  // Install/replace the table, then free the previous allocation.
  //
  Status = gBS->InstallConfigurationTable (&gEfiImageSecurityDatabaseGuid, (VOID *)NewImageExeInfoTable);
  if (EFI_ERROR (Status)) {
    FreePool (NewImageExeInfoTable);
    return Status;
  }

  if (ImageExeInfoTable != NULL) {
    FreePool (ImageExeInfoTable);
  }

  return EFI_SUCCESS;
}

/**
  Record a rejected image into the Image Execution Information Table.

  Appends a single EFI_IMAGE_EXECUTION_INFO entry describing the rejection. The
  image name is derived from File via ConvertDevicePathToText. When Digest is
  non-NULL it is wrapped as an EFI_SIGNATURE_LIST typed by HashType and recorded
  as the entry's signature (matching the legacy SIG_FOUND / SIG_FAILED
  behavior); otherwise no signature is recorded. All transient allocations are
  freed before return.

  @param[in]  File        Device path of the rejected image. Must be non-NULL.
  @param[in]  Action      The EFI_IMAGE_EXECUTION_ACTION describing why the image
                          was rejected.
  @param[in]  HashType    Image-hash algorithm GUID describing Digest. Ignored
                          when Digest is NULL.
  @param[in]  Digest      Optional image digest to record, or NULL to record no
                          signature.
  @param[in]  DigestSize  Size of Digest in bytes; must be non-zero when Digest
                          is non-NULL.
**/
VOID
RecordRejectedImage (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File,
  IN  EFI_IMAGE_EXECUTION_ACTION      Action,
  IN  CONST EFI_GUID                  *HashType,
  IN  CONST UINT8                     *Digest OPTIONAL,
  IN  UINTN                           DigestSize
  )
{
  EFI_STATUS          Status;
  CHAR16              *NameStr;
  EFI_SIGNATURE_LIST  *SignatureList;
  UINTN               SignatureListSize;

  SignatureList     = NULL;
  SignatureListSize = 0;

  //
  // Only signed-image rejections that the legacy implementation recorded with a
  // digest (SIG_FOUND / SIG_FAILED) supply one. When present, wrap it as a
  // signature list typed by the image-hash algorithm.
  //
  if ((Digest != NULL) && (DigestSize != 0) && (HashType != NULL)) {
    Status = BuildImageDigestSignatureList (HashType, Digest, DigestSize, &SignatureList, &SignatureListSize);
    if (EFI_ERROR (Status)) {
      SignatureList     = NULL;
      SignatureListSize = 0;
    }
  }

  NameStr = ConvertDevicePathToText (File, FALSE, TRUE);

  AddImageExeInfo (Action, NameStr, File, SignatureList, SignatureListSize);

  if (NameStr != NULL) {
    FreePool (NameStr);
  }

  if (SignatureList != NULL) {
    FreePool (SignatureList);
  }
}
