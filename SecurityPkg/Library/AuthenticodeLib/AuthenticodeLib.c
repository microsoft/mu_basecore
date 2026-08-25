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
  )
{
  RETURN_STATUS                   PeCoffStatus;
  PE_COFF_LOADER_IMAGE_CONTEXT    ImageContext;
  AUTHENTICODE_IMAGE_HANDLE       Handle;
  CONST UINT8                     *Image;
  CONST EFI_IMAGE_NT_HEADERS32    *Nt32;
  CONST EFI_IMAGE_NT_HEADERS64    *Nt64;
  CONST EFI_IMAGE_SECTION_HEADER  *SectionTable;
  EFI_IMAGE_SECTION_HEADER        *Sorted;
  UINT8                           *Out;
  UINT32                          PeOffset;
  UINT16                          Magic;
  UINT16                          NumberOfSections;
  UINT16                          SizeOfOptionalHeader;
  UINT32                          NumberOfRvaAndSizes;
  UINT32                          SizeOfHeaders;
  UINT32                          CertSize;
  UINTN                           ChecksumOffset;
  UINTN                           SecDirOffset;
  UINTN                           SectionTableOffset;
  UINTN                           OutPos;
  UINTN                           SumOfBytesHashed;
  UINTN                           Remaining;
  UINTN                           Index;
  UINTN                           Pos;
  EFI_STATUS                      Status;

  if ((FileBuffer == NULL) || (AuthImage == NULL) || (AuthImageSize == NULL) || (FileSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  *AuthImage     = NULL;
  *AuthImageSize = 0;
  Image          = (CONST UINT8 *)FileBuffer;
  Nt64           = NULL;
  Sorted         = NULL;
  Out            = NULL;

  //
  // Delegate DOS/PE header validation to PeCoffLib. It records the PE header offset, the total
  // header size, and the bounds-checked Certificate Table data-directory entry, letting this
  // routine skip the hand-rolled DOS-stub, PE-signature, and data-directory parsing.
  //
  ZeroMem (&ImageContext, sizeof (ImageContext));
  Handle.Base            = Image;
  Handle.Size            = FileSize;
  ImageContext.Handle    = &Handle;
  ImageContext.ImageRead = AuthenticodeImageRead;

  PeCoffStatus = PeCoffLoaderGetImageInfo (&ImageContext);
  if (RETURN_ERROR (PeCoffStatus)) {
    DEBUG ((DEBUG_INFO, "AuthenticodeLib: invalid PE/COFF image (0x%lx).\n", (UINT64)PeCoffStatus));
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // The Authenticode layout is defined for PE images; a TE image has no equivalent form.
  //
  if (ImageContext.IsTeImage) {
    return EFI_UNSUPPORTED;
  }

  PeOffset      = ImageContext.PeCoffHeaderOffset;
  SizeOfHeaders = (UINT32)ImageContext.SizeOfHeaders;
  CertSize      = ImageContext.SecurityDataDirectory.Size;

  //
  // PeCoffLib validated the headers; the manual reads below only locate the CheckSum field, the
  // Certificate Table entry, and the section table. Confirm each read fits the image first.
  //
  if ((PeOffset > FileSize) || (FileSize - PeOffset < sizeof (EFI_IMAGE_NT_HEADERS32))) {
    return EFI_VOLUME_CORRUPTED;
  }

  Nt32  = (CONST EFI_IMAGE_NT_HEADERS32 *)(Image + PeOffset);
  Magic = Nt32->OptionalHeader.Magic;

  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    NumberOfRvaAndSizes = Nt32->OptionalHeader.NumberOfRvaAndSizes;
    ChecksumOffset      = (CONST UINT8 *)&Nt32->OptionalHeader.CheckSum - Image;
    SecDirOffset        = (CONST UINT8 *)&Nt32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY] - Image;
  } else if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    if (FileSize - PeOffset < sizeof (EFI_IMAGE_NT_HEADERS64)) {
      return EFI_VOLUME_CORRUPTED;
    }

    Nt64                = (CONST EFI_IMAGE_NT_HEADERS64 *)(Image + PeOffset);
    NumberOfRvaAndSizes = Nt64->OptionalHeader.NumberOfRvaAndSizes;
    ChecksumOffset      = (CONST UINT8 *)&Nt64->OptionalHeader.CheckSum - Image;
    SecDirOffset        = (CONST UINT8 *)&Nt64->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY] - Image;
  } else {
    return EFI_VOLUME_CORRUPTED;
  }

  NumberOfSections     = Nt32->FileHeader.NumberOfSections;
  SizeOfOptionalHeader = Nt32->FileHeader.SizeOfOptionalHeader;
  SectionTableOffset   = (UINTN)PeOffset + sizeof (UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + SizeOfOptionalHeader;

  if ((SectionTableOffset > FileSize) ||
      ((FileSize - SectionTableOffset) / sizeof (EFI_IMAGE_SECTION_HEADER) < NumberOfSections))
  {
    return EFI_VOLUME_CORRUPTED;
  }

  SectionTable = (CONST EFI_IMAGE_SECTION_HEADER *)(Image + SectionTableOffset);

  //
  // The message only omits bytes (the checksum, the security directory entry, and the attribute
  // certificate table), so it never exceeds the image size.
  //
  Out = AllocatePool (FileSize);
  if (Out == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  OutPos = 0;

  //
  // Header up to the CheckSum field (the 4-byte CheckSum is then skipped).
  //
  Status = AppendImageRegion (Image, FileSize, 0, ChecksumOffset, Out, &OutPos);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
    //
    // No Certificate Table directory entry: append the rest of the headers.
    //
    if (SizeOfHeaders < ChecksumOffset + sizeof (UINT32)) {
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }

    Status = AppendImageRegion (Image, FileSize, ChecksumOffset + sizeof (UINT32), SizeOfHeaders - (ChecksumOffset + sizeof (UINT32)), Out, &OutPos);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  } else {
    //
    // Append from after the CheckSum to the Certificate Table directory entry, skip that 8-byte
    // entry, then append the remaining headers.
    //
    if (SecDirOffset < ChecksumOffset + sizeof (UINT32)) {
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }

    Status = AppendImageRegion (Image, FileSize, ChecksumOffset + sizeof (UINT32), SecDirOffset - (ChecksumOffset + sizeof (UINT32)), Out, &OutPos);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    if (SizeOfHeaders < SecDirOffset + sizeof (EFI_IMAGE_DATA_DIRECTORY)) {
      Status = EFI_VOLUME_CORRUPTED;
      goto Done;
    }

    Status = AppendImageRegion (Image, FileSize, SecDirOffset + sizeof (EFI_IMAGE_DATA_DIRECTORY), SizeOfHeaders - (SecDirOffset + sizeof (EFI_IMAGE_DATA_DIRECTORY)), Out, &OutPos);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  SumOfBytesHashed = SizeOfHeaders;

  //
  // Append each section's raw data in ascending PointerToRawData order.
  //
  if (NumberOfSections > 0) {
    Sorted = AllocateZeroPool ((UINTN)NumberOfSections * sizeof (EFI_IMAGE_SECTION_HEADER));
    if (Sorted == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    for (Index = 0; Index < NumberOfSections; Index++) {
      Pos = Index;
      while ((Pos > 0) && (SectionTable[Index].PointerToRawData < Sorted[Pos - 1].PointerToRawData)) {
        CopyMem (&Sorted[Pos], &Sorted[Pos - 1], sizeof (EFI_IMAGE_SECTION_HEADER));
        Pos--;
      }

      CopyMem (&Sorted[Pos], &SectionTable[Index], sizeof (EFI_IMAGE_SECTION_HEADER));
    }

    for (Index = 0; Index < NumberOfSections; Index++) {
      if (Sorted[Index].SizeOfRawData == 0) {
        continue;
      }

      Status = AppendImageRegion (Image, FileSize, Sorted[Index].PointerToRawData, Sorted[Index].SizeOfRawData, Out, &OutPos);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      SumOfBytesHashed += Sorted[Index].SizeOfRawData;
    }
  }

  //
  // Trailing data after the sections, excluding the attribute-certificate table (CertSize bytes).
  //
  if (FileSize > SumOfBytesHashed) {
    Remaining = FileSize - SumOfBytesHashed;
    if (Remaining > CertSize) {
      Status = AppendImageRegion (Image, FileSize, SumOfBytesHashed, Remaining - CertSize, Out, &OutPos);
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
  if (Sorted != NULL) {
    FreePool (Sorted);
  }

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
