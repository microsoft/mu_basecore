/** @file
  Utilities for working with Windows Authenticode signatures on PE/COFF images.

  This library collects operations related to a PE/COFF image's Windows Authenticode signature;

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/WinCertificate.h>
#include <IndustryStandard/PeImage.h>
#include <Library/AuthenticodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeCoffLib.h>

//
// The bytes of the in-memory PE/COFF image made available to PeCoffLib's ImageRead callback.
//
typedef struct {
  CONST UINT8    *Base;
  UINTN          Size;
} AUTHENTICODE_IMAGE_HANDLE;

typedef struct {
  UINTN                             ChecksumOffset;
  UINTN                             SecurityDirectoryOffset;
  UINT32                            NumberOfRvaAndSizes;
  UINT32                            SizeOfHeaders;
  CONST EFI_IMAGE_SECTION_HEADER    *SectionTable;
  UINT16                            NumberOfSections;
} AUTHENTICODE_IMAGE_LAYOUT;

/**
  Bounds-checked PE_COFF_LOADER_READ_FILE implementation over an in-memory image.

  Reads that start within the image are truncated to the available bytes; reads that start at or
  beyond the end of the image return zero bytes. This prevents PeCoffLib from ever reading past the
  supplied buffer.

  @param[in]      FileHandle  Pointer to an AUTHENTICODE_IMAGE_HANDLE describing the image bytes.
  @param[in]      FileOffset  Byte offset within the image to read from.
  @param[in,out]  ReadSize    On input, the bytes requested; on output, the bytes copied.
  @param[out]     Buffer      Destination buffer of at least the input *ReadSize bytes.

  @retval RETURN_SUCCESS            The read completed; *ReadSize is the number of bytes copied.
  @retval RETURN_INVALID_PARAMETER  FileHandle, ReadSize, or Buffer was NULL.
**/
STATIC
RETURN_STATUS
EFIAPI
AuthenticodeImageRead (
  IN     VOID   *FileHandle,
  IN     UINTN  FileOffset,
  IN OUT UINTN  *ReadSize,
  OUT    VOID   *Buffer
  )
{
  CONST AUTHENTICODE_IMAGE_HANDLE  *Handle;
  UINTN                            Available;

  if ((FileHandle == NULL) || (ReadSize == NULL) || (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  Handle = (CONST AUTHENTICODE_IMAGE_HANDLE *)FileHandle;

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
  Append a region of the image to the assembled Authenticode message, with bounds checks.

  @param[in]      FileBuffer  The image bytes.
  @param[in]      FileSize    Size of FileBuffer, and the capacity of Out.
  @param[in]      Offset      Start of the region within FileBuffer.
  @param[in]      Size        Length of the region in bytes.
  @param[out]     Out         Destination buffer of FileSize bytes.
  @param[in,out]  OutPos      Current write position in Out; advanced by Size on success.

  @retval EFI_SUCCESS           The region was copied (or Size was 0).
  @retval EFI_VOLUME_CORRUPTED  The region falls outside FileBuffer or would overflow Out.
**/
STATIC
EFI_STATUS
AppendImageRegion (
  IN     CONST UINT8  *FileBuffer,
  IN     UINTN        FileSize,
  IN     UINTN        Offset,
  IN     UINTN        Size,
  OUT    UINT8        *Out,
  IN OUT UINTN        *OutPos
  )
{
  if (Size == 0) {
    return EFI_SUCCESS;
  }

  //
  // The source region must lie within the image, and the copy must fit the output buffer (a
  // malformed image with overlapping regions could otherwise write past Out).
  //
  if ((Offset > FileSize) || (Size > FileSize - Offset) || (Size > FileSize - *OutPos)) {
    return EFI_VOLUME_CORRUPTED;
  }

  CopyMem (Out + *OutPos, FileBuffer + Offset, Size);
  *OutPos += Size;
  return EFI_SUCCESS;
}

/**
  Locate the Authenticode header exclusions and section table in a validated PE image.
**/
STATIC
EFI_STATUS
GetAuthenticodeImageLayout (
  IN  CONST UINT8                         *Image,
  IN  UINTN                               FileSize,
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  OUT AUTHENTICODE_IMAGE_LAYOUT           *Layout
  )
{
  CONST EFI_IMAGE_NT_HEADERS32  *Nt32;
  CONST EFI_IMAGE_NT_HEADERS64  *Nt64;
  UINT32                        PeOffset;
  UINT16                        Magic;
  UINTN                         SectionTableOffset;

  if (ImageContext->IsTeImage) {
    return EFI_UNSUPPORTED;
  }

  PeOffset              = ImageContext->PeCoffHeaderOffset;
  Layout->SizeOfHeaders = (UINT32)ImageContext->SizeOfHeaders;

  if ((PeOffset > FileSize) || (FileSize - PeOffset < sizeof (EFI_IMAGE_NT_HEADERS32))) {
    return EFI_VOLUME_CORRUPTED;
  }

  Nt32  = (CONST EFI_IMAGE_NT_HEADERS32 *)(Image + PeOffset);
  Magic = Nt32->OptionalHeader.Magic;

  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    Layout->NumberOfRvaAndSizes     = Nt32->OptionalHeader.NumberOfRvaAndSizes;
    Layout->ChecksumOffset          = (CONST UINT8 *)&Nt32->OptionalHeader.CheckSum - Image;
    Layout->SecurityDirectoryOffset = (CONST UINT8 *)&Nt32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY] - Image;
  } else if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    if (FileSize - PeOffset < sizeof (EFI_IMAGE_NT_HEADERS64)) {
      return EFI_VOLUME_CORRUPTED;
    }

    Nt64                            = (CONST EFI_IMAGE_NT_HEADERS64 *)(Image + PeOffset);
    Layout->NumberOfRvaAndSizes     = Nt64->OptionalHeader.NumberOfRvaAndSizes;
    Layout->ChecksumOffset          = (CONST UINT8 *)&Nt64->OptionalHeader.CheckSum - Image;
    Layout->SecurityDirectoryOffset = (CONST UINT8 *)&Nt64->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY] - Image;
  } else {
    return EFI_VOLUME_CORRUPTED;
  }

  Layout->NumberOfSections = Nt32->FileHeader.NumberOfSections;
  SectionTableOffset       = (UINTN)PeOffset + sizeof (UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + Nt32->FileHeader.SizeOfOptionalHeader;

  if ((SectionTableOffset > FileSize) ||
      ((FileSize - SectionTableOffset) / sizeof (EFI_IMAGE_SECTION_HEADER) < Layout->NumberOfSections))
  {
    return EFI_VOLUME_CORRUPTED;
  }

  Layout->SectionTable = (CONST EFI_IMAGE_SECTION_HEADER *)(Image + SectionTableOffset);
  return EFI_SUCCESS;
}

