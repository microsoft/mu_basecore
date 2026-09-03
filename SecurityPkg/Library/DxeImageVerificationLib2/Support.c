/** @file
  Utility functions for DxeImageVerificationLib.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Support.h"

/**
  Populate Authority with a newly allocated V1 EFI_SIGNATURE_DATA that wraps a certificate payload.

  The allocation is a 16-byte SignatureOwner followed by a copy of Payload. It is owned by the
  caller and released with FreeImageAuthority (). Authority->SignatureType is left unchanged so the
  caller can record the authorizing list's signature type independently.

  @param[in]   Owner        SignatureOwner GUID to store, or NULL to store a zeroed GUID (used for a
                            matching V2 EFI_SIGNATURE_V2_DATA entry, which carries no owner).
  @param[in]   Payload      The certificate (or other signature payload) to copy.
  @param[in]   PayloadSize  Size of Payload in bytes.
  @param[out]  Authority    On success, Authority->Data references the allocated EFI_SIGNATURE_DATA
                            and Authority->Size is its total length.

  @retval EFI_SUCCESS            Authority was populated.
  @retval EFI_INVALID_PARAMETER  Payload or Authority is NULL, or PayloadSize is 0 or too large.
  @retval EFI_OUT_OF_RESOURCES   The allocation failed.
**/
EFI_STATUS
BuildImageAuthority (
  IN  CONST EFI_GUID   *Owner  OPTIONAL,
  IN  CONST UINT8      *Payload,
  IN  UINTN            PayloadSize,
  OUT IMAGE_AUTHORITY  *Authority
  )
{
  EFI_SIGNATURE_DATA  *SigData;
  UINTN               TotalSize;

  if ((Payload == NULL) || (PayloadSize == 0) || (Authority == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Reject a payload large enough to overflow the header addition. PayloadSize is derived from
  // attacker-controlled db/dbx and image data, so the bound is checked before allocation.
  //
  if (PayloadSize > MAX_UINTN - OFFSET_OF (EFI_SIGNATURE_DATA, SignatureData)) {
    return EFI_INVALID_PARAMETER;
  }

  TotalSize = OFFSET_OF (EFI_SIGNATURE_DATA, SignatureData) + PayloadSize;

  SigData = AllocateZeroPool (TotalSize);
  if (SigData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // A NULL Owner leaves the zeroed SignatureOwner from AllocateZeroPool in place (V2 source entry).
  //
  if (Owner != NULL) {
    CopyGuid (&SigData->SignatureOwner, Owner);
  }

  CopyMem (SigData->SignatureData, Payload, PayloadSize);

  Authority->Data = SigData;
  Authority->Size = TotalSize;

  return EFI_SUCCESS;
}

/**
  Release the allocation owned by an IMAGE_AUTHORITY.

  Frees Authority->Data (if any) and clears Authority->Data / Authority->Size. Authority->SignatureType
  is left intact. Safe to call on an already-empty authority or a NULL pointer.

  @param[in,out]  Authority  Authority whose owned Data is released.
**/
VOID
FreeImageAuthority (
  IN OUT IMAGE_AUTHORITY  *Authority
  )
{
  if (Authority == NULL) {
    return;
  }

  if (Authority->Data != NULL) {
    FreePool (Authority->Data);
    Authority->Data = NULL;
  }

  Authority->Size = 0;
}
