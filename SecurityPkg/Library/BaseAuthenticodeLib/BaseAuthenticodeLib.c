/** @file
  Utilities for working with Windows Authenticode signatures on PE/COFF images.

  This library collects operations related to a PE/COFF image's Windows Authenticode signature.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

// MU_CHANGE - NEW FILE

#include <Uefi.h>

#include <Guid/WinCertificate.h>
#include <IndustryStandard/PeImage.h>
#include <Library/AuthenticodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeCoffLib.h>
#include <Library/SafeIntLib.h>

//
// The bytes of the in-memory PE/COFF image made available to PeCoffLib's ImageRead callback.
//
typedef struct {
  CONST UINT8    *Base;
  UINTN          Size;
} AUTHENTICODE_IMAGE_HANDLE;

typedef struct {
  UINTN     ChecksumOffset;
  UINTN     SecurityDirectoryOffset;
  UINTN     SectionTableOffset;
  UINTN     SizeOfHeaders;
  UINT32    NumberOfRvaAndSizes;
  UINT16    NumberOfSections;
} AUTHENTICODE_IMAGE_LAYOUT;

typedef struct {
  UINT32    Offset;
  UINT32    Size;
} AUTHENTICODE_SECTION;

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
  Check that a range lies entirely within a buffer, without overflowing.

  @param[in]  BufferSize  Size of the buffer in bytes.
  @param[in]  Offset      Start of the range within the buffer.
  @param[in]  Size        Length of the range in bytes.

  @retval TRUE   The range fits within the buffer.
  @retval FALSE  The range extends beyond the buffer.
**/
STATIC
BOOLEAN
IsBufferRangeValid (
  IN UINTN  BufferSize,
  IN UINTN  Offset,
  IN UINTN  Size
  )
{
  return (BOOLEAN)((Offset <= BufferSize) && (Size <= BufferSize - Offset));
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
  if (!IsBufferRangeValid (FileSize, Offset, Size) ||
      !IsBufferRangeValid (FileSize, *OutPos, Size))
  {
    return EFI_VOLUME_CORRUPTED;
  }

  if (Size != 0) {
    CopyMem (Out + *OutPos, FileBuffer + Offset, Size);
    *OutPos += Size;
  }

  return EFI_SUCCESS;
}

