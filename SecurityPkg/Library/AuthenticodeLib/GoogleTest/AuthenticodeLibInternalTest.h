/** @file
  Test-only adapters for AuthenticodeLib's internal range checks.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

EFI_STATUS
EFIAPI
TestAppendImageRegion (
  IN     CONST UINT8  *Image,
  IN     UINTN        FileSize,
  IN     UINTN        Offset,
  IN     UINTN        Size,
  OUT    UINT8        *Out,
  IN OUT UINTN        *OutPos
  );

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
  );

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
  );

EFI_STATUS
EFIAPI
TestAppendRemainingImageData (
  IN     CONST UINT8  *Image,
  IN     UINTN        FileSize,
  IN     UINTN        NextFileOffset,
  IN     UINTN        DataEnd,
  OUT    UINT8        *Out,
  IN OUT UINTN        *OutPos
  );
