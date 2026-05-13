/** @file
  Image verification policy helpers for the DXE Image Verification Library.

  Data types, image source classifications, and APIs that resolve an
  EFI_DEVICE_PATH_PROTOCOL into an authorization policy that the verification
  handler should apply. The current policy is intentionally minimal: images
  loaded from a Firmware Volume are always allowed to execute; everything else is
  denied if validation fails.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Policy.h"

/**
  Determine whether the given device path resolves to a Firmware Volume.

  @param[in]   File       Device path describing the image origin.
  @param[out]  ImageType  On success, set to IMAGE_FROM_FV.

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

  //
  // Confirm the protocol is actually present on the resolved handle.
  //
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
