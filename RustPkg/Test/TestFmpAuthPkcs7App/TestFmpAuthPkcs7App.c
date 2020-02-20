/** @file
  A shell application that triggers capsule update process.

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestFmpAuthPkcs7App.h"

/**
  Create UX capsule.

  @retval EFI_SUCCESS            The capsule header is appended.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to create UX capsule.
**/
EFI_STATUS
TestFmpAuth (
  VOID
  )
{
  VOID                                          *FmpImageBuffer;
  UINTN                                         FmpImageBufferSize;
  VOID                                          *CertBuffer;
  UINTN                                         CertBufferSize;
  EFI_STATUS                                    Status;

  EFI_CAPSULE_HEADER                            *CapsuleHeader;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  UINT64                                        *ItemOffsetList;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader;
  UINT8                                         *Image;
  UINTN                                         ImageSize;

  FmpImageBuffer = NULL;
  CertBuffer = NULL;

  Status = ReadFileToBuffer(Argv[2], &FmpImageBufferSize, &FmpImageBuffer);
  if (EFI_ERROR(Status)) {
    Print(L"TestFmpAuthPkcs7App: FMP image (%s) is not found.\n", Argv[2]);
    goto Done;
  }
  Status = ReadFileToBuffer(Argv[4], &CertBufferSize, &CertBuffer);
  if (EFI_ERROR(Status)) {
    Print(L"TestFmpAuthPkcs7App: Cert (%s) is not found.\n", Argv[2]);
    goto Done;
  }

  CapsuleHeader = (EFI_CAPSULE_HEADER *)FmpImageBuffer;
  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *) ((UINT8 *) CapsuleHeader + CapsuleHeader->HeaderSize);
  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);
  ImageHeader  = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[0]);

  if (ImageHeader->Version >= EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) {
    Image = (UINT8 *)(ImageHeader + 1);
  } else {
    //
    // If the EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER is version 1,
    // Header should exclude UpdateHardwareInstance field
    //
    Image = (UINT8 *)ImageHeader + OFFSET_OF(EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, UpdateHardwareInstance);
  }

  ImageSize = FmpImageBufferSize - ((UINTN)Image - (UINTN)FmpImageBuffer);

  Status = AuthenticateFmpImage(
             (EFI_FIRMWARE_IMAGE_AUTHENTICATION *)Image,
             ImageSize,
             CertBuffer,
             CertBufferSize
             );
  Print(L"TestFmpAuthPkcs7App: Auth result (%r).\n", Status);

Done:
  if (FmpImageBuffer != NULL) {
    FreePool(FmpImageBuffer);
  }
  if (CertBuffer != NULL) {
    FreePool(CertBuffer);
  }

  return Status;
}

/**
  Print APP usage.
**/
VOID
PrintUsage (
  VOID
  )
{
  Print(L"TestFmpAuthPkcs7App:  usage\n");
  Print(L"  TestFmpAuthPkcs7App -I <Image> -C <Cert>\n");
  Print(L"Parameter:\n");
  Print(L"  -I:  FmpImage\n");
  Print(L"  -C:  Certificate\n");
}

/**
  Update Capsule image.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval EFI_SUCCESS            Command completed successfully.
  @retval EFI_UNSUPPORTED        Command usage unsupported.
  @retval EFI_INVALID_PARAMETER  Command usage invalid.
  @retval EFI_NOT_FOUND          The input file can't be found.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;

  Status = GetArg();
  if (EFI_ERROR(Status)) {
    Print(L"Please use UEFI SHELL to run this application!\n", Status);
    return Status;
  }
  if (Argc != 5) {
    PrintUsage();
    return EFI_UNSUPPORTED;
  }
  if (StrCmp(Argv[1], L"-I") != 0) {
    PrintUsage();
    return EFI_UNSUPPORTED;
  }
  if (StrCmp(Argv[3], L"-C") != 0) {
    PrintUsage();
    return EFI_UNSUPPORTED;
  }

  Status = TestFmpAuth ();
  return Status;
}
