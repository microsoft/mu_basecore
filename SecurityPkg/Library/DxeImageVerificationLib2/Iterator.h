/** @file
  Forward-only iterators for walking different data structures.

  Iterators perform data validation during initialization. If initialization succeeds, the
  iterator is guaranteed to produce valid results during iteration. This does mean that certain
  iterator implemenations walk the data structures twice. Once during initialization to validate
  the data, and again during iteration to produce the results.

  All iterators use the same Init / Next contract. `<Structure>Init(..)` initializes the iterator
  and validates the structure while `<Structure>Next(..)` returns the next item or NULL once
  initialized.

  Three iterators are provided:

    * SIG_DATABASE_ITER - walks every EFI_SIGNATURE_LIST in a signature
                          database buffer (db / dbx / dbt contents).
    * SIG_LIST_ITER     - walks every EFI_SIGNATURE_DATA entry inside a
                          single EFI_SIGNATURE_LIST.
    * WIN_CERT_ITER     - walks every WIN_CERTIFICATE in a PE/COFF
                          image's security data directory.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DXE_IMAGE_VERIFICATION_LIB_ITERATOR_H_
#define DXE_IMAGE_VERIFICATION_LIB_ITERATOR_H_

#include <Uefi.h>
#include <Guid/ImageAuthentication.h>
#include <IndustryStandard/PeImage.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Iterator state for walking EFI_SIGNATURE_LIST records inside a
  signature-database buffer.

  Fields are owned by the iterator implementation; callers should treat them as opaque.
**/
typedef struct {
  CONST UINT8    *Cursor;     // Next byte to consume.
  UINTN          Remaining;   // Bytes left in the buffer.
} SIG_DATABASE_ITER;

/**
  Iterator state for walking EFI_SIGNATURE_DATA entries inside a single EFI_SIGNATURE_LIST.

  Fields are owned by the iterator implementation; callers should treat them as opaque.
**/
typedef struct {
  CONST UINT8    *Cursor;     // Next entry pointer.
  UINTN          Stride;      // SignatureSize from the parent list.
  UINTN          Remaining;   // Entries left to produce.
} SIG_LIST_ITER;

/**
  Iterator state for walking WIN_CERTIFICATE records inside a PE/COFF image's security data
  directory.

  Fields are owned by the iterator implementation; callers should treat them as opaque.
**/
typedef struct {
  CONST UINT8    *Cursor;     // Next byte to consume.
  UINTN          Remaining;   // Bytes left in the certificate table.
} WIN_CERT_ITER;

/**
  Initialize an iterator over the EFI_SIGNATURE_LIST records contained in a signature database
  buffer.

  Init validates every list header in the buffer end-to-end. After a successful return,
  DatabaseIterNext is infallible.

  @param[out]  Iter        Iterator state to initialize.
  @param[in]   Buffer      Raw database contents, or NULL for an empty database.
  @param[in]   BufferSize  Size of Buffer in bytes; 0 when Buffer is NULL.

  @retval EFI_SUCCESS            Iterator is ready for use.
  @retval EFI_INVALID_PARAMETER  Iter is NULL, or Buffer is NULL with a non-zero BufferSize.
  @retval EFI_VOLUME_CORRUPTED   The buffer contains a malformed EFI_SIGNATURE_LIST (bad size
                                 fields, trailing bytes, or payload not a multiple of
                                 SignatureSize).
**/
EFI_STATUS
DatabaseIterInit (
  OUT SIG_DATABASE_ITER  *Iter,
  IN  CONST VOID         *Buffer,
  IN  UINTN              BufferSize
  );

/**
  Return the next EFI_SIGNATURE_LIST from the buffer being iterated.

  Cannot fail after a successful DatabaseIterInit.

  @param[in,out]  Iter  Iterator initialized by DatabaseIterInit.

  @retval non-NULL  Pointer to the next EFI_SIGNATURE_LIST.
  @retval NULL      Iteration is complete.
**/
CONST EFI_SIGNATURE_LIST *
DatabaseIterNext (
  IN OUT SIG_DATABASE_ITER  *Iter
  );

/**
  Initialize an iterator over the EFI_SIGNATURE_DATA entries contained in a single
  EFI_SIGNATURE_LIST.

  Init validates the list's size fields. After a successful return, SigListIterNext is infallible.

  @param[out]  Iter  Iterator state to initialize.
  @param[in]   List  The signature list to walk.

  @retval EFI_SUCCESS            Iterator is ready for use.
  @retval EFI_INVALID_PARAMETER  Iter or List is NULL.
  @retval EFI_VOLUME_CORRUPTED   List has internally inconsistent size fields.
**/
EFI_STATUS
SigListIterInit (
  OUT SIG_LIST_ITER             *Iter,
  IN  CONST EFI_SIGNATURE_LIST  *List
  );

/**
  Return the next EFI_SIGNATURE_DATA entry from the list being iterated.

  Cannot fail after a successful SigListIterInit.

  @param[in,out]  Iter  Iterator initialized by SigListIterInit.

  @retval non-NULL  Pointer to the next EFI_SIGNATURE_DATA entry.
  @retval NULL      Iteration is complete.
**/
CONST EFI_SIGNATURE_DATA *
SigListIterNext (
  IN OUT SIG_LIST_ITER  *Iter
  );

/**
  Initialize an iterator over the WIN_CERTIFICATE records contained in a PE/COFF image's security
  data directory.

  Init validates that the directory described by SecDataDir lies entirely within the supplied file
  buffer and walks every WIN_CERTIFICATE header to verify its dwLength fits the remaining table.
  After a successful return, WinCertIterNext is infallible.

  @param[out]  Iter        Iterator state to initialize.
  @param[in]   FileBuffer  Pointer to the in-memory PE/COFF image.
  @param[in]   FileSize    Size of FileBuffer in bytes.
  @param[in]   SecDataDir  Security data directory describing the embedded WIN_CERTIFICATE table.

  @retval EFI_SUCCESS            Iterator is ready for use.
  @retval EFI_INVALID_PARAMETER  Iter, FileBuffer, or SecDataDir is NULL.
  @retval EFI_VOLUME_CORRUPTED   SecDataDir is out of bounds of the file, or an entry has a
                                 malformed dwLength.
**/
EFI_STATUS
WinCertIterInit (
  OUT WIN_CERT_ITER                   *Iter,
  IN  CONST VOID                      *FileBuffer,
  IN  UINTN                           FileSize,
  IN  CONST EFI_IMAGE_DATA_DIRECTORY  *SecDataDir
  );

/**
  Return the next WIN_CERTIFICATE from the directory being iterated.

  Cannot fail after a successful WinCertIterInit.

  @param[in,out]  Iter  Iterator initialized by WinCertIterInit.

  @retval non-NULL  Pointer to the next WIN_CERTIFICATE.
  @retval NULL      Iteration is complete.
**/
CONST WIN_CERTIFICATE *
WinCertIterNext (
  IN OUT WIN_CERT_ITER  *Iter
  );

#endif // DXE_IMAGE_VERIFICATION_LIB_ITERATOR_H_
