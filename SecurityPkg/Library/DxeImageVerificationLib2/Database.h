/** @file
  Secureboot DB/DBX/DBT (EFI_SIGNATURE_LIST) helpers for the DXE Image Verification Library.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DXE_IMAGE_VERIFICATION_LIB_DATABASE_H_
#define DXE_IMAGE_VERIFICATION_LIB_DATABASE_H_

#include "DxeImageVerificationLib.h"
#include <Library/UefiLib.h>

//
// Supported image hash algorithms defined via their GUID.
//
STATIC CONST EFI_GUID  *CONST  mKnownImageHashGuids[] = {
  &gEfiCertSha1Guid,
  &gEfiCertSha256Guid,
  &gEfiCertSha384Guid,
  &gEfiCertSha512Guid
};

//
// A Set of image hash signature-type GUIDs
//
typedef struct {
  UINTN       Count;
  EFI_GUID    Guids[ARRAY_SIZE (mKnownImageHashGuids)];
} HASH_ALGORITHM_SET;

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
  );

/**
  A callback that is executed by WalkSignatureDatabase once per
  EFI_SIGNATURE_LIST in a signature database buffer.

  Returning EFI_SUCCESS continues iteration. Returning any other status
  stops the walk and the error is propagated to the caller.

  @param[in]  List     The signature list currently being iterated.
  @param[in]  Context  Caller-owned opaque pointer passed unmodified
                       through WalkSignatureDatabase.

  @retval EFI_SUCCESS            The list was processed successfully.
  @retval other                  Callback-specific error.
**/
typedef
EFI_STATUS
(EFIAPI *SIGNATURE_LIST_CALLBACK)(
  IN CONST EFI_SIGNATURE_LIST  *List,
  IN VOID                      *Context  OPTIONAL
  );

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
  );

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
  );

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
  );

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
  );

#endif // DXE_IMAGE_VERIFICATION_LIB_DATABASE_H_
