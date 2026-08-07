/** @file
  Build and iterate the Image Secure Boot Verification Result Table (IVRT).

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
#include <Library/ImageSecureBootVerificationResultTableLib.h>

//
// Cached record header sizes. Every record is walked by its self-describing
// Length, so these fixed prefixes bound where the variable payloads begin.
//
#define IVRT_TABLE_HEADER_SIZE  (sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE))
#define IVRT_IMAGE_HEADER_SIZE  (sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT))
#define IVRT_SIG_HEADER_SIZE    (sizeof (EFI_SIGNATURE_VERIFICATION_RESULT))

// ***************************************************************************
// Shared helpers
// ***************************************************************************

/**
  Add two UINTN values, reporting UINT32 overflow.

  The table and every record size field is a UINT32, so growth arithmetic is
  validated against MAX_UINT32 using the addition form to avoid wraparound.

  @param[in]   Augend  First value.
  @param[in]   Addend  Second value.
  @param[out]  Sum     On success, Augend + Addend.

  @retval TRUE   The sum fits in a UINT32 and was written to Sum.
  @retval FALSE  The sum would exceed MAX_UINT32; Sum is untouched.
**/
STATIC
BOOLEAN
SafeAddU32 (
  IN  UINTN   Augend,
  IN  UINTN   Addend,
  OUT UINT32  *Sum
  )
{
  if ((Augend > MAX_UINT32) || (Addend > MAX_UINT32 - Augend)) {
    return FALSE;
  }

  *Sum = (UINT32)(Augend + Addend);
  return TRUE;
}

/**
  Store an EFI_GUID into a (possibly unaligned) record field, defaulting to the
  zero GUID when no source GUID was supplied.

  @param[out]  Destination  Field to populate.
  @param[in]   Source       Optional GUID to copy; NULL stores the zero GUID.
**/
STATIC
VOID
SetOptionalGuid (
  OUT VOID            *Destination,
  IN  CONST EFI_GUID  *Source OPTIONAL
  )
{
  if (Source != NULL) {
    CopyMem (Destination, Source, sizeof (EFI_GUID));
  } else {
    ZeroMem (Destination, sizeof (EFI_GUID));
  }
}

// ***************************************************************************
// Builder (write side)
// ***************************************************************************

