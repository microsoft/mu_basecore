/** @file
  Bridges ECIT capability records from Standalone MM to DXE.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/CryptoIndicatorRegistrationLib.h>
#include <Guid/CryptoIndicatorMmBridge.h>
#include <Protocol/MmCommunication2.h>

//
// Initial payload capacity. The MM handler reports the required size when a
// larger buffer is needed.
//
#define ECIT_BRIDGE_DEFAULT_DATA_SIZE  (4 * 1024)
#define ECIT_BRIDGE_MAX_DATA_SIZE      (MAX_UINT8 * MAX_UINT16)

/**
  Issue one drain request to the MM collector with DataCapacity payload bytes.

  On success the allocated communicate buffer (caller frees) carries the MM
  handler's ECIT_MM_BRIDGE_COMM result. The function result reports the MM
  transport status; the MM handler's own status is in Comm->ReturnStatus.

  @param[in]  MmComm        MM communication protocol.
  @param[in]  DataCapacity  Payload bytes reserved after the ECIT_MM_BRIDGE_COMM.
  @param[out] CommHeader    Allocated communicate buffer on success.

  @retval EFI_SUCCESS           The communicate completed (inspect ReturnStatus).
  @retval EFI_OUT_OF_RESOURCES  Allocation failed.
  @retval Others                The MM communicate failed.
**/
STATIC
EFI_STATUS
EcitBridgeCommunicate (
  IN  EFI_MM_COMMUNICATION2_PROTOCOL  *MmComm,
  IN  UINTN                           DataCapacity,
  OUT EFI_MM_COMMUNICATE_HEADER       **CommHeader
  )
{
  EFI_STATUS                 Status;
  EFI_MM_COMMUNICATE_HEADER  *Header;
  ECIT_MM_BRIDGE_COMM        *Comm;
  UINTN                      MessageLength;
  UINTN                      BufferSize;
  UINTN                      CommSize;

  MessageLength = sizeof (ECIT_MM_BRIDGE_COMM) + DataCapacity;
  BufferSize    = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + MessageLength;

  Header = AllocateZeroPool (BufferSize);
  if (Header == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyGuid (&Header->HeaderGuid, &gEcitMmBridgeHandlerGuid);
  Header->MessageLength = MessageLength;

  Comm            = (ECIT_MM_BRIDGE_COMM *)Header->Data;
  Comm->Signature = ECIT_MM_BRIDGE_COMM_SIGNATURE;
  Comm->Revision  = ECIT_MM_BRIDGE_COMM_REVISION;

  CommSize = BufferSize;
  Status   = MmComm->Communicate (MmComm, Header, Header, &CommSize);
  if (EFI_ERROR (Status)) {
    FreePool (Header);
    return Status;
  }

  *CommHeader = Header;
  return EFI_SUCCESS;
}

/**
  Register the records returned by the MM bridge.

  @param[in] Comm          MM bridge response.
  @param[in] DataCapacity  Number of bytes available after Comm.

  @retval EFI_SUCCESS           All records were registered.
  @retval EFI_COMPROMISED_DATA  The serialized record data is malformed.
  @retval Others                Registration of one or more records failed.
**/
STATIC
EFI_STATUS
EcitBridgeRegisterDrained (
  IN ECIT_MM_BRIDGE_COMM  *Comm,
  IN UINTN                DataCapacity
  )
{
  EFI_STATUS            FirstError;
  EFI_STATUS            Status;
  ECIT_MM_BRIDGE_ENTRY  *EntryHdr;
  UINT8                 *Cursor;
  UINTN                 Remaining;
  UINT32                Index;
  UINT32                Registered;

  if ((Comm->EntryCount > MAX_UINT8) || (Comm->EntriesSize > DataCapacity)) {
    return EFI_COMPROMISED_DATA;
  }

  Cursor     = (UINT8 *)(Comm + 1);
  Remaining  = Comm->EntriesSize;
  Registered = 0;
  FirstError = EFI_SUCCESS;

  for (Index = 0; Index < Comm->EntryCount; Index++) {
    if (Remaining < sizeof (ECIT_MM_BRIDGE_ENTRY)) {
      return EFI_COMPROMISED_DATA;
    }

    EntryHdr   = (ECIT_MM_BRIDGE_ENTRY *)Cursor;
    Cursor    += sizeof (ECIT_MM_BRIDGE_ENTRY);
    Remaining -= sizeof (ECIT_MM_BRIDGE_ENTRY);
    if (EntryHdr->DataSize > Remaining) {
      return EFI_COMPROMISED_DATA;
    }

    Status = EcitRegisterCryptoCapability (
               &EntryHdr->FeatureIdentifier,
               (EntryHdr->DataSize != 0) ? Cursor : NULL,
               EntryHdr->DataSize
               );
    DEBUG ((DEBUG_INFO, "ECIT: bridged MM feature %g (%u bytes) - %r\n", &EntryHdr->FeatureIdentifier, EntryHdr->DataSize, Status));
    if (EFI_ERROR (Status)) {
      if (!EFI_ERROR (FirstError)) {
        FirstError = Status;
      }
    } else {
      Registered++;
    }

    Cursor    += EntryHdr->DataSize;
    Remaining -= EntryHdr->DataSize;
  }

  if (Remaining != 0) {
    return EFI_COMPROMISED_DATA;
  }

  DEBUG ((DEBUG_INFO, "ECIT: registered %u of %u MM record(s).\n", Registered, Comm->EntryCount));
  return FirstError;
}

/**
  Fetch the MM collector records and register each with the DXE collector.

  A first request uses a modest payload capacity; if the MM collector needs more
  room it reports the required size and the bridge retries once with an exactly
  sized buffer, so no record is silently dropped.
**/
STATIC
VOID
EcitDrainMm (
  VOID
  )
{
  EFI_STATUS                      Status;
  EFI_MM_COMMUNICATION2_PROTOCOL  *MmComm;
  EFI_MM_COMMUNICATE_HEADER       *CommHeader;
  ECIT_MM_BRIDGE_COMM             *Comm;
  UINTN                           DataCapacity;
  UINTN                           Attempt;

  Status = gBS->LocateProtocol (&gEfiMmCommunication2ProtocolGuid, NULL, (VOID **)&MmComm);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "ECIT: MM communication unavailable (%r); nothing to drain.\n", Status));
    return;
  }

  DataCapacity = ECIT_BRIDGE_DEFAULT_DATA_SIZE;

  for (Attempt = 0; Attempt < 2; Attempt++) {
    CommHeader = NULL;
    Status     = EcitBridgeCommunicate (MmComm, DataCapacity, &CommHeader);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "ECIT: MM communicate failed - %r\n", Status));
      return;
    }

    Comm = (ECIT_MM_BRIDGE_COMM *)CommHeader->Data;

    if ((Comm->Signature != ECIT_MM_BRIDGE_COMM_SIGNATURE) ||
        (Comm->Revision != ECIT_MM_BRIDGE_COMM_REVISION))
    {
      DEBUG ((DEBUG_ERROR, "ECIT: MM handler returned an invalid response header.\n"));
      FreePool (CommHeader);
      return;
    }

    if (((EFI_STATUS)Comm->ReturnStatus == EFI_BUFFER_TOO_SMALL) && (Attempt == 0)) {
      if ((Comm->EntriesSize <= DataCapacity) || (Comm->EntriesSize > ECIT_BRIDGE_MAX_DATA_SIZE)) {
        DEBUG ((DEBUG_ERROR, "ECIT: MM handler returned an invalid required size (%u).\n", Comm->EntriesSize));
        FreePool (CommHeader);
        return;
      }

      DataCapacity = Comm->EntriesSize;
      FreePool (CommHeader);
      continue;
    }

    if ((EFI_STATUS)Comm->ReturnStatus != EFI_SUCCESS) {
      DEBUG ((DEBUG_WARN, "ECIT: MM handler returned - %r (EntriesSize %u)\n", (EFI_STATUS)Comm->ReturnStatus, Comm->EntriesSize));
      FreePool (CommHeader);
      return;
    }

    Status = EcitBridgeRegisterDrained (Comm, DataCapacity);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ECIT: failed to register MM records - %r\n", Status));
    }

    FreePool (CommHeader);
    return;
  }
}

/**
  Drains the MM records before the DXE collector seals the table.

  @param[in] Event    Event that triggered this notification.
  @param[in] Context  Pointer to the notification context.
**/
STATIC
VOID
EFIAPI
EcitBridgeOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EcitDrainMm ();
  gBS->CloseEvent (Event);
}

/**
  Creates the event that drains MM records at ready-to-boot.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The ready-to-boot event was created.
  @retval Others       The event could not be created.
**/
EFI_STATUS
EFIAPI
CryptoIndicatorBridgeDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  //
  // TPL_NOTIFY ensures this callback runs before the collector's TPL_CALLBACK
  // callback seals the table.
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_NOTIFY,
             EcitBridgeOnReadyToBoot,
             NULL,
             &Event
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT: bridge failed to create ready-to-boot event - %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "ECIT: MM->DXE bridge armed.\n"));
  return EFI_SUCCESS;
}
