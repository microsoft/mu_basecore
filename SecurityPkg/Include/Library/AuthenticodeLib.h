/** @file
  Utilities for working with Windows Authenticode signatures on PE/COFF images.

  This library collects operations related to a PE/COFF image's Windows Authenticode signature;

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Guid/WinCertificate.h>

/**
  Assemble the Authenticode image (the byte stream the Windows Authenticode algorithm hashes) for a
  PE/COFF image.

  Produces the exact bytes hashed for the Authenticode digest: the image with the optional-header
  CheckSum field, the Certificate Table data-directory entry, and the trailing attribute-certificate
  table excluded.

  @param[in]   FileBuffer     In-memory PE/COFF image.
  @param[in]   FileSize       Size of FileBuffer in bytes.
  @param[out]  AuthImage      On success, a pool-allocated buffer (caller frees with FreePool ())
                              holding the assembled Authenticode image.
  @param[out]  AuthImageSize  On success, the length of AuthImage in bytes.

  @retval EFI_SUCCESS            AuthImage / AuthImageSize were populated.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL or FileSize is 0.
  @retval EFI_UNSUPPORTED        FileBuffer is an unsupported image type with no Authenticode form.
  @retval EFI_VOLUME_CORRUPTED   FileBuffer could not be parsed as a PE/COFF image.
  @retval EFI_OUT_OF_RESOURCES   An allocation failed.
**/
EFI_STATUS
EFIAPI
BuildAuthenticodeImage (
  IN  CONST VOID  *FileBuffer,
  IN  UINTN       FileSize,
  OUT UINT8       **AuthImage,
  OUT UINTN       *AuthImageSize
  );

/**
  Locate a PE/COFF image's embedded WIN_CERTIFICATE table (its attribute-certificate data).

  The table is a packed sequence of variable-length WIN_CERTIFICATE entries (each padded to an
  8-byte boundary); walk it by each entry's dwLength rather than indexing it as an array.

  @param[in]   FileBuffer       In-memory PE/COFF image.
  @param[in]   FileSize         Size of FileBuffer in bytes.
  @param[out]  WinCertificates  On success, a pointer into FileBuffer to the first WIN_CERTIFICATE,
                                or NULL when the image carries no certificate table.
  @param[out]  Length           On success, the certificate table length in bytes (0 when the image
                                carries none).

  @retval EFI_SUCCESS            WinCertificates / Length were populated.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_VOLUME_CORRUPTED   FileBuffer could not be parsed as a PE/COFF image.
**/
EFI_STATUS
EFIAPI
GetWinCertificates (
  IN  CONST VOID             *FileBuffer,
  IN  UINTN                  FileSize,
  OUT CONST WIN_CERTIFICATE  **WinCertificates,
  OUT UINTN                  *Length
  );
