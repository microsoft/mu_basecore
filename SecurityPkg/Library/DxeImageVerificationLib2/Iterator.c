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

#include "Iterator.h"

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
  )
{
  CONST UINT8               *Cursor;
  UINTN                     Remaining;
  CONST EFI_SIGNATURE_LIST  *List;

  if (Iter == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Buffer == NULL) && (BufferSize != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Walk the buffer using SignatureListSize. The only thing this iter
  // needs to guarantee is that the lists tile the buffer cleanly.
  //
  Cursor    = (CONST UINT8 *)Buffer;
  Remaining = (Buffer == NULL) ? 0 : BufferSize;

  while (Remaining > 0) {
    if (Remaining < sizeof (EFI_SIGNATURE_LIST)) {
      DEBUG ((DEBUG_ERROR, "DatabaseIter: trailing bytes in signature database.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    List = (CONST EFI_SIGNATURE_LIST *)(CONST VOID *)Cursor;
    if ((List->SignatureListSize < sizeof (EFI_SIGNATURE_LIST)) ||
        (List->SignatureListSize > Remaining))
    {
      DEBUG ((DEBUG_ERROR, "DatabaseIter: malformed SignatureListSize.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    Cursor    += List->SignatureListSize;
    Remaining -= List->SignatureListSize;
  }

  Iter->Cursor    = (CONST UINT8 *)Buffer;
  Iter->Remaining = (Buffer == NULL) ? 0 : BufferSize;
  return EFI_SUCCESS;
}

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
  )
{
  CONST EFI_SIGNATURE_LIST  *List;

  if ((Iter == NULL) || (Iter->Remaining == 0)) {
    return NULL;
  }

  List             = (CONST EFI_SIGNATURE_LIST *)(CONST VOID *)Iter->Cursor;
  Iter->Cursor    += List->SignatureListSize;
  Iter->Remaining -= List->SignatureListSize;
  return List;
}

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
  )
{
  UINTN  PayloadSize;

  if ((Iter == NULL) || (List == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (List->SignatureListSize < sizeof (EFI_SIGNATURE_LIST)) {
    DEBUG ((DEBUG_ERROR, "SigListIter: malformed SignatureListSize.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  if (List->SignatureSize < sizeof (EFI_GUID)) {
    DEBUG ((DEBUG_ERROR, "SigListIter: malformed SignatureSize.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  if (List->SignatureHeaderSize > List->SignatureListSize - sizeof (EFI_SIGNATURE_LIST)) {
    DEBUG ((DEBUG_ERROR, "SigListIter: malformed SignatureHeaderSize.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  PayloadSize = List->SignatureListSize
                - sizeof (EFI_SIGNATURE_LIST)
                - List->SignatureHeaderSize;
  if ((PayloadSize % List->SignatureSize) != 0) {
    DEBUG ((DEBUG_ERROR, "SigListIter: payload size is not a multiple of signature size.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  Iter->Stride    = List->SignatureSize;
  Iter->Remaining = PayloadSize / List->SignatureSize;
  Iter->Cursor    = (CONST UINT8 *)List
                    + sizeof (EFI_SIGNATURE_LIST)
                    + List->SignatureHeaderSize;
  return EFI_SUCCESS;
}

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
  )
{
  CONST EFI_SIGNATURE_DATA  *Entry;

  if ((Iter == NULL) || (Iter->Remaining == 0)) {
    return NULL;
  }

  Entry         = (CONST EFI_SIGNATURE_DATA *)(CONST VOID *)Iter->Cursor;
  Iter->Cursor += Iter->Stride;
  Iter->Remaining--;
  return Entry;
}

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
  )
{
  CONST UINT8            *Cursor;
  UINTN                  Remaining;
  CONST WIN_CERTIFICATE  *Cert;
  UINTN                  EntrySize;

  if ((Iter == NULL) || (FileBuffer == NULL) || (SecDataDir == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((SecDataDir->VirtualAddress > FileSize) ||
      (SecDataDir->Size > FileSize - SecDataDir->VirtualAddress))
  {
    DEBUG ((DEBUG_ERROR, "WinCertIter: security data directory is out of bounds of the file.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Validate the entire certificate table up front.
  //
  Cursor    = (CONST UINT8 *)FileBuffer + SecDataDir->VirtualAddress;
  Remaining = SecDataDir->Size;

  while (Remaining > 0) {
    if (Remaining < sizeof (WIN_CERTIFICATE)) {
      DEBUG ((DEBUG_ERROR, "Iterator: trailing bytes in certificate table.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    Cert = (CONST WIN_CERTIFICATE *)(CONST VOID *)Cursor;

    if ((Cert->dwLength < sizeof (WIN_CERTIFICATE)) ||
        (Cert->dwLength > Remaining))
    {
      DEBUG ((DEBUG_ERROR, "Iterator: malformed WIN_CERTIFICATE dwLength.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Each entry is padded to an 8-byte boundary. The rounded-up size
    // may exceed Remaining if the final entry uses up the rest of the
    // table without padding; clamp to Remaining in that case.
    //
    EntrySize = ALIGN_VALUE (Cert->dwLength, 8);
    if (EntrySize > Remaining) {
      EntrySize = Remaining;
    }

    Cursor    += EntrySize;
    Remaining -= EntrySize;
  }

  Iter->Cursor    = (CONST UINT8 *)FileBuffer + SecDataDir->VirtualAddress;
  Iter->Remaining = SecDataDir->Size;
  return EFI_SUCCESS;
}

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
  )
{
  CONST WIN_CERTIFICATE  *Cert;
  UINTN                  EntrySize;

  if ((Iter == NULL) || (Iter->Remaining == 0)) {
    return NULL;
  }

  Cert      = (CONST WIN_CERTIFICATE *)(CONST VOID *)Iter->Cursor;
  EntrySize = ALIGN_VALUE (Cert->dwLength, 8);
  if (EntrySize > Iter->Remaining) {
    EntrySize = Iter->Remaining;
  }

  Iter->Cursor    += EntrySize;
  Iter->Remaining -= EntrySize;
  return Cert;
}