/**
  Locate the Authenticode header exclusions and section table in a validated PE image.

  @param[in]   Image         The PE/COFF image bytes.
  @param[in]   FileSize      Size of Image in bytes.
  @param[in]   ImageContext  Context populated by PeCoffLoaderGetImageInfo().
  @param[out]  Layout        On success, the header offsets, sizes, and section-table information.

  @retval EFI_SUCCESS            The layout was populated.
  @retval EFI_INVALID_PARAMETER  Image, ImageContext, or Layout is NULL.
  @retval EFI_UNSUPPORTED        The image is a TE image.
  @retval EFI_VOLUME_CORRUPTED   The image headers or layout fields are invalid.
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
  EFI_IMAGE_NT_HEADERS32  Nt32;
  EFI_IMAGE_NT_HEADERS64  Nt64;
  UINTN                   PeOffset;
  UINTN                   ChecksumRelativeOffset;
  UINTN                   DirectoryRelativeOffset;
  UINTN                   DirectoryArrayOffset;
  UINTN                   DirectoryArraySize;
  UINTN                   ExpectedOptionalHeaderSize;
  UINTN                   OptionalHeaderOffset;
  UINTN                   SectionTableSize;

  if ((Image == NULL) || (ImageContext == NULL) || (Layout == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ImageContext->IsTeImage) {
    return EFI_UNSUPPORTED;
  }

  PeOffset              = ImageContext->PeCoffHeaderOffset;
  Layout->SizeOfHeaders = ImageContext->SizeOfHeaders;

  if ((Layout->SizeOfHeaders > FileSize) ||
      !IsBufferRangeValid (FileSize, PeOffset, sizeof (Nt32)))
  {
    return EFI_VOLUME_CORRUPTED;
  }

  CopyMem (&Nt32, Image + PeOffset, sizeof (Nt32));

  if (Nt32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    Layout->NumberOfRvaAndSizes = Nt32.OptionalHeader.NumberOfRvaAndSizes;
    ChecksumRelativeOffset      = OFFSET_OF (EFI_IMAGE_NT_HEADERS32, OptionalHeader.CheckSum);
    DirectoryRelativeOffset     = OFFSET_OF (EFI_IMAGE_NT_HEADERS32, OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]);
    DirectoryArrayOffset        = OFFSET_OF (EFI_IMAGE_OPTIONAL_HEADER32, DataDirectory);
  } else if (Nt32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    if (!IsBufferRangeValid (FileSize, PeOffset, sizeof (Nt64))) {
      return EFI_VOLUME_CORRUPTED;
    }

    CopyMem (&Nt64, Image + PeOffset, sizeof (Nt64));
    Layout->NumberOfRvaAndSizes = Nt64.OptionalHeader.NumberOfRvaAndSizes;
    ChecksumRelativeOffset      = OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader.CheckSum);
    DirectoryRelativeOffset     = OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]);
    DirectoryArrayOffset        = OFFSET_OF (EFI_IMAGE_OPTIONAL_HEADER64, DataDirectory);
  } else {
    return EFI_VOLUME_CORRUPTED;
  }

  Layout->NumberOfSections = Nt32.FileHeader.NumberOfSections;
  if ((Layout->NumberOfRvaAndSizes > EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES) ||
      RETURN_ERROR (SafeUintnMult (Layout->NumberOfRvaAndSizes, sizeof (EFI_IMAGE_DATA_DIRECTORY), &DirectoryArraySize)) ||
      RETURN_ERROR (SafeUintnAdd (DirectoryArrayOffset, DirectoryArraySize, &ExpectedOptionalHeaderSize)) ||
      (Nt32.FileHeader.SizeOfOptionalHeader != ExpectedOptionalHeaderSize) ||
      RETURN_ERROR (SafeUintnAdd (PeOffset, ChecksumRelativeOffset, &Layout->ChecksumOffset)) ||
      RETURN_ERROR (SafeUintnAdd (PeOffset, DirectoryRelativeOffset, &Layout->SecurityDirectoryOffset)) ||
      RETURN_ERROR (SafeUintnAdd (PeOffset, OFFSET_OF (EFI_IMAGE_NT_HEADERS32, OptionalHeader), &OptionalHeaderOffset)) ||
      RETURN_ERROR (SafeUintnAdd (OptionalHeaderOffset, Nt32.FileHeader.SizeOfOptionalHeader, &Layout->SectionTableOffset)) ||
      RETURN_ERROR (SafeUintnMult (Layout->NumberOfSections, sizeof (EFI_IMAGE_SECTION_HEADER), &SectionTableSize)) ||
      !IsBufferRangeValid (Layout->SizeOfHeaders, Layout->ChecksumOffset, sizeof (UINT32)) ||
      ((Layout->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) &&
       !IsBufferRangeValid (Layout->SizeOfHeaders, Layout->SecurityDirectoryOffset, sizeof (EFI_IMAGE_DATA_DIRECTORY))) ||
      !IsBufferRangeValid (Layout->SizeOfHeaders, Layout->SectionTableOffset, SectionTableSize))
  {
    return EFI_VOLUME_CORRUPTED;
  }

  return EFI_SUCCESS;
}

/**
  Append headers, excluding the checksum and any Certificate Table directory entry.

  Out and OutPos may be partially updated if an error occurs.

  @param[in]      Image     The PE/COFF image bytes.
  @param[in]      FileSize  Size of Image in bytes, and the capacity of Out.
  @param[in]      Layout    Validated header exclusions and sizes.
  @param[out]     Out       Destination buffer of FileSize bytes.
  @param[in,out]  OutPos    Current write position in Out; advanced by the bytes copied.

  @retval EFI_SUCCESS            The headers were appended.
  @retval EFI_INVALID_PARAMETER  Image, Layout, Out, or OutPos is NULL.
  @retval EFI_VOLUME_CORRUPTED   A header exclusion or source or destination range is invalid.
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

  if ((Image == NULL) || (Layout == NULL) || (Out == NULL) || (OutPos == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RETURN_ERROR (SafeUintnAdd (Layout->ChecksumOffset, sizeof (UINT32), &ChecksumEnd)) ||
      (ChecksumEnd > FileSize))
  {
    return EFI_VOLUME_CORRUPTED;
  }

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

  if (RETURN_ERROR (SafeUintnAdd (Layout->SecurityDirectoryOffset, sizeof (EFI_IMAGE_DATA_DIRECTORY), &SecurityDirectoryEnd)) ||
      (Layout->SizeOfHeaders < SecurityDirectoryEnd))
  {
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Skip the Certificate Table directory entry; include the remaining data directories,
  // section headers, and header padding through SizeOfHeaders.
  //
  return AppendImageRegion (Image, FileSize, SecurityDirectoryEnd, Layout->SizeOfHeaders - SecurityDirectoryEnd, Out, OutPos);
}

/**
  Restore the max-heap property without recursion.

  The child subtrees of Root must already satisfy the max-heap property.

  @param[in,out]  Sections  Array of section ranges containing the heap.
  @param[in]      Root      Index of the root of the subtree to restore.
  @param[in]      Count     Number of entries in the heap.
**/
STATIC
VOID
SiftSectionHeap (
  IN OUT AUTHENTICODE_SECTION  *Sections,
  IN     UINTN                 Root,
  IN     UINTN                 Count
  )
{
  UINTN                 Child;
  AUTHENTICODE_SECTION  Swap;

  while (Root < Count / 2) {
    Child = Root * 2 + 1;
    if ((Child + 1 < Count) && (Sections[Child].Offset < Sections[Child + 1].Offset)) {
      Child++;
    }

    if (Sections[Root].Offset >= Sections[Child].Offset) {
      return;
    }

    Swap            = Sections[Root];
    Sections[Root]  = Sections[Child];
    Sections[Child] = Swap;
    Root            = Child;
  }
}

