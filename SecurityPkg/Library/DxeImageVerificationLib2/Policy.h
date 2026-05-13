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

#ifndef DXE_IMAGE_VERIFICATION_LIB_POLICY_H_
#define DXE_IMAGE_VERIFICATION_LIB_POLICY_H_

#include "DxeImageVerificationLib.h"
#include <Pi/PiFirmwareFile.h>
#include <Pi/PiFirmwareVolume.h>
#include <Protocol/FirmwareVolume2.h>

//
// Authorization policy bit definition
//
#define ALWAYS_EXECUTE                      0x00000000
#define DENY_EXECUTE_ON_SECURITY_VIOLATION  0x00000001

//
// Image type definitions
//
#define IMAGE_UNKNOWN  0x00000000
#define IMAGE_FROM_FV  0x00000001

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

#endif // DXE_IMAGE_VERIFICATION_LIB_POLICY_H_
