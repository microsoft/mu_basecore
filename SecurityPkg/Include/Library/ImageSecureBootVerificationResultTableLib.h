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

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_LIB_H_
#define IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_LIB_H_

#include <Uefi.h>
#include <Guid/ImageSecureBootVerificationResultTable.h>

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
  );

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
  );

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
  );

///
/// Two-level forward-only iterator over an SBRT.
///
/// Fields are owned by the library implementation; callers should treat them as
/// opaque and only manipulate them through the iterator routines below.
///
typedef struct {
  CONST UINT8    *NextImageRecord;        // Next image record to return.
  UINTN          ImagesRemaining;         // Valid images left in the top-level range.
  CONST UINT8    *NextSignatureRecord;    // Next signature record of the current image.
  UINTN          SignaturesRemaining;     // Signatures left in the current image.
} IMAGE_VERIFICATION_RESULT_ITERATOR;

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
  );

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
  );

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
  );

#endif // IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_LIB_H_