/**
  Allocate an image record from its identity (name and device path).

  Returns a self-describing EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT allocation
  with zero signatures; the overall status is supplied later, to
  ImageVerificationResultAppendImage().

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
  UINT32                                     RecordSize;
  UINT8                                      *Cursor;

  if ((Image == NULL) || (DevicePath == NULL) || (DevicePathSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  NameSize = (Name != NULL) ? StrSize (Name) : 0;

  //
  // RecordSize = header + Name + DevicePath, guarded to fit the UINT32 Length.
  //
  if (!SafeAddU32 (IVRT_IMAGE_HEADER_SIZE, NameSize, &RecordSize) ||
      !SafeAddU32 (RecordSize, DevicePathSize, &RecordSize))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  Record = AllocatePool (RecordSize);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // The record is allocated exactly (Length == allocation size). It is 8-aligned,
  // so the fixed header is written through the struct directly; ImageStatus and
  // ImageDigestAlgorithm stay zero until AppendImage stamps them.
  //
  Record->Length             = RecordSize;
  Record->ImageStatus        = 0;
  Record->NumberOfSignatures = 0;
  Record->NameLength         = (UINT32)NameSize;
  Record->DevicePathLength   = (UINT32)DevicePathSize;
  ZeroMem (&Record->ImageDigestAlgorithm, sizeof (EFI_GUID));

  Cursor = (UINT8 *)Record + IVRT_IMAGE_HEADER_SIZE;
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
  @param[in]      SignatureIndex       0-based ordinal of the WIN_CERTIFICATE within the image.
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
  UINT32                                     OldLength;
  UINT32                                     SigRecordSize;
  UINT32                                     NewLength;
  UINT8                                      *NewRecord;
  UINT8                                      *Signature;

  if ((Image == NULL) || (*Image == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Thumbprint and its size must agree: a present blob has a non-zero size, an
  // absent blob has a zero size.
  //
  if ((Thumbprint == NULL) != (ThumbprintSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The record is allocated exactly, so its Length is also its current size.
  // SigRecordSize = signature header + thumbprint; the grown Length must fit its
  // UINT32 field.
  //
  Record    = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)*Image;
  OldLength = Record->Length;

  if (!SafeAddU32 (IVRT_SIG_HEADER_SIZE, ThumbprintSize, &SigRecordSize) ||
      !SafeAddU32 (OldLength, SigRecordSize, &NewLength))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Reallocate exactly one signature larger. On failure ReallocatePool leaves the
  // old allocation intact, so *Image stays valid.
  //
  NewRecord = ReallocatePool (OldLength, NewLength, *Image);
  if (NewRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Image = NewRecord;
  Record = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NewRecord;

  //
  // The signature lands at the (unaligned) tail, so its scalar fields go through
  // the unaligned writers.
  //
  Signature = NewRecord + OldLength;

  WriteUnaligned32 ((UINT32 *)(Signature + OFFSET_OF (EFI_SIGNATURE_VERIFICATION_RESULT, Length)), SigRecordSize);
  WriteUnaligned32 ((UINT32 *)(Signature + OFFSET_OF (EFI_SIGNATURE_VERIFICATION_RESULT, SignatureIndex)), SignatureIndex);
  WriteUnaligned32 ((UINT32 *)(Signature + OFFSET_OF (EFI_SIGNATURE_VERIFICATION_RESULT, Status)), Status);
  SetOptionalGuid (Signature + OFFSET_OF (EFI_SIGNATURE_VERIFICATION_RESULT, ThumbprintAlgorithm), ThumbprintAlgorithm);

  if (ThumbprintSize != 0) {
    CopyMem (Signature + IVRT_SIG_HEADER_SIZE, Thumbprint, ThumbprintSize);
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
  UINT32                                                 ImageLength;
  UINT32                                                 OldLength;
  UINT32                                                 OldImages;
  UINT32                                                 NewLength;
  UINT32                                                 NewImages;
  UINT8                                                  *ImageRecord;

  if ((Image == NULL) || (NewTable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Establish the base table: an empty header when there is no old table, or the
  // validated old table otherwise. The old table is trusted (previously produced
  // by this library), so its self-describing Length bounds the copy.
  //
  if (OldTable == NULL) {
    Old       = NULL;
    OldLength = (UINT32)IVRT_TABLE_HEADER_SIZE;
    OldImages = 0;
  } else {
    Old = (CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE *)OldTable;
    if ((Old->Signature != EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE) ||
        (Old->Version != EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION) ||
        (Old->Length < IVRT_TABLE_HEADER_SIZE))
    {
      return EFI_INVALID_PARAMETER;
    }

    OldLength = Old->Length;
    OldImages = Old->NumberOfImages;
  }

  ImageLength = ((CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Image)->Length;

  if (!SafeAddU32 (OldLength, ImageLength, &NewLength) ||
      !SafeAddU32 (OldImages, 1, &NewImages))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  Result = AllocatePool (NewLength);
  if (Result == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the base (old images, or a fresh header), append the image record, then
  // stamp the now-known status and digest algorithm at its (unaligned) offset.
  //
  if (Old != NULL) {
    CopyMem (Result, Old, OldLength);
  } else {
    Result->Signature = EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE;
    Result->Version   = EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION;
  }

  ImageRecord = (UINT8 *)Result + OldLength;
  CopyMem (ImageRecord, Image, ImageLength);
  WriteUnaligned32 ((UINT32 *)(ImageRecord + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, ImageStatus)), ImageStatus);
  SetOptionalGuid (ImageRecord + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, ImageDigestAlgorithm), ImageDigestAlgorithm);

  Result->Length         = NewLength;
  Result->NumberOfImages = NewImages;

  *NewTable = Result;

  return EFI_SUCCESS;
}

// ***************************************************************************
// Iterator (read side)
// ***************************************************************************

/**
  Validate the signature records that trail one image record.

  Confirms the region [RegionSize] holds exactly ExpectedCount self-sized
  EFI_SIGNATURE_VERIFICATION_RESULT records that tile the region with no leftover
  bytes.

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
  UINTN   Remaining;
  UINT32  Count;
  UINT32  RecordLength;

  Remaining = RegionSize;
  Count     = 0;

  while (Remaining > 0) {
    if (Remaining < IVRT_SIG_HEADER_SIZE) {
      return FALSE;
    }

    RecordLength = ReadUnaligned32 ((CONST UINT32 *)(Region + OFFSET_OF (EFI_SIGNATURE_VERIFICATION_RESULT, Length)));
    if ((RecordLength < IVRT_SIG_HEADER_SIZE) || (RecordLength > Remaining)) {
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

  Checks that the fixed header fits, that Name + DevicePath + signatures fit
  within the record's declared Length, and that the trailing signature records
  are well-formed and match the declared NumberOfSignatures.

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
  UINT32  Length;
  UINT32  NumberOfSignatures;
  UINT32  NameLength;
  UINT32  DevicePathLength;
  UINTN   PayloadOffset;

  if (Remaining < IVRT_IMAGE_HEADER_SIZE) {
    return FALSE;
  }

  Length             = ReadUnaligned32 ((CONST UINT32 *)(Record + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, Length)));
  NumberOfSignatures = ReadUnaligned32 ((CONST UINT32 *)(Record + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, NumberOfSignatures)));
  NameLength         = ReadUnaligned32 ((CONST UINT32 *)(Record + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, NameLength)));
  DevicePathLength   = ReadUnaligned32 ((CONST UINT32 *)(Record + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, DevicePathLength)));

  if ((Length < IVRT_IMAGE_HEADER_SIZE) || (Length > Remaining)) {
    return FALSE;
  }

  //
  // header + Name + DevicePath must fit within Length (computed in UINTN, which
  // cannot overflow here because each addend is a UINT32).
  //
  PayloadOffset = IVRT_IMAGE_HEADER_SIZE + (UINTN)NameLength + (UINTN)DevicePathLength;
  if (PayloadOffset > Length) {
    return FALSE;
  }

  if (!ValidateSignatureRegion (Record + PayloadOffset, Length - PayloadOffset, NumberOfSignatures)) {
    return FALSE;
  }

  *RecordLength = Length;
  return TRUE;
}

/**
  Initialize an iterator over an IVRT and validate its structure.

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

  //
  // The table is self-describing: Header->Length bounds the walk (as any
  // configuration-table consumer must trust, since the table carries no external
  // size). Reject a foreign or mis-versioned buffer, or a Length too small to
  // hold the header.
  //
  if ((Header->Signature != EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE) ||
      (Header->Version != EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION) ||
      (Header->Length < IVRT_TABLE_HEADER_SIZE))
  {
    return FALSE;
  }

  Cursor     = (CONST UINT8 *)Table + IVRT_TABLE_HEADER_SIZE;
  Remaining  = Header->Length - IVRT_TABLE_HEADER_SIZE;
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

  Iterator->NextImageRecord = (CONST UINT8 *)Table + IVRT_TABLE_HEADER_SIZE;
  Iterator->ImagesRemaining = ImageCount;

  //
  // Clean only when the whole images region tiled AND the tiled count matches
  // the declared image count.
  //
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
  CONST UINT8  *Record;
  UINT32       RecordLength;
  UINT32       NameLength;
  UINT32       DevicePathLength;
  UINT32       NumberOfSignatures;

  if ((Iterator == NULL) || (Iterator->ImagesRemaining == 0)) {
    return NULL;
  }

  Record             = Iterator->NextImageRecord;
  RecordLength       = ReadUnaligned32 ((CONST UINT32 *)(Record + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, Length)));
  NumberOfSignatures = ReadUnaligned32 ((CONST UINT32 *)(Record + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, NumberOfSignatures)));
  NameLength         = ReadUnaligned32 ((CONST UINT32 *)(Record + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, NameLength)));
  DevicePathLength   = ReadUnaligned32 ((CONST UINT32 *)(Record + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, DevicePathLength)));

  //
  // Reset the inner cursor to this image's signature region. Init already proved
  // the offsets and record count are in-bounds, so this arithmetic is safe.
  //
  Iterator->NextSignatureRecord = Record + IVRT_IMAGE_HEADER_SIZE + NameLength + DevicePathLength;
  Iterator->SignaturesRemaining = NumberOfSignatures;

  Iterator->NextImageRecord  = Record + RecordLength;
  Iterator->ImagesRemaining -= 1;

  return (CONST EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Record;
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
  CONST UINT8  *Record;
  UINT32       RecordLength;

  if ((Iterator == NULL) || (Iterator->SignaturesRemaining == 0)) {
    return NULL;
  }

  Record       = Iterator->NextSignatureRecord;
  RecordLength = ReadUnaligned32 ((CONST UINT32 *)(Record + OFFSET_OF (EFI_SIGNATURE_VERIFICATION_RESULT, Length)));

  Iterator->NextSignatureRecord  = Record + RecordLength;
  Iterator->SignaturesRemaining -= 1;

  return (CONST EFI_SIGNATURE_VERIFICATION_RESULT *)Record;
}
