/** @file
  Forward-only iterators for walking different data structures.

  Iterators perform data validation during initialization. Rather than rejecting a malformed
  container, initialization clamps the iteration range to the valid prefix: it stops at the first
  entry that fails to parse and only iterates over the entries that precede it. This does mean that
  certain iterator implementations walk the data structures twice. Once during initialization to
  validate the data and establish the valid range, and again during iteration to produce the
  results.

  All iterators use the same Init / Next contract. `<Structure>Init(..)` initializes the iterator
  and returns whether the range is complete: TRUE if the whole structure parsed cleanly, FALSE if
  it had to truncate the range because one or more trailing entries were dropped (a parse error or
  unusable inputs). `<Structure>Next(..)` returns the next item or NULL once the range is
  exhausted. Init never logs; callers decide what to log based on the returned boolean.

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

  The initialization validates the list and will truncate the iteration range to the
  last valid entry if the list if malformed.

  @param[out]  Iter        Iterator state to initialize.
  @param[in]   Buffer      Raw database contents, or NULL for an empty database.
  @param[in]   BufferSize  Size of Buffer in bytes; 0 when Buffer is NULL.

  @retval TRUE   The iterator covers every entry in the list.
  @retval FALSE  The iterator was truncated due to invalid arguments or a malformed table.
**/
BOOLEAN
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
    return FALSE;
  }

  Iter->Cursor    = (CONST UINT8 *)Buffer;
  Iter->Remaining = 0;

  //
  // An empty database has nothing to iterate and is not a truncation. A NULL buffer with a
  // non-zero size is inconsistent input; expose an empty iterator and report truncation.
  //
  if (Buffer == NULL) {
    return (BOOLEAN)(BufferSize == 0);
  }

  //
  // Walk the buffer using SignatureListSize. The lists must tile the buffer cleanly; the first
  // list that does not marks the end of the valid iteration range.
  //
  Cursor    = (CONST UINT8 *)Buffer;
  Remaining = BufferSize;

  while (Remaining > 0) {
    if (Remaining < sizeof (EFI_SIGNATURE_LIST)) {
      break;
    }

    List = (CONST EFI_SIGNATURE_LIST *)(CONST VOID *)Cursor;
    if ((List->SignatureListSize < sizeof (EFI_SIGNATURE_LIST)) ||
        (List->SignatureListSize > Remaining))
    {
      break;
    }

    Cursor    += List->SignatureListSize;
    Remaining -= List->SignatureListSize;
  }

  //
  // Remaining is the size of the tail that could not be parsed; the valid prefix is everything
  // that came before it.
  //
  Iter->Cursor    = (CONST UINT8 *)Buffer;
  Iter->Remaining = BufferSize - Remaining;
  return (BOOLEAN)(Remaining == 0);
}

/**
  Return the next EFI_SIGNATURE_LIST from the buffer being iterated.

  Infallible over the range established by DatabaseIterInit.

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

  The initialization validates the list and will truncate the iteration range to the
  last valid entry if the list if malformed.

  @param[out]  Iter  Iterator state to initialize.
  @param[in]   List  The signature list to walk.

  @retval TRUE   The iterator covers every entry in the list.
  @retval FALSE  The iterator was truncated due to invalid arguments or a malformed table.
**/
BOOLEAN
SigListIterInit (
  OUT SIG_LIST_ITER             *Iter,
  IN  CONST EFI_SIGNATURE_LIST  *List
  )
{
  UINTN  PayloadSize;

  if (Iter == NULL) {
    return FALSE;
  }

  Iter->Cursor    = NULL;
  Iter->Stride    = 0;
  Iter->Remaining = 0;

  //
  // Without a list, or with size fields that leave the entry stride or payload region
  // undeterminable, there is no entry that can be safely produced. Expose an empty iterator.
  //
  if (List == NULL) {
    return FALSE;
  }

  if (List->SignatureListSize < sizeof (EFI_SIGNATURE_LIST)) {
    return FALSE;
  }

  if (List->SignatureSize < sizeof (EFI_GUID)) {
    return FALSE;
  }

  if (List->SignatureHeaderSize > List->SignatureListSize - sizeof (EFI_SIGNATURE_LIST)) {
    return FALSE;
  }

  //
  // The payload holds a whole number of fixed-size entries. If it does not divide evenly, drop the
  // trailing partial entry and iterate only the whole entries that precede it.
  //
  PayloadSize = List->SignatureListSize
                - sizeof (EFI_SIGNATURE_LIST)
                - List->SignatureHeaderSize;

  Iter->Stride    = List->SignatureSize;
  Iter->Remaining = PayloadSize / List->SignatureSize;
  Iter->Cursor    = (CONST UINT8 *)List
                    + sizeof (EFI_SIGNATURE_LIST)
                    + List->SignatureHeaderSize;

  return (BOOLEAN)((PayloadSize % List->SignatureSize) == 0);
}

/**
  Return the next EFI_SIGNATURE_DATA entry from the list being iterated.

  Infallible over the range established by SigListIterInit.

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

  The initialization validates the list and will truncate the iteration range to the
  last valid entry if the list if malformed.

  @param[out]  Iter        Iterator state to initialize.
  @param[in]   FileBuffer  Pointer to the in-memory PE/COFF image.
  @param[in]   FileSize    Size of FileBuffer in bytes.
  @param[in]   SecDataDir  Security data directory describing the embedded WIN_CERTIFICATE table.

  @retval TRUE   The iterator covers every entry in the list.
  @retval FALSE  The iterator was truncated due to invalid arguments or a malformed table.
**/
BOOLEAN
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

  if (Iter == NULL) {
    return FALSE;
  }

  Iter->Cursor    = NULL;
  Iter->Remaining = 0;

  //
  // Without a file buffer or directory, or with a directory that does not lie within the file, the
  // certificate table cannot be located. Expose an empty iterator.
  //
  if ((FileBuffer == NULL) || (SecDataDir == NULL)) {
    return FALSE;
  }

  if ((SecDataDir->VirtualAddress > FileSize) ||
      (SecDataDir->Size > FileSize - SecDataDir->VirtualAddress))
  {
    return FALSE;
  }

  //
  // Walk the certificate table. The first entry with a malformed dwLength, or a trailing fragment
  // too small to hold a header, marks the end of the valid iteration range.
  //
  Cursor    = (CONST UINT8 *)FileBuffer + SecDataDir->VirtualAddress;
  Remaining = SecDataDir->Size;

  while (Remaining > 0) {
    if (Remaining < sizeof (WIN_CERTIFICATE)) {
      break;
    }

    Cert = (CONST WIN_CERTIFICATE *)(CONST VOID *)Cursor;

    if ((Cert->dwLength < sizeof (WIN_CERTIFICATE)) ||
        (Cert->dwLength > Remaining))
    {
      break;
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

  //
  // Remaining is the size of the tail that could not be parsed; the valid prefix is everything
  // that came before it.
  //
  Iter->Cursor    = (CONST UINT8 *)FileBuffer + SecDataDir->VirtualAddress;
  Iter->Remaining = SecDataDir->Size - Remaining;
  return (BOOLEAN)(Remaining == 0);
}

/**
  Return the next WIN_CERTIFICATE from the directory being iterated.

  Infallible over the range established by WinCertIterInit.

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
