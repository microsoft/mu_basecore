/** @file
  ECIT MM->DXE bridge (DXE).

  Drains the Standalone MM side's accumulated ECIT records into the DXE
  collector before the table is sealed. Feature owners whose cryptographic work
  runs in MM (for example authenticated-variable servicing) record with the
  MM-side CryptoIndicatorRegistrationLib, which serves them from a single MMI
  handler; this driver fetches those records via MM communication and submits
  them to the DXE collector through CryptoIndicatorRegistrationLib.

  The drain runs on a ready-to-boot notify at TPL_NOTIFY, which executes before
  the DXE collector's TPL_CALLBACK seal callback, so the drained records are
  accepted into the published table.

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
// Default comm-buffer payload capacity for the first drain attempt. The ECIT is
// small (NumberOfEntries is a UINT8, each entry <= MAX_UINT16), so one pass
// almost always suffices; if the MM collector needs more, its handler reports
// the required size and the bridge retries once with an exactly sized buffer.
//
#define ECIT_BRIDGE_DEFAULT_DATA_SIZE  (4 * 1024)

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
  @retval other                 The MM communicate failed.
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
  Register every record carried in a drained communicate buffer with the DXE
  collector, bounding each record against the reported EntriesSize.
**/
STATIC
VOID
EcitBridgeRegisterDrained (
  IN ECIT_MM_BRIDGE_COMM  *Comm
  )
{
  EFI_STATUS            Status;
  ECIT_MM_BRIDGE_ENTRY  *EntryHdr;
  UINT8                 *Cursor;
  UINT8                 *End;
  UINT32                Index;

  Cursor = (UINT8 *)(Comm + 1);
  End    = Cursor + Comm->EntriesSize;

  for (Index = 0; Index < Comm->EntryCount; Index++) {
    if ((Cursor + sizeof (ECIT_MM_BRIDGE_ENTRY)) > End) {
      break;
    }

    EntryHdr = (ECIT_MM_BRIDGE_ENTRY *)Cursor;
    Cursor  += sizeof (ECIT_MM_BRIDGE_ENTRY);
    if ((Cursor + EntryHdr->DataSize) > End) {
      break;
    }

    Status = EcitRegisterCryptoCapability (
               &EntryHdr->FeatureIdentifier,
               (EntryHdr->DataSize != 0) ? Cursor : NULL,
               EntryHdr->DataSize
               );
    DEBUG ((DEBUG_INFO, "ECIT: bridged MM feature %g (%u bytes) - %r\n", &EntryHdr->FeatureIdentifier, EntryHdr->DataSize, Status));

    Cursor += EntryHdr->DataSize;
  }

  DEBUG ((DEBUG_INFO, "ECIT: drained %u MM record(s).\n", Comm->EntryCount));
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

    if (((EFI_STATUS)Comm->ReturnStatus == EFI_BUFFER_TOO_SMALL) && (Attempt == 0)) {
      //
      // The MM collector needs more room than the default; retry once with the
      // exact size it reported.
      //
      DataCapacity = Comm->EntriesSize;
      FreePool (CommHeader);
      continue;
    }

    if ((EFI_STATUS)Comm->ReturnStatus != EFI_SUCCESS) {
      DEBUG ((DEBUG_WARN, "ECIT: MM handler returned - %r (EntriesSize %u)\n", (EFI_STATUS)Comm->ReturnStatus, Comm->EntriesSize));
      FreePool (CommHeader);
      return;
    }

    EcitBridgeRegisterDrained (Comm);
    FreePool (CommHeader);
    return;
  }
}

/**
  Ready-to-boot notify (TPL_NOTIFY): drain MM before the DXE collector seals.
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
  Driver entry point: arm the ready-to-boot drain.
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
