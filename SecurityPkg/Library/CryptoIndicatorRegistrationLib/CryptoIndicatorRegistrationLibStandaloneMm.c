/** @file
  Standalone MM instance of CryptoIndicatorRegistrationLib.

  On this platform the only Standalone MM feature that reports cryptographic
  capabilities to the ECIT is authenticated-variable servicing, so the MM side
  does not need a general registration protocol or a separate collector driver.
  This instance accumulates the linking module's records in MM memory and, on
  first use, registers an MMI handler that serializes them to the DXE bridge on
  request. The DXE bridge drains the records into the DXE collector, which owns
  ECIT publication.

  Because the records live in the linking module and are served by a single MMI
  handler keyed by gEcitMmBridgeHandlerGuid, exactly one MM module per firmware
  volume may link this instance. If a second MM feature ever needs to report,
  restore the shared MM collector (see the ecit-complex-mm-architecture backup
  branch).

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
#include <Guid/CryptoIndicatorMmBridge.h>

//
// One accumulated record, awaiting a drain by the DXE bridge.
//
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

  CommBuffer points at an ECIT_MM_BRIDGE_COMM the DXE bridge supplied; on return
  it carries ReturnStatus, EntryCount, EntriesSize, and EntriesSize bytes of
  packed ECIT_MM_BRIDGE_ENTRY records. If the buffer cannot hold the records the
  handler reports EFI_BUFFER_TOO_SMALL with the required EntriesSize.
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

  //
  // Compute the required space for all records.
  //
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
  @retval other        MmiHandlerRegister failed.
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
  Submit an ECIT feature record. See
  <Library/CryptoIndicatorRegistrationLib.h>.
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

  //
  // Each drained entry's DataSize is a UINT32 but the DXE table caps an entry at
  // MAX_UINT16 and NumberOfEntries at MAX_UINT8; reject anything that would not
  // fit downstream.
  //
  if ((sizeof (ECIT_MM_BRIDGE_ENTRY) + EntryDataSize) > MAX_UINT16) {
    return EFI_INVALID_PARAMETER;
  }

  if (mNumberOfEntries >= MAX_UINT8) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // A feature GUID must be unique across the table.
  //
  for (Link = GetFirstNode (&mEntryList); !IsNull (&mEntryList, Link); Link = GetNextNode (&mEntryList, Link)) {
    Node = BASE_CR (Link, ECIT_MM_ENTRY_NODE, Link);
    if (CompareGuid (&Node->FeatureIdentifier, FeatureIdentifier)) {
      return EFI_ALREADY_STARTED;
    }
  }

  //
  // Ensure the drain handler is live before we hold a record the bridge must be
  // able to reach.
  //
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
