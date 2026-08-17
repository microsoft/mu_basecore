/** @file
  Standalone MM instance of CryptoIndicatorRegistrationLib.

  Stores records in the linking MM module and exposes them to the DXE bridge.
  This instance may be linked into only one MM module in a platform.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/CryptoIndicatorRegistrationLib.h>
#include <Guid/CryptoIndicatorTable.h>
#include <Guid/CryptoIndicatorMmBridge.h>

typedef struct {
  LIST_ENTRY    Link;
  EFI_GUID      FeatureIdentifier;
  UINTN         DataSize;
  UINT8         *Data;   ///< Copy of the caller's payload (NULL when DataSize == 0).
} ECIT_MM_ENTRY_NODE;

STATIC LIST_ENTRY  mEntryList         = INITIALIZE_LIST_HEAD_VARIABLE (mEntryList);
STATIC UINTN       mNumberOfEntries   = 0;
STATIC BOOLEAN     mHandlerRegistered = FALSE;

/**
  MMI handler: serialize the accumulated records into the caller comm buffer.

  @param[in]     DispatchHandle  The unique handle assigned to this handler.
  @param[in]     Context         Optional handler context.
  @param[in,out] CommBuffer      MM communication buffer.
  @param[in,out] CommBufferSize  Size of CommBuffer in bytes.

  @retval EFI_SUCCESS            The request status is in CommBuffer.
  @retval EFI_INVALID_PARAMETER  The communication buffer is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
EcitMmBridgeHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer       OPTIONAL,
  IN OUT UINTN       *CommBufferSize   OPTIONAL
  )
{
  ECIT_MM_BRIDGE_COMM   *Comm;
  LIST_ENTRY            *Link;
  ECIT_MM_ENTRY_NODE    *Node;
  ECIT_MM_BRIDGE_ENTRY  *EntryHdr;
  UINT8                 *Cursor;
  UINT8                 *BufEnd;
  UINTN                 Required;
  UINT32                Count;

  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*CommBufferSize < sizeof (ECIT_MM_BRIDGE_COMM)) {
    return EFI_INVALID_PARAMETER;
  }

  Comm = (ECIT_MM_BRIDGE_COMM *)CommBuffer;
  if ((Comm->Signature != ECIT_MM_BRIDGE_COMM_SIGNATURE) ||
      (Comm->Revision != ECIT_MM_BRIDGE_COMM_REVISION))
  {
    Comm->ReturnStatus = (UINT64)EFI_INVALID_PARAMETER;
    return EFI_SUCCESS;
  }

  Required = 0;
  for (Link = GetFirstNode (&mEntryList); !IsNull (&mEntryList, Link); Link = GetNextNode (&mEntryList, Link)) {
    Node      = BASE_CR (Link, ECIT_MM_ENTRY_NODE, Link);
    Required += sizeof (ECIT_MM_BRIDGE_ENTRY) + Node->DataSize;
  }

  Comm->EntryCount  = 0;
  Comm->EntriesSize = (UINT32)Required;

  if ((sizeof (ECIT_MM_BRIDGE_COMM) + Required) > *CommBufferSize) {
    Comm->ReturnStatus = (UINT64)EFI_BUFFER_TOO_SMALL;
    return EFI_SUCCESS;
  }

  Cursor = (UINT8 *)(Comm + 1);
  BufEnd = (UINT8 *)CommBuffer + *CommBufferSize;
  Count  = 0;

  for (Link = GetFirstNode (&mEntryList); !IsNull (&mEntryList, Link); Link = GetNextNode (&mEntryList, Link)) {
    Node = BASE_CR (Link, ECIT_MM_ENTRY_NODE, Link);
    if ((Cursor + sizeof (ECIT_MM_BRIDGE_ENTRY) + Node->DataSize) > BufEnd) {
      break;
    }

    EntryHdr = (ECIT_MM_BRIDGE_ENTRY *)Cursor;
    CopyGuid (&EntryHdr->FeatureIdentifier, &Node->FeatureIdentifier);
    EntryHdr->DataSize = (UINT32)Node->DataSize;
    Cursor            += sizeof (ECIT_MM_BRIDGE_ENTRY);

    if (Node->DataSize != 0) {
      CopyMem (Cursor, Node->Data, Node->DataSize);
      Cursor += Node->DataSize;
    }

    Count++;
  }

  Comm->EntryCount   = Count;
  Comm->ReturnStatus = (UINT64)EFI_SUCCESS;
  return EFI_SUCCESS;
}

/**
  Register the drain MMI handler once, on the first accepted record.

  @retval EFI_SUCCESS  The handler is registered (or was already).
  @retval Others       MmiHandlerRegister failed.
**/
STATIC
EFI_STATUS
EcitEnsureHandler (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DispatchHandle;

  if (mHandlerRegistered) {
    return EFI_SUCCESS;
  }

  Status = gMmst->MmiHandlerRegister (
                    EcitMmBridgeHandler,
                    &gEcitMmBridgeHandlerGuid,
                    &DispatchHandle
                    );
  if (!EFI_ERROR (Status)) {
    mHandlerRegistered = TRUE;
  }

  return Status;
}

