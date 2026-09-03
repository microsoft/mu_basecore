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

#pragma once

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
  );

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
  );

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
  );

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
  );

/**
  Initialize an iterator over a packed table of WIN_CERTIFICATE records (a PE/COFF image's
  attribute-certificate data).

  The initialization validates the table and will truncate the iteration range to the last valid
  entry if the table is malformed.

  @param[out]  Iter             Iterator state to initialize.
  @param[in]   WinCertificates  The WIN_CERTIFICATE table, or NULL when there are none.
  @param[in]   Length           Length of the table in bytes; 0 when WinCertificates is NULL.

  @retval TRUE   The iterator covers every entry in the table.
  @retval FALSE  The iterator was truncated due to invalid arguments or a malformed table.
**/
BOOLEAN
WinCertIterInit (
  OUT WIN_CERT_ITER          *Iter,
  IN  CONST WIN_CERTIFICATE  *WinCertificates,
  IN  UINTN                  Length
  );

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
  );

//
// Control returned by a per-entry visitor to WalkDatabase.
//
typedef enum {
  WalkContinue,   // Continue to the next entry in the current list.
  WalkSkipList,   // Skip the remaining entries of the current list; continue with the next list.
  WalkStop        // Stop the walk; WalkDatabase returns TRUE.
} WALK_ACTION;

/**
  Per-entry callback invoked by WalkDatabase for each entry of every EFI_SIGNATURE_LIST in a
  database's valid prefix.

  The entry layout depends on SignatureType (a V1 EFI_SIGNATURE_DATA carries a 16-byte
  SignatureOwner prefix; a V2 EFI_SIGNATURE_V2_DATA does not), so Entry is passed untyped and the
  visitor interprets it. Returning WalkContinue advances to the next entry, WalkSkipList abandons
  the rest of the current list (e.g. once its type is known to be irrelevant), and WalkStop ends the
  walk.

  @param[in]      SignatureType  The list's EFI_SIGNATURE_LIST SignatureType GUID.
  @param[in]      Entry          The current entry (raw bytes; V1 or V2 layout per SignatureType).
  @param[in]      EntrySize      The entry size in bytes (the list's SignatureSize).
  @param[in,out]  Context        Caller state threaded through the walk.

  @retval WalkContinue  Continue to the next entry in the current list.
  @retval WalkSkipList  Skip the rest of the current list and continue with the next list.
  @retval WalkStop      Stop the walk.
**/
typedef
WALK_ACTION
(EFIAPI *SIG_ENTRY_VISITOR)(
  IN     CONST EFI_GUID  *SignatureType,
  IN     CONST VOID      *Entry,
  IN     UINTN           EntrySize,
  IN OUT VOID            *Context
  );

/**
  Walk each entry of every EFI_SIGNATURE_LIST in a signature database, invoking Visit for each, and
  report whether Visit stopped the walk.

  This is the single iteration primitive shared by the membership searches and the certificate
  evaluator. It owns the database and list iterators and accounts structural truncation, but does
  not interpret entries: it hands each entry's SignatureType, bytes, and size to Visit, which
  classifies and locates the payload itself. Visit may skip the rest of a list (WalkSkipList) or end
  the walk (WalkStop).

  @param[in]      Database      Raw database contents, or NULL for an empty database.
  @param[in]      DatabaseSize  Size of Database in bytes; 0 when Database is NULL.
  @param[in]      Visit         Per-entry callback. Required.
  @param[in,out]  Context       State threaded to Visit.
  @param[out]     Truncated     Optional. Set TRUE if the walk could not be proven complete (a
                                malformed database or list clamped the range).

  @retval TRUE   Visit returned WalkStop for some entry.
  @retval FALSE  The walk completed without Visit stopping it.
**/
BOOLEAN
WalkDatabase (
  IN     CONST VOID         *Database,
  IN     UINTN              DatabaseSize,
  IN     SIG_ENTRY_VISITOR  Visit,
  IN OUT VOID               *Context,
  OUT    BOOLEAN            *Truncated   OPTIONAL
  );
