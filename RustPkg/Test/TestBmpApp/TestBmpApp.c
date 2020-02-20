/** @file
  A shell application that triggers capsule update process.

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBmpApp.h"

/**
  Create UX capsule.

  @retval EFI_SUCCESS            The capsule header is appended.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to create UX capsule.
**/
EFI_STATUS
TestBmp (
  VOID
  )
{
  VOID                                          *BmpBuffer;
  UINTN                                         FileSize;
  CHAR16                                        *BmpName;
  EFI_STATUS                                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL                  *Gop;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION          *Info;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL                 *GopBlt;
  UINTN                                         GopBltSize;
  UINTN                                         Height;
  UINTN                                         Width;

  Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&Gop);
  if (EFI_ERROR(Status)) {
    Print(L"TestBmpApp: NO GOP is found.\n");
    return EFI_UNSUPPORTED;
  }
  Info = Gop->Mode->Info;
  Print(L"Current GOP: Mode - %d, ", Gop->Mode->Mode);
  Print(L"HorizontalResolution - %d, ", Info->HorizontalResolution);
  Print(L"VerticalResolution - %d\n", Info->VerticalResolution);
  // HorizontalResolution >= BMP_IMAGE_HEADER.PixelWidth
  // VerticalResolution   >= BMP_IMAGE_HEADER.PixelHeight

  if (Argc != 3) {
    Print(L"TestBmpApp: Incorrect parameter count.\n");
    return EFI_UNSUPPORTED;
  }

  GopBlt = NULL;
  BmpBuffer = NULL;
  FileSize = 0;

  BmpName = Argv[2];
  Status = ReadFileToBuffer(BmpName, &FileSize, &BmpBuffer);
  if (EFI_ERROR(Status)) {
    Print(L"TestBmpApp: BMP image (%s) is not found.\n", BmpName);
    goto Done;
  }

  Status = TranslateBmpToGopBlt (
             BmpBuffer,
             FileSize,
             &GopBlt,
             &GopBltSize,
             &Height,
             &Width
             );
  if (EFI_ERROR(Status)) {
    Print(L"TestBmpApp: BMP image (%s) is not valid.\n", BmpName);
    goto Done;
  }
  Print(L"BMP image (%s), Width - %d, Height - %d\n", BmpName, Width, Height);

  if (Height > Info->VerticalResolution) {
    Status = EFI_INVALID_PARAMETER;
    Print(L"TestBmpApp: BMP image (%s) height is larger than current resolution.\n", BmpName);
    goto Done;
  }
  if (Width > Info->HorizontalResolution) {
    Status = EFI_INVALID_PARAMETER;
    Print(L"TestBmpApp: BMP image (%s) width is larger than current resolution.\n", BmpName);
    goto Done;
  }

  //
  // Put bitmap 3/4 down the display.  If bitmap is too tall, then align bottom
  // of bitmap at bottom of display.
  //

  Status = Gop->Blt (
                  Gop,
                  GopBlt,
                  EfiBltBufferToVideo,
                  0,
                  0,
                  (Info->HorizontalResolution - Width) / 2, // OffsetX
                  MIN (
                    Info->VerticalResolution - Height,
                    ((3 * Info->VerticalResolution) - (2 * Height)) / 4
                    ),  // OffsetY
                  Width,
                  Height,
                  Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                  );

Done:
  if (GopBlt != NULL) {
    FreePool (GopBlt);
  }
  if (BmpBuffer != NULL) {
    FreePool(BmpBuffer);
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
  Print(L"TestBmpApp:  usage\n");
  Print(L"  TestBmpApp -G <BMP>\n");
  Print(L"Parameter:\n");
  Print(L"  -G:  a BMP file\n");
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
  if (Argc < 2) {
    PrintUsage();
    return EFI_UNSUPPORTED;
  }
  if (StrCmp(Argv[1], L"-G") == 0) {
    Status = TestBmp ();
    return Status;
  }

  return EFI_UNSUPPORTED;
}