/**
  Store an ECIT feature record for collection by the DXE bridge.

  The function copies EntryData into the linking MM module. FeatureIdentifier
  must be unique among the records stored by this module.

  @param[in] FeatureIdentifier  GUID identifying the feature.
  @param[in] EntryData          Feature capability payload. May be NULL only
                                when EntryDataSize is zero.
  @param[in] EntryDataSize      Size of EntryData in bytes.

  @retval EFI_SUCCESS            The record was stored.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid or the record exceeds
                                 the format limits.
  @retval EFI_ALREADY_STARTED    FeatureIdentifier is already stored.
  @retval EFI_OUT_OF_RESOURCES   Allocation failed or the entry limit was
                                 reached.
  @retval Others                 MMI handler registration failed.
**/
EFI_STATUS
EFIAPI
EcitRegisterCryptoCapability (
  IN CONST EFI_GUID  *FeatureIdentifier,
  IN CONST VOID      *EntryData        OPTIONAL,
  IN UINTN           EntryDataSize
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Link;
  ECIT_MM_ENTRY_NODE  *Node;

  if (FeatureIdentifier == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((EntryData == NULL) && (EntryDataSize != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((sizeof (EFI_CRYPTO_INDICATOR_ENTRY) + EntryDataSize) > MAX_UINT16) {
    return EFI_INVALID_PARAMETER;
  }

  if (mNumberOfEntries >= MAX_UINT8) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Link = GetFirstNode (&mEntryList); !IsNull (&mEntryList, Link); Link = GetNextNode (&mEntryList, Link)) {
    Node = BASE_CR (Link, ECIT_MM_ENTRY_NODE, Link);
    if (CompareGuid (&Node->FeatureIdentifier, FeatureIdentifier)) {
      return EFI_ALREADY_STARTED;
    }
  }

  Status = EcitEnsureHandler ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT(MM): failed to register drain handler - %r\n", Status));
    return Status;
  }

  Node = AllocateZeroPool (sizeof (ECIT_MM_ENTRY_NODE));
  if (Node == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (EntryDataSize != 0) {
    Node->Data = AllocateCopyPool (EntryDataSize, EntryData);
    if (Node->Data == NULL) {
      FreePool (Node);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  CopyGuid (&Node->FeatureIdentifier, FeatureIdentifier);
  Node->DataSize = EntryDataSize;
  InsertTailList (&mEntryList, &Node->Link);
  mNumberOfEntries++;

  DEBUG ((DEBUG_INFO, "ECIT(MM): recorded feature %g (%u byte payload)\n", FeatureIdentifier, (UINT32)EntryDataSize));
  return EFI_SUCCESS;
}
