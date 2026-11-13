/** @file
  Build and iterate the Image Secure Boot Verification Result Table (SBRT).

  This library provides functionality for building an Image Secure Boot Verification
  Result Table (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE) and iterating over
  the contents of an existing table. The table is defined in
  <Guid/ImageSecureBootVerificationResultTable.h>.

  == Builder ==

    VOID  *Image;

    ImageVerificationResultCreateImage (Name, DevicePath, DevicePathSize, &Image);
    ImageVerificationResultAppendSignature (&Image, ...);   // once per evaluated signature

    // OldTable is an existing table, or NULL to create the table. Sizes live in
    // the table headers, so none are passed (the new size is NewTable->Length).
    ImageVerificationResultAppendImage (
      OldTable, Image, ImageStatus, DigestAlgorithm, &NewTable);

    FreePool (Image);
    FreePool (OldTable);

  == Iterator ==

    ImageVerificationResultIteratorInit (&Iter, Table);
    while ((Image = ImageVerificationResultIteratorNextImage (&Iter)) != NULL) {
      // ... consume Image ...
      while ((Sig = ImageVerificationResultIteratorNextSignature (&Iter)) != NULL) {
        // ... consume Sig (a signature of Image) ...
      }
    }

  Records land at unaligned offsets inside the packed table, so every access to a
  record's scalar fields goes through the unaligned Read/Write helpers, and blob
  fields are moved with CopyMem.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SafeIntLib.h>
#include <Library/ImageSecureBootVerificationResultTableLib.h>

#define SBRT_TABLE_HEADER_SIZE  (sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE))
#define SBRT_IMAGE_HEADER_SIZE  (sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT))
#define SBRT_SIG_HEADER_SIZE    (sizeof (EFI_SIGNATURE_VERIFICATION_RESULT))
#define SBRT_RECORD_ALIGNMENT   8U

/**
  Round an unpadded record size up to the table's 8-byte record alignment.

  Every record starts on an 8-byte boundary so consumers can read record fields
  directly, so each record's stored Length is padded up to SBRT_RECORD_ALIGNMENT.

  @param[in]   Unpadded  The record's header + payload size in bytes.
  @param[out]  Aligned   On success, Unpadded rounded up to SBRT_RECORD_ALIGNMENT.

  @retval TRUE   The aligned size fits in a UINT32 and was written to Aligned.
  @retval FALSE  The aligned size would exceed MAX_UINT32; Aligned is untouched.
**/
STATIC
BOOLEAN
AlignRecordSize (
  IN  UINTN   Unpadded,
  OUT UINT32  *Aligned
  )
{
  UINTN  Pad;
  UINTN  Padded;

  Pad = (SBRT_RECORD_ALIGNMENT - (Unpadded & (SBRT_RECORD_ALIGNMENT - 1))) & (SBRT_RECORD_ALIGNMENT - 1);

  if (EFI_ERROR (SafeUintnAdd (Unpadded, Pad, &Padded)) ||
      EFI_ERROR (SafeUintnToUint32 (Padded, Aligned)))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Allocate an image record from its identity (name and device path).

  Returns a EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT allocation
  with zero signatures; the overall status is set by the caller.

  Caller is responsible for freeing Image with FreePool().

  @param[in]   Name            Optional NUL-terminated CHAR16 image name; NULL for none.
  @param[in]   DevicePath      The image's device path bytes. Required.
  @param[in]   DevicePathSize  Size of DevicePath in bytes, including its end-of-path node.
  @param[out]  Image           On success, the newly allocated image record. Free with FreePool().

  @retval EFI_SUCCESS            The image record was allocated.
  @retval EFI_INVALID_PARAMETER  Image or DevicePath is NULL, or DevicePathSize is 0.
  @retval EFI_BAD_BUFFER_SIZE    The resulting record would exceed a UINT32 size field.
  @retval EFI_OUT_OF_RESOURCES   The record could not be allocated.
**/
EFI_STATUS
EFIAPI
ImageVerificationResultCreateImage (
  IN  CONST CHAR16  *Name OPTIONAL,
  IN  CONST VOID    *DevicePath,
  IN  UINTN         DevicePathSize,
  OUT VOID          **Image
  )
{
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT  *Record;
  UINTN                                      NameSize;
  UINTN                                      RecordSizeN;
  UINT32                                     RecordSize;
  UINT8                                      *Cursor;

  if ((Image == NULL) || (DevicePath == NULL) || (DevicePathSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  NameSize = (Name != NULL) ? StrSize (Name) : 0;

  //
  // RecordSize = ALIGN8 (header + Name + DevicePath), guarded to fit the UINT32
  // Length.
  //
  if (EFI_ERROR (SafeUintnAdd (SBRT_IMAGE_HEADER_SIZE, NameSize, &RecordSizeN)) ||
      EFI_ERROR (SafeUintnAdd (RecordSizeN, DevicePathSize, &RecordSizeN)) ||
      !AlignRecordSize (RecordSizeN, &RecordSize))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Zero-fill so the reserved field and every padding byte are deterministic.
  //
  Record = AllocateZeroPool (RecordSize);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Record->Length           = RecordSize;
  Record->NameLength       = (UINT32)NameSize;
  Record->DevicePathLength = (UINT32)DevicePathSize;

  Cursor = (UINT8 *)Record + SBRT_IMAGE_HEADER_SIZE;
  if (NameSize != 0) {
    CopyMem (Cursor, Name, NameSize);
    Cursor += NameSize;
  }

  CopyMem (Cursor, DevicePath, DevicePathSize);

  *Image = Record;
  return EFI_SUCCESS;
}

/**
  Grow an image record by one evaluated signature.

  Reallocates *Image a signature larger and appends the signature at its tail. On
  failure *Image is left unchanged (and still valid). Signatures are appended in
  evaluation order.

  @param[in,out]  Image                The image record to extend (from CreateImage). Updated in place.
  @param[in]      SignatureIndex       Index of the WIN_CERTIFICATE within the image.
  @param[in]      Status               EFI_SIGNATURE_VERIFICATION_* outcome for the signature.
  @param[in]      ThumbprintAlgorithm  Optional digest algorithm of Thumbprint; NULL => zero GUID.
  @param[in]      Thumbprint           Optional decisive-cert TBS-cert hash; NULL => none.
  @param[in]      ThumbprintSize       Bytes of Thumbprint; must be 0 iff Thumbprint is NULL.

  @retval EFI_SUCCESS            The signature was appended.
  @retval EFI_INVALID_PARAMETER  Image or *Image is NULL, or the Thumbprint / ThumbprintSize /
                                 ThumbprintAlgorithm arguments are inconsistent.
  @retval EFI_BAD_BUFFER_SIZE    The resulting record would exceed a UINT32 size field.
  @retval EFI_OUT_OF_RESOURCES   The record could not be grown.
**/
EFI_STATUS
EFIAPI
ImageVerificationResultAppendSignature (
  IN OUT VOID            **Image,
  IN     UINT32          SignatureIndex,
  IN     UINT32          Status,
  IN     CONST EFI_GUID  *ThumbprintAlgorithm OPTIONAL,
  IN     CONST VOID      *Thumbprint OPTIONAL,
  IN     UINTN           ThumbprintSize
  )
{
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT  *Record;
  EFI_SIGNATURE_VERIFICATION_RESULT          *Signature;
  UINTN                                      SigRecordSizeN;
  UINT32                                     OldLength;
  UINT32                                     SigRecordSize;
  UINT32                                     NewLength;
  UINT8                                      *NewRecord;

  if ((Image == NULL) || (*Image == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Thumbprint is null, ThumbprintSize must be 0; if Thumbprint is non-null,
  // ThumbprintSize must be non-zero.
  //
  if ((Thumbprint == NULL) != (ThumbprintSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Record    = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)*Image;
  OldLength = Record->Length;

  //
  // SigRecordSize = ALIGN8 (header + Thumbprint).
  if (EFI_ERROR (SafeUintnAdd (SBRT_SIG_HEADER_SIZE, ThumbprintSize, &SigRecordSizeN)) ||
      !AlignRecordSize (SigRecordSizeN, &SigRecordSize) ||
      EFI_ERROR (SafeUint32Add (OldLength, SigRecordSize, &NewLength)))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  NewRecord = ReallocatePool (OldLength, NewLength, *Image);
  if (NewRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Image = NewRecord;
  Record = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NewRecord;

  //
  // ReallocatePool leaves the grown tail uninitialized; zero the new record.
  Signature = (EFI_SIGNATURE_VERIFICATION_RESULT *)(NewRecord + OldLength);
  ZeroMem (Signature, SigRecordSize);

  Signature->Length         = SigRecordSize;
  Signature->SignatureIndex = SignatureIndex;
  Signature->Status         = Status;
  Signature->ThumbprintSize = (UINT32)ThumbprintSize;
  if (ThumbprintAlgorithm != NULL) {
    CopyGuid (&Signature->ThumbprintAlgorithm, ThumbprintAlgorithm);
  }

  if (ThumbprintSize != 0) {
    CopyMem ((UINT8 *)Signature + SBRT_SIG_HEADER_SIZE, Thumbprint, ThumbprintSize);
  }

  Record->Length              = NewLength;
  Record->NumberOfSignatures += 1;

  return EFI_SUCCESS;
}

/**
  Append an image record to a copy of an existing table or creates a new table if OldTable is NULL.

  Caller is responsible for freeing NewTable with FreePool().

  @param[in]   OldTable             Optional currently installed table to extend; NULL for a fresh table.
  @param[in]   Image                The image record to append (from CreateImage / AppendSignature).
  @param[in]   ImageStatus          EFI_IMAGE_VERIFICATION_STATUS_* outcome for the image.
  @param[in]   ImageDigestAlgorithm Optional matching digest algorithm; NULL => zero GUID.
  @param[out]  NewTable             On success, the newly allocated table. Free with FreePool().

  @retval EFI_SUCCESS            The image was appended and NewTable was produced.
  @retval EFI_INVALID_PARAMETER  Image or NewTable is NULL, or OldTable is non-NULL but not a
                                 valid table.
  @retval EFI_BAD_BUFFER_SIZE    The resulting table would exceed the UINT32 Length / image count.
  @retval EFI_OUT_OF_RESOURCES   The new table could not be allocated.
**/
EFI_STATUS
EFIAPI
ImageVerificationResultAppendImage (
  IN  CONST VOID                                       *OldTable OPTIONAL,
  IN  CONST VOID                                       *Image,
  IN  UINT32                                           ImageStatus,
  IN  CONST EFI_GUID                                   *ImageDigestAlgorithm OPTIONAL,
  OUT EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  **NewTable
  )
{
  CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *Old;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE        *Result;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT              *NewImage;
  UINT32                                                 ImageLength;
  UINT32                                                 OldLength;
  UINT32                                                 OldImages;
  UINT32                                                 NewLength;
  UINT32                                                 NewImages;

  if ((Image == NULL) || (NewTable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Reuse the old table's header if present, otherwise start a new table.
  if (OldTable == NULL) {
    Old       = NULL;
    OldLength = (UINT32)SBRT_TABLE_HEADER_SIZE;
    OldImages = 0;
  } else {
    Old = (CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE *)OldTable;
    if ((Old->Signature != EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE) ||
        (Old->Version != EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION) ||
        (Old->Length < SBRT_TABLE_HEADER_SIZE))
    {
      return EFI_INVALID_PARAMETER;
    }

    OldLength = Old->Length;
    OldImages = Old->NumberOfImages;
  }

  ImageLength = ((CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Image)->Length;

  if (EFI_ERROR (SafeUint32Add (OldLength, ImageLength, &NewLength)) ||
      EFI_ERROR (SafeUint32Add (OldImages, 1, &NewImages)))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  Result = AllocatePool (NewLength);
  if (Result == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Old != NULL) {
    CopyMem (Result, Old, OldLength);
  } else {
    Result->Signature = EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE;
    Result->Version   = EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION;
  }

  NewImage = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)((UINT8 *)Result + OldLength);
  CopyMem (NewImage, Image, ImageLength);
  NewImage->ImageStatus = ImageStatus;
  if (ImageDigestAlgorithm != NULL) {
    CopyGuid (&NewImage->ImageDigestAlgorithm, ImageDigestAlgorithm);
  }

  Result->Length         = NewLength;
  Result->NumberOfImages = NewImages;

  *NewTable = Result;

  return EFI_SUCCESS;
}

/**
  Validate the signature records that trail one image record.

  @param[in]  Region       Start of the signature region.
  @param[in]  RegionSize   Size of the signature region in bytes.
  @param[in]  ExpectedCount  NumberOfSignatures declared by the owning image.

  @retval TRUE   The region contains exactly ExpectedCount well-formed records.
  @retval FALSE  The region is malformed or the record count disagrees.
**/
STATIC
BOOLEAN
ValidateSignatureRegion (
  IN CONST UINT8  *Region,
  IN UINTN        RegionSize,
  IN UINT32       ExpectedCount
  )
{
  CONST EFI_SIGNATURE_VERIFICATION_RESULT  *Signature;
  UINTN                                    Remaining;
  UINT32                                   Count;
  UINT32                                   RecordLength;

  Remaining = RegionSize;
  Count     = 0;

  while (Remaining > 0) {
    if (Remaining < SBRT_SIG_HEADER_SIZE) {
      return FALSE;
    }

    Signature    = (CONST EFI_SIGNATURE_VERIFICATION_RESULT *)Region;
    RecordLength = Signature->Length;

    if ((RecordLength < SBRT_SIG_HEADER_SIZE) ||
        (RecordLength > Remaining) ||
        ((RecordLength & (SBRT_RECORD_ALIGNMENT - 1)) != 0) ||
        (Signature->ThumbprintSize > RecordLength - SBRT_SIG_HEADER_SIZE))
    {
      return FALSE;
    }

    if (Count == MAX_UINT32) {
      return FALSE;
    }

    Region    += RecordLength;
    Remaining -= RecordLength;
    Count     += 1;
  }

  return (BOOLEAN)(Count == ExpectedCount);
}

/**
  Validate a single image record and report its total size.

  @param[in]   Record        Start of the candidate image record.
  @param[in]   Remaining     Bytes available from Record to the end of the images region.
  @param[out]  RecordLength  On success, the record's total size in bytes.

  @retval TRUE   The record is well-formed; RecordLength was written.
  @retval FALSE  The record is malformed.
**/
STATIC
BOOLEAN
ValidateImageRecord (
  IN  CONST UINT8  *Record,
  IN  UINTN        Remaining,
  OUT UINT32       *RecordLength
  )
{
  CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT  *Image;
  UINT32                                           Length;
  UINT32                                           NameLength;
  UINT32                                           DevicePathLength;
  UINTN                                            PayloadOffset;

  if (Remaining < SBRT_IMAGE_HEADER_SIZE) {
    return FALSE;
  }

  Image            = (CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Record;
  Length           = Image->Length;
  NameLength       = Image->NameLength;
  DevicePathLength = Image->DevicePathLength;

  if ((Length < SBRT_IMAGE_HEADER_SIZE) ||
      (Length > Remaining) ||
      ((Length & (SBRT_RECORD_ALIGNMENT - 1)) != 0) ||
      (NameLength > Length - SBRT_IMAGE_HEADER_SIZE) ||
      (DevicePathLength > Length - SBRT_IMAGE_HEADER_SIZE - NameLength))
  {
    return FALSE;
  }

  //
  // Signatures[] begins at the next 8-byte boundary after the name/device path.
  //
  PayloadOffset = ALIGN_VALUE (SBRT_IMAGE_HEADER_SIZE + (UINTN)NameLength + (UINTN)DevicePathLength, SBRT_RECORD_ALIGNMENT);
  if (PayloadOffset > Length) {
    return FALSE;
  }

  if (!ValidateSignatureRegion (Record + PayloadOffset, Length - PayloadOffset, Image->NumberOfSignatures)) {
    return FALSE;
  }

  *RecordLength = Length;
  return TRUE;
}

/**
  Initialize an iterator over an SBRT and validate its structure.

  The table is validated here, so that the different iteration routines can be
  infallible over the valid prefix. During initialization, the structure is
  walked, and if any record fails to parse, the iteration range is truncated to
  the last valid record. The TRUE / FALSE return indicates whether the entire
  table was valid or not.

  @param[out]  Iterator   Iterator state to initialize.
  @param[in]   Table      The table buffer to walk, or NULL for an empty iterator.

  @retval TRUE   The iterator covers every image and signature in the table.
  @retval FALSE  The range was truncated (a malformed record was dropped) or the inputs were
                 unusable (bad signature/version, or an inconsistent length). The exposed
                 iterator is still safe to use and covers the valid prefix.
**/
BOOLEAN
EFIAPI
ImageVerificationResultIteratorInit (
  OUT IMAGE_VERIFICATION_RESULT_ITERATOR  *Iterator,
  IN  CONST VOID                          *Table
  )
{
  CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *Header;
  CONST UINT8                                            *Cursor;
  UINTN                                                  Remaining;
  UINTN                                                  ImageCount;
  UINT32                                                 RecordLength;

  if (Iterator == NULL) {
    return FALSE;
  }

  ZeroMem (Iterator, sizeof (*Iterator));

  //
  // An absent table is an empty (not truncated) iteration.
  //
  if (Table == NULL) {
    return TRUE;
  }

  Header = (CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE *)Table;

  if ((Header->Signature != EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE) ||
      (Header->Version != EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION) ||
      (Header->Length < SBRT_TABLE_HEADER_SIZE))
  {
    return FALSE;
  }

  Cursor     = (CONST UINT8 *)Table + SBRT_TABLE_HEADER_SIZE;
  Remaining  = Header->Length - SBRT_TABLE_HEADER_SIZE;
  ImageCount = 0;

  //
  // Walk the image records, stopping at the first one that fails to parse. The
  // valid prefix is everything that tiled cleanly before it.
  //
  while (Remaining > 0) {
    if (!ValidateImageRecord (Cursor, Remaining, &RecordLength)) {
      break;
    }

    Cursor     += RecordLength;
    Remaining  -= RecordLength;
    ImageCount += 1;
  }

  Iterator->NextImageRecord = (CONST UINT8 *)Table + SBRT_TABLE_HEADER_SIZE;
  Iterator->ImagesRemaining = ImageCount;

  return (BOOLEAN)((Remaining == 0) && (ImageCount == Header->NumberOfImages));
}

/**
  Return the next image result and reset the inner signature cursor to it.

  @param[in,out]  Iterator  An initialized iterator.

  @retval non-NULL  Pointer to the next EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT.
  @retval NULL      Iteration over the images is complete.
**/
CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *
EFIAPI
ImageVerificationResultIteratorNextImage (
  IN OUT IMAGE_VERIFICATION_RESULT_ITERATOR  *Iterator
  )
{
  CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT  *Image;
  CONST UINT8                                      *Record;

  if ((Iterator == NULL) || (Iterator->ImagesRemaining == 0)) {
    return NULL;
  }

  Record = Iterator->NextImageRecord;
  Image  = (CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Record;

  // Reset the inner cursor to this image's signature region, which begins at the next 8-byte boundary after the name and device path.
  Iterator->NextSignatureRecord = Record + ALIGN_VALUE (SBRT_IMAGE_HEADER_SIZE + (UINTN)Image->NameLength + (UINTN)Image->DevicePathLength, SBRT_RECORD_ALIGNMENT);
  Iterator->SignaturesRemaining = Image->NumberOfSignatures;

  Iterator->NextImageRecord  = Record + Image->Length;
  Iterator->ImagesRemaining -= 1;

  return Image;
}

/**
  Return the next signature result for the current image.

  @param[in,out]  Iterator  An initialized iterator.

  @retval non-NULL  Pointer to the next EFI_SIGNATURE_VERIFICATION_RESULT of the current image.
  @retval NULL      The current image has no more signatures (or no image is selected).
**/
CONST EFI_SIGNATURE_VERIFICATION_RESULT *
EFIAPI
ImageVerificationResultIteratorNextSignature (
  IN OUT IMAGE_VERIFICATION_RESULT_ITERATOR  *Iterator
  )
{
  CONST EFI_SIGNATURE_VERIFICATION_RESULT  *Signature;

  if ((Iterator == NULL) || (Iterator->SignaturesRemaining == 0)) {
    return NULL;
  }

  Signature = (CONST EFI_SIGNATURE_VERIFICATION_RESULT *)Iterator->NextSignatureRecord;

  Iterator->NextSignatureRecord  = (CONST UINT8 *)Signature + Signature->Length;
  Iterator->SignaturesRemaining -= 1;

  return Signature;
}
