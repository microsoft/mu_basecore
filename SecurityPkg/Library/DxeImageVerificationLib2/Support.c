/** @file
  Utility functions for DxeImageVerificationLib.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Support.h"

//
// Caller-owned state passed through PeCoffLoaderGetImageInfo() to
// SecurityDirectoryCallback() and back.
//
typedef struct {
  EFI_IMAGE_DATA_DIRECTORY    SecDataDir;
} SECURITY_DIR_CALLBACK_CTX;

//
// Image Handle that contains the image bytes and size.
//
typedef struct {
  CONST UINT8    *Base;
  UINTN          Size;
} PE_COFF_IMAGE_HANDLE;

/**
  Buffer size aware implementation of PE_COFF_LOADER_READ_FILE.

  @param[in]      FileHandle  Pointer to a PE_COFF_IMAGE_HANDLE describing
                              the image bytes available to PeCoffLib.
  @param[in]      FileOffset  Byte offset within the image to read from.
  @param[in,out]  ReadSize    On input, bytes requested. On output, bytes
                              actually copied (may be 0 or less than
                              requested if the range extends past EOF).
  @param[out]     Buffer      Destination buffer of at least the input
                              *ReadSize bytes.

  @retval RETURN_SUCCESS            Read succeeded; *ReadSize is the
                                    number of bytes copied.
  @retval RETURN_INVALID_PARAMETER  FileHandle, ReadSize, or Buffer was
                                    NULL.
**/
STATIC
RETURN_STATUS
EFIAPI
BoundedImageRead (
  IN     VOID   *FileHandle,
  IN     UINTN  FileOffset,
  IN OUT UINTN  *ReadSize,
  OUT    VOID   *Buffer
  )
{
  CONST PE_COFF_IMAGE_HANDLE  *Handle;
  UINTN                       Available;

  if ((FileHandle == NULL) || (ReadSize == NULL) || (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  Handle = (CONST PE_COFF_IMAGE_HANDLE *)FileHandle;

  if (FileOffset >= Handle->Size) {
    *ReadSize = 0;
    return RETURN_SUCCESS;
  }

  Available = Handle->Size - FileOffset;
  if (*ReadSize > Available) {
    *ReadSize = Available;
  }

  CopyMem (Buffer, Handle->Base + FileOffset, *ReadSize);
  return RETURN_SUCCESS;
}

/**
  PE_COFF_LOADER_DATA_DIRECTORY_CALLBACK that captures the EFI_IMAGE_DATA_DIRECTORY
  for the EFI_IMAGE_DIRECTORY_ENTRY_SECURITY data directory.

  @param[in]  Index            Data directory.
  @param[in]  DataDirectory    The directory entry.
  @param[out]  CallbackContext  Pointer to a SECURITY_DIR_CALLBACK_CTX.

  @retval RETURN_SUCCESS  Always.
**/
STATIC
RETURN_STATUS
EFIAPI
SecurityDirectoryCallback (
  IN UINT32                          Index,
  IN CONST EFI_IMAGE_DATA_DIRECTORY  *DataDirectory,
  OUT VOID                           *CallbackContext  OPTIONAL
  )
{
  SECURITY_DIR_CALLBACK_CTX  *Ctx;

  if (Index != EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
    return RETURN_SUCCESS;
  }

  Ctx             = (SECURITY_DIR_CALLBACK_CTX *)CallbackContext;
  Ctx->SecDataDir = *DataDirectory;
  return RETURN_SUCCESS;
}

/**
  Locate the EFI_IMAGE_DIRECTORY_ENTRY_SECURITY data directory in the
  PE/COFF image contained in FileBuffer.

  Caution: This function may receive untrusted input. The PE/COFF image
  is external input and is bounds-checked by PeCoffLib before any field
  is dereferenced.

  @param[in]   FileBuffer  Pointer to the in-memory PE/COFF image.
  @param[in]   FileSize    Size of FileBuffer in bytes.
  @param[out]  SecDataDir  On success, filled with a copy of the image's
                           security data directory entry. Zeroed when
                           the image declares no security directory.

  @retval EFI_SUCCESS            SecDataDir has been populated.
  @retval EFI_INVALID_PARAMETER  FileBuffer or SecDataDir is NULL.
  @retval EFI_LOAD_ERROR         FileBuffer does not contain a valid
                                 PE/COFF image, or PeCoffLib otherwise
                                 rejected the headers.
**/
EFI_STATUS
GetImageSecurityDataDirectory (
  IN  VOID                      *FileBuffer,
  IN  UINTN                     FileSize,
  OUT EFI_IMAGE_DATA_DIRECTORY  *SecDataDir
  )
{
  RETURN_STATUS                 PeCoffStatus;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  SECURITY_DIR_CALLBACK_CTX     CallbackCtx;
  PE_COFF_IMAGE_HANDLE          Handle;

  if ((FileBuffer == NULL) || (SecDataDir == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (SecDataDir, sizeof (*SecDataDir));
  ZeroMem (&CallbackCtx, sizeof (CallbackCtx));

  Handle.Base = (CONST UINT8 *)FileBuffer;
  Handle.Size = FileSize;

  //
  // Delegate header parsing, signature validation, optional-header
  // magic checks, and security-directory bounds checking to PeCoffLib.
  // The callback receives the validated security data directory entry.
  //
  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle                   = &Handle;
  ImageContext.ImageRead                = BoundedImageRead;
  ImageContext.DataDirectoryRead        = SecurityDirectoryCallback;
  ImageContext.DataDirectoryReadContext = &CallbackCtx;

  PeCoffStatus = PeCoffLoaderGetImageInfo (&ImageContext);
  if (RETURN_ERROR (PeCoffStatus)) {
    DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: PeImage invalid (0x%lx).\n", (UINT64)PeCoffStatus));
    return EFI_LOAD_ERROR;
  }

  *SecDataDir = CallbackCtx.SecDataDir;
  return EFI_SUCCESS;
}
