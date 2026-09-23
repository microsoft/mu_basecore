/** @file
  DXE instance of CryptoIndicatorRegistrationLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/CryptoIndicatorRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Protocol/CryptoIndicatorRegistration.h>

typedef struct {
  LIST_ENTRY    Link;
  EFI_GUID      FeatureIdentifier;
  UINTN         DataSize;
  UINT8         *Data;
} ECIT_PENDING_NODE;

STATIC LIST_ENTRY  mPendingList         = INITIALIZE_LIST_HEAD_VARIABLE (mPendingList);
STATIC EFI_EVENT   mNotifyEvent         = NULL;
STATIC VOID        *mNotifyRegistration = NULL;

/**
  Free a pending registration node and its copied payload.

  @param[in] Node  Pending registration node.
**/
STATIC
VOID
FreePendingNode (
  IN ECIT_PENDING_NODE  *Node
  )
{
  if (Node->Data != NULL) {
    FreePool (Node->Data);
  }

  FreePool (Node);
}

/**
  Flush every queued record to the collector, freeing the queue as it goes.

  @param[in] Registration  ECIT registration protocol.
**/
STATIC
VOID
FlushPending (
  IN EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL  *Registration
  )
{
  LIST_ENTRY         *Link;
  ECIT_PENDING_NODE  *Node;
  EFI_STATUS         Status;

  while (!IsListEmpty (&mPendingList)) {
    Link = GetFirstNode (&mPendingList);
    Node = BASE_CR (Link, ECIT_PENDING_NODE, Link);
    RemoveEntryList (Link);

    Status = Registration->RegisterEntry (
                             Registration,
                             &Node->FeatureIdentifier,
                             Node->Data,
                             Node->DataSize
                             );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "ECIT: deferred registration of %g failed - %r\n", &Node->FeatureIdentifier, Status));
    }

    FreePendingNode (Node);
  }
}

/**
  Flush queued records when the collector protocol is installed.

  @param[in] Event    Event that triggered this notification.
  @param[in] Context  Pointer to the notification context.
**/
STATIC
VOID
EFIAPI
OnCollectorInstalled (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                                  Status;
  EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL  *Registration;

  Status = gBS->LocateProtocol (
                  &gEfiCryptoIndicatorRegistrationProtocolGuid,
                  mNotifyRegistration,
                  (VOID **)&Registration
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  FlushPending (Registration);

  gBS->CloseEvent (Event);
  mNotifyEvent = NULL;
}

/**
  Queue a record and arm the collector-installation notify on first use.

  @param[in] FeatureIdentifier  GUID identifying the feature.
  @param[in] EntryData          Feature capability payload.
  @param[in] EntryDataSize      Size of EntryData in bytes.

  @retval EFI_SUCCESS           The record was queued.
  @retval EFI_OUT_OF_RESOURCES  Allocation failed.
  @retval Others                Event or protocol notification setup failed.
**/
STATIC
EFI_STATUS
QueueRecord (
  IN CONST EFI_GUID  *FeatureIdentifier,
  IN CONST VOID      *EntryData,
  IN UINTN           EntryDataSize
  )
{
  EFI_STATUS         Status;
  ECIT_PENDING_NODE  *Node;

  Node = AllocateZeroPool (sizeof (ECIT_PENDING_NODE));
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
  InsertTailList (&mPendingList, &Node->Link);

  if (mNotifyEvent == NULL) {
    Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, OnCollectorInstalled, NULL, &mNotifyEvent);
    if (EFI_ERROR (Status)) {
      mNotifyEvent = NULL;
      RemoveEntryList (&Node->Link);
      FreePendingNode (Node);
      return Status;
    }

    Status = gBS->RegisterProtocolNotify (
                    &gEfiCryptoIndicatorRegistrationProtocolGuid,
                    mNotifyEvent,
                    &mNotifyRegistration
                    );
    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (mNotifyEvent);
      mNotifyEvent        = NULL;
      mNotifyRegistration = NULL;
      RemoveEntryList (&Node->Link);
      FreePendingNode (Node);
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Submit an ECIT feature record to the collector.

  If the collector is present, the record is registered immediately. Otherwise,
  the function copies the record and queues it until the collector is installed.
  The caller may free or reuse EntryData after this function returns.

  @param[in] FeatureIdentifier  GUID identifying the feature.
  @param[in] EntryData          Feature capability payload. May be NULL only
                                when EntryDataSize is zero.
  @param[in] EntryDataSize      Size of EntryData in bytes.

  @retval EFI_SUCCESS            The record was registered or queued.
  @retval EFI_INVALID_PARAMETER  FeatureIdentifier is NULL, or EntryData is NULL
                                 with a non-zero EntryDataSize.
  @retval EFI_ACCESS_DENIED      The table has already been sealed.
  @retval EFI_ALREADY_STARTED    FeatureIdentifier is already registered.
  @retval EFI_OUT_OF_RESOURCES   Allocation failed or the entry limit was
                                 reached.
  @retval Others                 Event or protocol notification setup failed.
**/
EFI_STATUS
EFIAPI
EcitRegisterCryptoCapability (
  IN CONST EFI_GUID  *FeatureIdentifier,
  IN CONST VOID      *EntryData        OPTIONAL,
  IN UINTN           EntryDataSize
  )
{
  EFI_STATUS                                  Status;
  EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL  *Registration;

  if (FeatureIdentifier == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((EntryData == NULL) && (EntryDataSize != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (
                  &gEfiCryptoIndicatorRegistrationProtocolGuid,
                  NULL,
                  (VOID **)&Registration
                  );
  if (!EFI_ERROR (Status)) {
    return Registration->RegisterEntry (Registration, FeatureIdentifier, EntryData, EntryDataSize);
  }

  return QueueRecord (FeatureIdentifier, EntryData, EntryDataSize);
}
