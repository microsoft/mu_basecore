/** @file
  Secureboot DB/DBX/DBT (EFI_SIGNATURE_LIST) helpers for the DXE Image Verification Library.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Database.h"

/**
  Load a Secure Boot Signature Database into a pool-allocated buffer.

  When *Buffer is non-NULL the caller takes ownership of the allocation and
  must release it with FreePool.

  @param[in]   DatabaseName  Variable name (e.g. EFI_IMAGE_SECURITY_DATABASE,
                             EFI_IMAGE_SECURITY_DATABASE1).
  @param[out]  Buffer        Pool-allocated copy of the variable contents,
                             or NULL if the variable does not exist.
                             Caller frees with FreePool when non-NULL.
  @param[out]  BufferSize    Size of *Buffer in bytes, or 0 if the
                             variable does not exist.

  @retval EFI_SUCCESS            The variable was loaded successfully, or it was absent.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval Other                  Status from gRT->GetVariable.
**/
EFI_STATUS
LoadSignatureDatabase (
  IN  CONST CHAR16  *DatabaseName,
  OUT VOID          **Buffer,
  OUT UINTN         *BufferSize
  )
{
  EFI_STATUS  Status;

  if ((DatabaseName == NULL) || (Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Buffer     = NULL;
  *BufferSize = 0;

  Status = GetVariable2 (DatabaseName, &gEfiImageSecurityDatabaseGuid, Buffer, BufferSize);
  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  return Status;
}

/**
  Append a GUID to the set if it is not already present.

  @param[in,out]  Set   The set to update.
  @param[in]      Guid  The GUID to insert.
**/
STATIC
VOID
AppendUnique (
  IN OUT HASH_ALGORITHM_SET  *Set,
  IN CONST EFI_GUID          *Guid
  )
{
  UINTN  Index;

  for (Index = 0; Index < Set->Count; Index++) {
    if (CompareGuid (&Set->Guids[Index], Guid)) {
      return;
    }
  }

  ASSERT (Set->Count < ARRAY_SIZE (Set->Guids));
  if (Set->Count < ARRAY_SIZE (Set->Guids)) {
    CopyGuid (&Set->Guids[Set->Count], Guid);
    Set->Count++;
  }
}

/**
  Determines if the given GUID is a supported image hash signature type.

  @param[in]  Guid  Pointer to an EFI_SIGNATURE_LIST::SignatureType
                    value, or any candidate signature-type GUID.

  @retval TRUE   The image hash signature type is supported.
  @retval FALSE  The image hash signature type is not supported.
**/
BOOLEAN
IsKnownImageHashGuid (
  IN CONST EFI_GUID  *Guid
  )
{
  UINTN  Index;

  if (Guid == NULL) {
    return FALSE;
  }

  for (Index = 0; Index < ARRAY_SIZE (mKnownImageHashGuids); Index++) {
    if (CompareGuid (Guid, mKnownImageHashGuids[Index])) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Walk a signature-database buffer, invoking Callback for every
  well-formed EFI_SIGNATURE_LIST it contains.

  @param[in]  Buffer      The raw database contents.
  @param[in]  BufferSize  Size of Buffer in bytes.
  @param[in]  Callback    Invoked once per EFI_SIGNATURE_LIST.
  @param[in]  Context     Opaque pointer passed unmodified to Callback.

  @retval EFI_SUCCESS            Buffer was fully consumed and Callback
                                 returned EFI_SUCCESS for every list.
  @retval EFI_INVALID_PARAMETER  Buffer or Callback is NULL.
  @retval EFI_VOLUME_CORRUPTED   Buffer is structurally invalid.
  @retval Other                  First non-EFI_SUCCESS status returned
                                 by Callback. Iteration stops immediately.
**/
EFI_STATUS
WalkSignatureDatabase (
  IN  CONST VOID               *Buffer,
  IN  UINTN                    BufferSize,
  IN  SIGNATURE_LIST_CALLBACK  Callback,
  IN  VOID                     *Context  OPTIONAL
  )
{
  CONST UINT8         *Cursor;
  UINTN               Remaining;
  EFI_SIGNATURE_LIST  *List;
  UINTN               PayloadSize;
  EFI_STATUS          CallbackStatus;

  if ((Buffer == NULL) || (Callback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Cursor    = (CONST UINT8 *)Buffer;
  Remaining = BufferSize;

  while (Remaining >= sizeof (EFI_SIGNATURE_LIST)) {
    List = (EFI_SIGNATURE_LIST *)(VOID *)Cursor;

    //
    // Outer header bounds: the list must declare at least the header
    // size, and must fit inside what remains of the buffer.
    //
    if ((List->SignatureListSize < sizeof (EFI_SIGNATURE_LIST)) ||
        (List->SignatureListSize > Remaining))
    {
      DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: malformed signature list size.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Header + per-list header must fit within the declared list size,
    // and individual signatures must each be at least an EFI_GUID. The
    // remaining payload must divide evenly into SignatureSize chunks.
    //
    if ((List->SignatureSize < sizeof (EFI_GUID)) ||
        (List->SignatureHeaderSize > List->SignatureListSize - sizeof (EFI_SIGNATURE_LIST)))
    {
      DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: malformed signature list fields.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    PayloadSize = List->SignatureListSize
                  - sizeof (EFI_SIGNATURE_LIST)
                  - List->SignatureHeaderSize;
    if ((PayloadSize % List->SignatureSize) != 0) {
      DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: signature payload not a multiple of SignatureSize.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    CallbackStatus = Callback (List, Context);
    if (EFI_ERROR (CallbackStatus)) {
      return CallbackStatus;
    }

    Cursor    += List->SignatureListSize;
    Remaining -= List->SignatureListSize;
  }

  if (Remaining != 0) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: trailing bytes in signature database.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  return EFI_SUCCESS;
}

/**
  Walker callback for GetDatabaseHashAlgorithms: appends the list's
  SignatureType to the caller's set if it is a recognized image hash
  GUID.
**/
STATIC
EFI_STATUS
EFIAPI
CollectHashAlgorithmCallback (
  IN CONST EFI_SIGNATURE_LIST  *List,
  IN VOID                      *Context  OPTIONAL
  )
{
  HASH_ALGORITHM_SET  *Set;

  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Set = (HASH_ALGORITHM_SET *)Context;

  if (IsKnownImageHashGuid (&List->SignatureType)) {
    AppendUnique (Set, &List->SignatureType);
  }

  return EFI_SUCCESS;
}

/**
  Gathers all image hash signature types present in the Db and Dbx.

  @param[in]   Db              The raw `db` variable contents.
  @param[in]   DbSize          Size of Db in bytes.
  @param[in]   Dbx             The raw `dbx` variable contents.
  @param[in]   DbxSize         Size of Dbx in bytes.
  @param[out]  HashAlgorithms  On success, populated with the set of hash
                               signature types.

  @retval EFI_SUCCESS            Db / Dbx were walked and HashAlgorithms was populated.
  @retval EFI_INVALID_PARAMETER  HashAlgorithms is NULL.
  @retval EFI_VOLUME_CORRUPTED   One of the buffers' signature lists is malformed.
**/
EFI_STATUS
GetDatabaseHashAlgorithms (
  IN  CONST VOID          *Db        OPTIONAL,
  IN  UINTN               DbSize,
  IN  CONST VOID          *Dbx       OPTIONAL,
  IN  UINTN               DbxSize,
  OUT HASH_ALGORITHM_SET  *HashAlgorithms
  )
{
  EFI_STATUS  Status;

  if (HashAlgorithms == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (HashAlgorithms, sizeof (*HashAlgorithms));

  if (Db != NULL) {
    Status = WalkSignatureDatabase (Db, DbSize, CollectHashAlgorithmCallback, HashAlgorithms);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Dbx != NULL) {
    Status = WalkSignatureDatabase (Dbx, DbxSize, CollectHashAlgorithmCallback, HashAlgorithms);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

//
// Per-walk context for IsSignatureFoundInDatabase. The caller
// populates the three input fields and clears Found; the walker
// callback updates Found if a matching entry is encountered.
//
typedef struct {
  CONST UINT8       *Signature;
  CONST EFI_GUID    *SignatureType;
  UINTN             SignatureSize;
  BOOLEAN           Found;
} SIGNATURE_SEARCH_CTX;

/**
  Walker callback for IsSignatureFoundInDatabase.

  Records a match by setting Search->Found to TRUE and returning EFI_ABORTED
  so the walk terminates early.
**/
STATIC
EFI_STATUS
EFIAPI
SignatureSearchCallback (
  IN CONST EFI_SIGNATURE_LIST  *List,
  IN VOID                      *Context  OPTIONAL
  )
{
  SIGNATURE_SEARCH_CTX      *Search;
  UINTN                     EntryCount;
  CONST UINT8               *EntryCursor;
  CONST EFI_SIGNATURE_DATA  *Entry;
  UINTN                     Index;

  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Search = (SIGNATURE_SEARCH_CTX *)Context;

  if ((Search->Signature == NULL) ||
      (Search->SignatureType == NULL) ||
      (Search->SignatureSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // A list belongs to a different algorithm if either the type GUID or
  // the per-entry size disagrees. EFI_SIGNATURE_DATA already carries an
  // owner EFI_GUID, so the payload size is SignatureSize - sizeof(GUID).
  //
  if (!CompareGuid (&List->SignatureType, Search->SignatureType)) {
    return EFI_SUCCESS;
  }

  if (List->SignatureSize != sizeof (EFI_GUID) + Search->SignatureSize) {
    return EFI_SUCCESS;
  }

  EntryCount = (List->SignatureListSize
                - sizeof (EFI_SIGNATURE_LIST)
                - List->SignatureHeaderSize) / List->SignatureSize;
  EntryCursor = (CONST UINT8 *)List
                + sizeof (EFI_SIGNATURE_LIST)
                + List->SignatureHeaderSize;

  for (Index = 0; Index < EntryCount; Index++) {
    Entry = (CONST EFI_SIGNATURE_DATA *)EntryCursor;
    if (CompareMem (Entry->SignatureData, Search->Signature, Search->SignatureSize) == 0) {
      Search->Found = TRUE;
      return EFI_ABORTED;
    }

    EntryCursor += List->SignatureSize;
  }

  return EFI_SUCCESS;
}

/**
  Search a image signature database buffer for a signature match.

  @param[in]   Database       The raw database contents.
  @param[in]   DatabaseSize   Size of Database in bytes.
  @param[in]   Signature      The signature digest to search for.
  @param[in]   SignatureType  GUID identifying the digest algorithm.
  @param[in]   SignatureSize  Size of Signature digest in bytes.
  @param[out]  IsFound        TRUE if the signature was located.

  @retval EFI_SUCCESS            Search completed; IsFound is valid.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL or SignatureSize is 0.
  @retval EFI_VOLUME_CORRUPTED   Database is structurally malformed.
**/
EFI_STATUS
IsSignatureFoundInDatabase (
  IN  CONST VOID      *Database  OPTIONAL,
  IN  UINTN           DatabaseSize,
  IN  CONST UINT8     *Signature,
  IN  CONST EFI_GUID  *SignatureType,
  IN  UINTN           SignatureSize,
  OUT BOOLEAN         *IsFound
  )
{
  EFI_STATUS            Status;
  SIGNATURE_SEARCH_CTX  Search;

  if ((Signature == NULL) || (SignatureType == NULL) ||
      (IsFound == NULL) || (SignatureSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  *IsFound = FALSE;

  if (Database == NULL) {
    return EFI_SUCCESS;
  }

  Search = (SIGNATURE_SEARCH_CTX) {
    .Signature     = Signature,
    .SignatureType = SignatureType,
    .SignatureSize = SignatureSize,
    .Found         = FALSE
  };

  Status = WalkSignatureDatabase (
             Database,
             DatabaseSize,
             SignatureSearchCallback,
             &Search
             );
  //
  // SignatureSearchCallback returns EFI_ABORTED to stop the walk
  // once a match is recorded in Search.Found; translate it back here.
  //
  if (Status == EFI_ABORTED) {
    Status = EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    *IsFound = Search.Found;
  }

  return Status;
}