/**
  Append headers, excluding the checksum and any Certificate Table directory entry.
**/
STATIC
EFI_STATUS
AppendAuthenticodeHeaders (
  IN     CONST UINT8                      *Image,
  IN     UINTN                            FileSize,
  IN     CONST AUTHENTICODE_IMAGE_LAYOUT  *Layout,
  OUT    UINT8                            *Out,
  IN OUT UINTN                            *OutPos
  )
{
  EFI_STATUS  Status;
  UINTN       ChecksumEnd;
  UINTN       SecurityDirectoryEnd;

  ChecksumEnd = Layout->ChecksumOffset + sizeof (UINT32);

  //
  // From file offset zero through the optional-header fields preceding CheckSum.
  // Includes the DOS header/stub when present, PE signature, and COFF file header.
  //
  Status = AppendImageRegion (Image, FileSize, 0, Layout->ChecksumOffset, Out, OutPos);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Layout->NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
    if (Layout->SizeOfHeaders < ChecksumEnd) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // No Certificate Table directory entry exists: include everything after CheckSum
    // through SizeOfHeaders, including section headers and header padding.
    //
    return AppendImageRegion (Image, FileSize, ChecksumEnd, Layout->SizeOfHeaders - ChecksumEnd, Out, OutPos);
  }

  if (Layout->SecurityDirectoryOffset < ChecksumEnd) {
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Skip CheckSum; include the remaining optional-header fields and data directories
  // preceding the Certificate Table entry, stopping before that entry.
  //
  Status = AppendImageRegion (Image, FileSize, ChecksumEnd, Layout->SecurityDirectoryOffset - ChecksumEnd, Out, OutPos);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SecurityDirectoryEnd = Layout->SecurityDirectoryOffset + sizeof (EFI_IMAGE_DATA_DIRECTORY);
  if (Layout->SizeOfHeaders < SecurityDirectoryEnd) {
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Skip the Certificate Table directory entry; include the remaining data directories,
  // section headers, and header padding through SizeOfHeaders.
  //
  return AppendImageRegion (Image, FileSize, SecurityDirectoryEnd, Layout->SizeOfHeaders - SecurityDirectoryEnd, Out, OutPos);
}

/**
  Append sections in raw-file-offset order and update the combined header and raw-section size.
**/
STATIC
EFI_STATUS
AppendSections (
  IN     CONST UINT8                      *Image,
  IN     UINTN                            FileSize,
  IN     CONST AUTHENTICODE_IMAGE_LAYOUT  *Layout,
  OUT    UINT8                            *Out,
  IN OUT UINTN                            *OutPos,
  IN OUT UINTN                            *HeaderAndSectionSize
  )
{
  EFI_IMAGE_SECTION_HEADER  *Sorted;
  UINTN                     Index;
  UINTN                     Pos;
  EFI_STATUS                Status;

  if (Layout->NumberOfSections == 0) {
    return EFI_SUCCESS;
  }

  Sorted = AllocateZeroPool ((UINTN)Layout->NumberOfSections * sizeof (EFI_IMAGE_SECTION_HEADER));
  if (Sorted == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Insertion-sort header copies by PointerToRawData: Authenticode requires raw section
  // data in ascending file-offset order, which may differ from section-header order.
  //
  for (Index = 0; Index < Layout->NumberOfSections; Index++) {
    Pos = Index;
    while ((Pos > 0) && (Layout->SectionTable[Index].PointerToRawData < Sorted[Pos - 1].PointerToRawData)) {
      CopyMem (&Sorted[Pos], &Sorted[Pos - 1], sizeof (EFI_IMAGE_SECTION_HEADER));
      Pos--;
    }

    CopyMem (&Sorted[Pos], &Layout->SectionTable[Index], sizeof (EFI_IMAGE_SECTION_HEADER));
  }

  Status = EFI_SUCCESS;
  for (Index = 0; Index < Layout->NumberOfSections; Index++) {
    if (Sorted[Index].SizeOfRawData == 0) {
      continue;
    }

    Status = AppendImageRegion (Image, FileSize, Sorted[Index].PointerToRawData, Sorted[Index].SizeOfRawData, Out, OutPos);
    if (EFI_ERROR (Status)) {
      break;
    }

    *HeaderAndSectionSize += Sorted[Index].SizeOfRawData;
  }

  FreePool (Sorted);
  return Status;
}

/**
  Assemble the Authenticode byte stream, excluding the checksum, Certificate Table
  directory entry, and trailing attribute-certificate table.

  @param[in]   FileBuffer     In-memory PE/COFF image.
  @param[in]   FileSize       Size of FileBuffer in bytes.
  @param[out]  AuthImage      Pool-allocated buffer; caller frees with FreePool ().
  @param[out]  AuthImageSize  Length of AuthImage in bytes.

  @retval EFI_SUCCESS            The Authenticode image was assembled.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL or FileSize is 0.
  @retval EFI_UNSUPPORTED        The image has no Authenticode form.
  @retval EFI_VOLUME_CORRUPTED   The image could not be parsed.
  @retval EFI_OUT_OF_RESOURCES   An allocation failed.
**/
EFI_STATUS
EFIAPI
BuildAuthenticodeImage (
  IN  CONST VOID  *FileBuffer,
  IN  UINTN       FileSize,
  OUT UINT8       **AuthImage,
  OUT UINTN       *AuthImageSize
  )
{
  RETURN_STATUS                 PeCoffStatus;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  AUTHENTICODE_IMAGE_HANDLE     Handle;
  AUTHENTICODE_IMAGE_LAYOUT     Layout;
  UINT8                         *Out;
  UINTN                         OutPos;
  UINTN                         HeaderAndSectionSize;
  UINTN                         Remaining;
  UINT32                        CertSize;
  EFI_STATUS                    Status;

  if ((FileBuffer == NULL) || (AuthImage == NULL) || (AuthImageSize == NULL) || (FileSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  *AuthImage     = NULL;
  *AuthImageSize = 0;

  ZeroMem (&ImageContext, sizeof (ImageContext));
  Handle.Base            = FileBuffer;
  Handle.Size            = FileSize;
  ImageContext.Handle    = &Handle;
  ImageContext.ImageRead = AuthenticodeImageRead;

  PeCoffStatus = PeCoffLoaderGetImageInfo (&ImageContext);
  if (RETURN_ERROR (PeCoffStatus)) {
    DEBUG ((DEBUG_INFO, "AuthenticodeLib: invalid PE/COFF image (0x%lx).\n", (UINT64)PeCoffStatus));
    return EFI_VOLUME_CORRUPTED;
  }

  Status = GetAuthenticodeImageLayout (FileBuffer, FileSize, &ImageContext, &Layout);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Out = AllocatePool (FileSize);
  if (Out == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OutPos = 0;
  Status = AppendAuthenticodeHeaders (FileBuffer, FileSize, &Layout, Out, &OutPos);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  HeaderAndSectionSize = Layout.SizeOfHeaders;
  Status               = AppendSections (FileBuffer, FileSize, &Layout, Out, &OutPos, &HeaderAndSectionSize);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Trailing data after the sections, excluding the attribute-certificate table (CertSize bytes).
  //
  CertSize = ImageContext.SecurityDataDirectory.Size;
  if (FileSize > HeaderAndSectionSize) {
    Remaining = FileSize - HeaderAndSectionSize;
    if (Remaining > CertSize) {
      Status = AppendImageRegion (FileBuffer, FileSize, HeaderAndSectionSize, Remaining - CertSize, Out, &OutPos);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
    } else if (Remaining < CertSize) {
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }
  }

  *AuthImage     = Out;
  *AuthImageSize = OutPos;
  Out            = NULL;
  Status         = EFI_SUCCESS;

Done:
  if (Out != NULL) {
    FreePool (Out);
  }

  return Status;
}

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
  )
{
  RETURN_STATUS                 PeCoffStatus;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  AUTHENTICODE_IMAGE_HANDLE     Handle;
  UINT32                        VirtualAddress;
  UINT32                        Size;

  if ((FileBuffer == NULL) || (WinCertificates == NULL) || (Length == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *WinCertificates = NULL;
  *Length          = 0;

  ZeroMem (&ImageContext, sizeof (ImageContext));
  Handle.Base            = (CONST UINT8 *)FileBuffer;
  Handle.Size            = FileSize;
  ImageContext.Handle    = &Handle;
  ImageContext.ImageRead = AuthenticodeImageRead;

  PeCoffStatus = PeCoffLoaderGetImageInfo (&ImageContext);
  if (RETURN_ERROR (PeCoffStatus)) {
    DEBUG ((DEBUG_INFO, "AuthenticodeLib: invalid PE/COFF image (0x%lx).\n", (UINT64)PeCoffStatus));
    return EFI_VOLUME_CORRUPTED;
  }

  VirtualAddress = ImageContext.SecurityDataDirectory.VirtualAddress;
  Size           = ImageContext.SecurityDataDirectory.Size;

  //
  // A zeroed directory means the image carries no certificate table (an unsigned image).
  //
  if (Size == 0) {
    return EFI_SUCCESS;
  }

  //
  // PeCoffLoaderGetImageInfo validates the directory lies within the image; re-check before
  // handing out a pointer into FileBuffer.
  //
  if ((VirtualAddress > FileSize) || (Size > FileSize - VirtualAddress)) {
    return EFI_VOLUME_CORRUPTED;
  }

  *WinCertificates = (CONST WIN_CERTIFICATE *)((CONST UINT8 *)FileBuffer + VirtualAddress);
  *Length          = Size;
  return EFI_SUCCESS;
}