/**
  Sort section ranges by file offset in O(n log n) time and constant stack space.

  @param[in,out]  Sections  Array of section ranges, sorted by file offset on return.
  @param[in]      Count     Number of section ranges in the array.
**/
STATIC
VOID
SortSections (
  IN OUT AUTHENTICODE_SECTION  *Sections,
  IN     UINTN                 Count
  )
{
  UINTN                 Index;
  AUTHENTICODE_SECTION  Swap;

  for (Index = Count / 2; Index > 0; Index--) {
    SiftSectionHeap (Sections, Index - 1, Count);
  }

  for (Index = Count; Index > 1; Index--) {
    Swap                = Sections[0];
    Sections[0]         = Sections[Index - 1];
    Sections[Index - 1] = Swap;
    SiftSectionHeap (Sections, 0, Index - 1);
  }
}

/**
  Append non-overlapping sections and intervening file data in file-offset order.

  Sections with no raw data are skipped. Out, OutPos, and NextFileOffset may be
  partially updated if an error occurs.

  @param[in]      Image           The PE/COFF image bytes.
  @param[in]      FileSize        Size of Image in bytes, and the capacity of Out.
  @param[in]      Layout          Validated section-table and header information.
  @param[in]      DataEnd         Exclusive end of the image data to include.
  @param[out]     Out             Destination buffer of FileSize bytes.
  @param[in,out]  OutPos          Current write position in Out; advanced by the bytes copied.
  @param[in,out]  NextFileOffset  First unprocessed file offset; advanced past appended sections.

  @retval EFI_SUCCESS           The section ranges and intervening data were appended.
  @retval EFI_VOLUME_CORRUPTED  A section, its header, or a source or destination range is invalid.
  @retval EFI_OUT_OF_RESOURCES  The section sort buffer could not be allocated.
**/
STATIC
EFI_STATUS
AppendSections (
  IN     CONST UINT8                      *Image,
  IN     UINTN                            FileSize,
  IN     CONST AUTHENTICODE_IMAGE_LAYOUT  *Layout,
  IN     UINTN                            DataEnd,
  OUT    UINT8                            *Out,
  IN OUT UINTN                            *OutPos,
  IN OUT UINTN                            *NextFileOffset
  )
{
  EFI_IMAGE_SECTION_HEADER  Header;
  AUTHENTICODE_SECTION      *Sections;
  UINTN                     AllocationSize;
  UINTN                     HeaderOffset;
  UINTN                     SectionCount;
  UINTN                     SectionEnd;
  UINTN                     Index;
  EFI_STATUS                Status;

  if (Layout->NumberOfSections == 0) {
    return EFI_SUCCESS;
  }

  if (RETURN_ERROR (SafeUintnMult (Layout->NumberOfSections, sizeof (*Sections), &AllocationSize))) {
    return EFI_VOLUME_CORRUPTED;
  }

  Sections = AllocatePool (AllocationSize);
  if (Sections == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HeaderOffset = Layout->SectionTableOffset;
  SectionCount = 0;
  Status       = EFI_SUCCESS;
  for (Index = 0; Index < Layout->NumberOfSections; Index++) {
    if (!IsBufferRangeValid (FileSize, HeaderOffset, sizeof (Header))) {
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }

    CopyMem (&Header, Image + HeaderOffset, sizeof (Header));
    if (RETURN_ERROR (SafeUintnAdd (HeaderOffset, sizeof (Header), &HeaderOffset))) {
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }

    if (Header.SizeOfRawData == 0) {
      continue;
    }

    if ((Header.PointerToRawData < Layout->SizeOfHeaders) ||
        !IsBufferRangeValid (DataEnd, Header.PointerToRawData, Header.SizeOfRawData))
    {
      DEBUG ((DEBUG_WARN, "AuthenticodeLib: section data lies outside the image data range.\n"));
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }

    Sections[SectionCount].Offset = Header.PointerToRawData;
    Sections[SectionCount].Size   = Header.SizeOfRawData;
    SectionCount++;
  }

  SortSections (Sections, SectionCount);

  for (Index = 0; Index < SectionCount; Index++) {
    if ((Sections[Index].Offset < *NextFileOffset) ||
        RETURN_ERROR (SafeUintnAdd (Sections[Index].Offset, Sections[Index].Size, &SectionEnd)))
    {
      DEBUG ((DEBUG_WARN, "AuthenticodeLib: overlapping or invalid section data.\n"));
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }

    Status = AppendImageRegion (Image, FileSize, *NextFileOffset, Sections[Index].Offset - *NextFileOffset, Out, OutPos);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Status = AppendImageRegion (Image, FileSize, Sections[Index].Offset, Sections[Index].Size, Out, OutPos);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    *NextFileOffset = SectionEnd;
  }

Done:
  FreePool (Sections);
  return Status;
}

/**
  Append file data after the final section and before any certificate table.

  @param[in]      Image           The PE/COFF image bytes.
  @param[in]      FileSize        Size of Image in bytes, and the capacity of Out.
  @param[in]      NextFileOffset  Start of the remaining file data.
  @param[in]      DataEnd         Exclusive end of the image data to include.
  @param[out]     Out             Destination buffer of FileSize bytes.
  @param[in,out]  OutPos          Current write position in Out; advanced by the bytes copied.

  @retval EFI_SUCCESS           The remaining data was appended.
  @retval EFI_VOLUME_CORRUPTED  The remaining data or destination range is invalid.
**/
STATIC
EFI_STATUS
AppendRemainingImageData (
  IN     CONST UINT8  *Image,
  IN     UINTN        FileSize,
  IN     UINTN        NextFileOffset,
  IN     UINTN        DataEnd,
  OUT    UINT8        *Out,
  IN OUT UINTN        *OutPos
  )
{
  if (NextFileOffset > DataEnd) {
    return EFI_VOLUME_CORRUPTED;
  }

  return AppendImageRegion (Image, FileSize, NextFileOffset, DataEnd - NextFileOffset, Out, OutPos);
}

/**
  Validate the certificate-table range and determine the end of image data.

  @param[in]   SecurityDirectory  Certificate Table directory; its VirtualAddress is a file offset.
  @param[in]   FileSize           Size of the image in bytes.
  @param[in]   SizeOfHeaders      Size of the image headers in bytes.
  @param[out]  DataEnd            On success, the certificate-table offset, or FileSize if absent.

  @retval EFI_SUCCESS           The end of image data was determined.
  @retval EFI_VOLUME_CORRUPTED  The certificate-table range or placement is invalid.
**/
STATIC
EFI_STATUS
GetImageDataEnd (
  IN  CONST EFI_IMAGE_DATA_DIRECTORY  *SecurityDirectory,
  IN  UINTN                           FileSize,
  IN  UINTN                           SizeOfHeaders,
  OUT UINTN                           *DataEnd
  )
{
  UINTN  Offset;
  UINTN  Size;

  Offset = SecurityDirectory->VirtualAddress;
  Size   = SecurityDirectory->Size;
  if (Size == 0) {
    *DataEnd = FileSize;
    return EFI_SUCCESS;
  }

  if ((Size < sizeof (WIN_CERTIFICATE)) ||
      ((Offset & 7) != 0) ||
      (Offset < SizeOfHeaders) ||
      !IsBufferRangeValid (FileSize, Offset, Size) ||
      (Size != FileSize - Offset))
  {
    DEBUG ((DEBUG_WARN, "AuthenticodeLib: invalid certificate table range.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  *DataEnd = Offset;
  return EFI_SUCCESS;
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
  UINTN                         NextFileOffset;
  UINTN                         DataEnd;
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

  Status = GetImageDataEnd (&ImageContext.SecurityDataDirectory, FileSize, Layout.SizeOfHeaders, &DataEnd);
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

  NextFileOffset = Layout.SizeOfHeaders;
  Status         = AppendSections (FileBuffer, FileSize, &Layout, DataEnd, Out, &OutPos, &NextFileOffset);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Append remaining data up to the certificate table, or the end of an unsigned image.
  //
  Status = AppendRemainingImageData (FileBuffer, FileSize, NextFileOffset, DataEnd, Out, &OutPos);
  if (EFI_ERROR (Status)) {
    goto Done;
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
  @param[out]  WinCertificates  On success, a pointer into FileBuffer to the certificate-table
                                bytes, or NULL when the image carries no certificate table. These
                                bytes may be unaligned; copy record headers before accessing fields.
  @param[out]  Length           On success, the certificate table length in bytes (0 when the image
                                carries none).

  @retval EFI_SUCCESS            WinCertificates / Length were populated.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_VOLUME_CORRUPTED   The image could not be parsed or is malformed.
**/
EFI_STATUS
EFIAPI
GetWinCertificates (
  IN  CONST VOID   *FileBuffer,
  IN  UINTN        FileSize,
  OUT CONST UINT8  **WinCertificates,
  OUT UINTN        *Length
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
  // Validate the table range and minimum header size before handing out a pointer.
  // The pointer can still be unaligned if FileBuffer itself is unaligned.
  //
  if ((Size < sizeof (WIN_CERTIFICATE)) ||
      ((VirtualAddress & 7) != 0) ||
      !IsBufferRangeValid (FileSize, VirtualAddress, Size))
  {
    DEBUG ((DEBUG_WARN, "AuthenticodeLib: invalid certificate table range.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  *WinCertificates = (CONST UINT8 *)FileBuffer + VirtualAddress;
  *Length          = Size;
  return EFI_SUCCESS;
}
