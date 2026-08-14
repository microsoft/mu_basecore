/** @file
  DXE instance of CryptoIndicatorRegistrationLib.

  Submits ECIT feature records to the collector's registration protocol. If the
  collector is not yet available, records are queued and flushed automatically
  via a protocol-installation notify, so contributors do not need to order
  themselves after the collector.

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
  Flush every queued record to the collector, freeing the queue as it goes.
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

    if (Node->Data != NULL) {
      FreePool (Node->Data);
    }

    FreePool (Node);
  }
}

/**
  Protocol-notify callback: the collector has arrived; flush the queue.
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
      return Status;
    }

    Status = gBS->RegisterProtocolNotify (
                    &gEfiCryptoIndicatorRegistrationProtocolGuid,
                    mNotifyEvent,
                    &mNotifyRegistration
                    );
    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (mNotifyEvent);
      mNotifyEvent = NULL;
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Submit an ECIT feature record to the collector. See
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

  //
  // Collector not present yet: queue and flush when it installs.
  //
  return QueueRecord (FeatureIdentifier, EntryData, EntryDataSize);
}
