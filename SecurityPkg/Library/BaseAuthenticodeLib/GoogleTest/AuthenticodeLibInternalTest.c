/** @file
  Compile AuthenticodeLib into the fault-test module to exercise private range checks.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

// MU_CHANGE - NEW FILE

#include "../BaseAuthenticodeLib.c"
#include "AuthenticodeLibInternalTest.h"

EFI_STATUS
EFIAPI
TestGetAuthenticodeImageLayout (
  IN CONST UINT8                         *Image,
  IN UINTN                               FileSize,
  IN CONST PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN BOOLEAN                             NullLayout
  )
{
  AUTHENTICODE_IMAGE_LAYOUT  Layout;

  return GetAuthenticodeImageLayout (Image, FileSize, ImageContext, NullLayout ? NULL : &Layout);
}

EFI_STATUS
EFIAPI
TestAppendImageRegion (
  IN     CONST UINT8  *Image,
  IN     UINTN        FileSize,
  IN     UINTN        Offset,
  IN     UINTN        Size,
  OUT    UINT8        *Out,
  IN OUT UINTN        *OutPos
  )
{
  return AppendImageRegion (Image, FileSize, Offset, Size, Out, OutPos);
}

EFI_STATUS
EFIAPI
TestAppendAuthenticodeHeaders (
  IN     CONST UINT8  *Image,
  IN     UINTN        FileSize,
  IN     UINTN        ChecksumOffset,
  IN     UINTN        SecurityDirectoryOffset,
  IN     UINTN        SizeOfHeaders,
  IN     UINT32       NumberOfRvaAndSizes,
  OUT    UINT8        *Out,
  IN OUT UINTN        *OutPos
  )
{
  AUTHENTICODE_IMAGE_LAYOUT  Layout;

  ZeroMem (&Layout, sizeof (Layout));
  Layout.ChecksumOffset          = ChecksumOffset;
  Layout.SecurityDirectoryOffset = SecurityDirectoryOffset;
  Layout.SizeOfHeaders           = SizeOfHeaders;
  Layout.NumberOfRvaAndSizes     = NumberOfRvaAndSizes;
  return AppendAuthenticodeHeaders (Image, FileSize, &Layout, Out, OutPos);
}

EFI_STATUS
EFIAPI
TestAppendAuthenticodeHeadersNullLayout (
  IN     CONST UINT8  *Image,
  IN     UINTN        FileSize,
  OUT    UINT8        *Out,
  IN OUT UINTN        *OutPos
  )
{
  return AppendAuthenticodeHeaders (Image, FileSize, NULL, Out, OutPos);
}

EFI_STATUS
EFIAPI
TestAppendSections (
  IN     CONST UINT8  *Image,
  IN     UINTN        FileSize,
  IN     UINTN        SectionTableOffset,
  IN     UINT16       NumberOfSections,
  IN     UINTN        SizeOfHeaders,
  IN     UINTN        DataEnd,
  OUT    UINT8        *Out,
  IN OUT UINTN        *OutPos,
  IN OUT UINTN        *NextFileOffset
  )
{
  AUTHENTICODE_IMAGE_LAYOUT  Layout;

  ZeroMem (&Layout, sizeof (Layout));
  Layout.SectionTableOffset = SectionTableOffset;
  Layout.NumberOfSections   = NumberOfSections;
  Layout.SizeOfHeaders      = SizeOfHeaders;
  return AppendSections (Image, FileSize, &Layout, DataEnd, Out, OutPos, NextFileOffset);
}

EFI_STATUS
EFIAPI
TestAppendRemainingImageData (
  IN     CONST UINT8  *Image,
  IN     UINTN        FileSize,
  IN     UINTN        NextFileOffset,
  IN     UINTN        DataEnd,
  OUT    UINT8        *Out,
  IN OUT UINTN        *OutPos
  )
{
  return AppendRemainingImageData (Image, FileSize, NextFileOffset, DataEnd, Out, OutPos);
}
