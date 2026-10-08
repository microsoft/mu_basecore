/** @file
  ECIT capability payload encoders.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Guid/CryptoIndicatorTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/EcitEncodingLib.h>
#include <Library/MemoryAllocationLib.h>

#define ECIT_ENTRY_ALIGNMENT  8

STATIC
BOOLEAN
IsValidOid (
  IN CONST BASE_CRYPT_OP_CAPABILITY  *Capability
  )
{
  UINTN    Index;
  UINTN    ArcIndex;
  UINTN    ArcValue;
  UINTN    FirstArc;
  UINTN    Digit;
  BOOLEAN  InArc;

  if ((Capability->AlgorithmOid == NULL) || (Capability->AlgorithmOidSize <= 1) ||
      (Capability->AlgorithmOid[Capability->AlgorithmOidSize - 1] != '\0'))
  {
    return FALSE;
  }

  ArcIndex = 0;
  ArcValue = 0;
  FirstArc = 0;
  InArc    = FALSE;
  for (Index = 0; Index < Capability->AlgorithmOidSize - 1; Index++) {
    if ((Capability->AlgorithmOid[Index] >= '0') &&
        (Capability->AlgorithmOid[Index] <= '9'))
    {
      Digit = Capability->AlgorithmOid[Index] - '0';
      if (ArcValue > ((MAX_UINTN - Digit) / 10)) {
        return FALSE;
      }

      ArcValue = (ArcValue * 10) + Digit;
      InArc    = TRUE;
      continue;
    }

    if ((Capability->AlgorithmOid[Index] != '.') || !InArc) {
      return FALSE;
    }

    if ((ArcIndex == 0) && (ArcValue > 2)) {
      return FALSE;
    }

    if (ArcIndex == 0) {
      FirstArc = ArcValue;
    } else if ((ArcIndex == 1) && (FirstArc < 2) && (ArcValue > 39)) {
      return FALSE;
    }

    ArcIndex++;
    ArcValue = 0;
    InArc    = FALSE;
  }

  if (!InArc || (ArcIndex < 1)) {
    return FALSE;
  }

  if ((ArcIndex == 1) && (FirstArc < 2) && (ArcValue > 39)) {
    return FALSE;
  }

  return TRUE;
}

STATIC
BOOLEAN
IsDuplicate (
  IN CONST BASE_CRYPT_OP_CAPABILITY  *Capabilities,
  IN UINTN                           CapabilityIndex
  )
{
  UINTN  Index;

  for (Index = 0; Index < CapabilityIndex; Index++) {
    if ((Capabilities[Index].AlgorithmOidSize ==
         Capabilities[CapabilityIndex].AlgorithmOidSize) &&
        (CompareMem (
           Capabilities[Index].AlgorithmOid,
           Capabilities[CapabilityIndex].AlgorithmOid,
           Capabilities[Index].AlgorithmOidSize
           ) == 0))
    {
      return TRUE;
    }
  }

  return FALSE;
}

EFI_STATUS
EFIAPI
EcitEncodeOidSet (
  IN  CONST BASE_CRYPT_OP_CAPABILITY  *Capabilities OPTIONAL,
  IN  UINTN                           CapabilityCount,
  OUT VOID                            **EntryData,
  OUT UINTN                           *EntryDataSize
  )
{
  UINTN  Index;
  UINTN  SemanticSize;
  UINTN  EncodedSize;
  UINTN  UniqueCount;
  UINT8  *Encoded;
  UINT8  *Cursor;

  if (EntryData != NULL) {
    *EntryData = NULL;
  }

  if (EntryDataSize != NULL) {
    *EntryDataSize = 0;
  }

  if ((EntryData == NULL) || (EntryDataSize == NULL) ||
      ((CapabilityCount != 0) && (Capabilities == NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  SemanticSize = 1;
  UniqueCount  = 0;
  for (Index = 0; Index < CapabilityCount; Index++) {
    if (!IsValidOid (&Capabilities[Index])) {
      return EFI_INVALID_PARAMETER;
    }

    if (IsDuplicate (Capabilities, Index)) {
      continue;
    }

    if (SemanticSize >
        (MAX_UINTN - (Capabilities[Index].AlgorithmOidSize - 1) -
         ((UniqueCount == 0) ? 0 : 1)))
    {
      return EFI_OUT_OF_RESOURCES;
    }

    SemanticSize += Capabilities[Index].AlgorithmOidSize - 1;
    if (UniqueCount != 0) {
      SemanticSize++;
    }

    UniqueCount++;
  }

  if (SemanticSize > (MAX_UINTN - (ECIT_ENTRY_ALIGNMENT - 1))) {
    return EFI_OUT_OF_RESOURCES;
  }

  EncodedSize = ALIGN_VALUE (SemanticSize, ECIT_ENTRY_ALIGNMENT);
  if (EncodedSize > (MAX_UINT16 - sizeof (EFI_CRYPTO_INDICATOR_ENTRY))) {
    return EFI_OUT_OF_RESOURCES;
  }

  Encoded = AllocateZeroPool (EncodedSize);
  if (Encoded == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cursor      = Encoded;
  UniqueCount = 0;
  for (Index = 0; Index < CapabilityCount; Index++) {
    if (IsDuplicate (Capabilities, Index)) {
      continue;
    }

    if (UniqueCount != 0) {
      *Cursor++ = ',';
    }

    CopyMem (
      Cursor,
      Capabilities[Index].AlgorithmOid,
      Capabilities[Index].AlgorithmOidSize - 1
      );
    Cursor += Capabilities[Index].AlgorithmOidSize - 1;
    UniqueCount++;
  }

  *EntryData     = Encoded;
  *EntryDataSize = EncodedSize;
  return EFI_SUCCESS;
}
