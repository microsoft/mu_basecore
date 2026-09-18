/** @file
  ECIT collector driver.

  Produces the EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL so feature owners can
  submit their cryptographic-capability records during DXE, accumulates those
  records, and at ready-to-boot assembles and publishes the
  EFI_CRYPTO_INDICATOR_TABLE.

  The assembled blob carries a full ACPI SDT header (signature "ECIT", length,
  checksum, OEM identity) and is published two ways from a single
  EfiACPIReclaimMemory allocation: as an EFI_CONFIGURATION_TABLE (found by UEFI
  applications before ExitBootServices) and, when EFI_ACPI_TABLE_PROTOCOL is
  available, as a native ACPI table linked into the RSDT/XSDT (found by the OS).
  Both publication paths survive ExitBootServices because the buffer is
  allocated as EfiACPIReclaimMemory.

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
#include <Guid/CryptoIndicatorTable.h>
#include <Protocol/CryptoIndicatorRegistration.h>
#include <Protocol/AcpiTable.h>

//
// Memory type used for the published table. Allocated as EfiACPIReclaimMemory
// so both the configuration-table pointer and (when installed) the native ACPI
// table reference memory that survives ExitBootServices.
//
#define ECIT_TABLE_MEMORY_TYPE  EfiACPIReclaimMemory

//
// One accumulated, not-yet-published entry.
//
typedef struct {
  LIST_ENTRY    Link;
  EFI_GUID      FeatureIdentifier;
  UINTN         DataSize;
  UINT8         *Data;   ///< Collector-owned copy of the caller's EntryData (NULL when DataSize == 0).
} ECIT_ENTRY_NODE;

STATIC LIST_ENTRY  mEntryList        = INITIALIZE_LIST_HEAD_VARIABLE (mEntryList);
STATIC UINTN       mNumberOfEntries  = 0;
STATIC BOOLEAN     mSealed           = FALSE;
STATIC EFI_EVENT   mReadyToBootEvent = NULL;

/**
  Register one ECIT entry with the collector. See
  <Protocol/CryptoIndicatorRegistration.h>.
**/
STATIC
EFI_STATUS
EFIAPI
EcitRegisterEntry (
  IN EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL  *This,
  IN CONST EFI_GUID                              *FeatureIdentifier,
  IN CONST VOID                                  *EntryData        OPTIONAL,
  IN UINTN                                       EntryDataSize
  )
{
  LIST_ENTRY       *Link;
  ECIT_ENTRY_NODE  *Node;

  if ((This == NULL) || (FeatureIdentifier == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((EntryData == NULL) && (EntryDataSize != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Each entry's EntryLength is a UINT16, and the table's NumberOfEntries is a
  // UINT8; reject anything that would not fit.
  //
  if ((sizeof (EFI_CRYPTO_INDICATOR_ENTRY) + EntryDataSize) > MAX_UINT16) {
    return EFI_INVALID_PARAMETER;
  }

  if (mSealed) {
    return EFI_ACCESS_DENIED;
  }

  if (mNumberOfEntries >= MAX_UINT8) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // A feature GUID must be unique across the table.
  //
  for (Link = GetFirstNode (&mEntryList); !IsNull (&mEntryList, Link); Link = GetNextNode (&mEntryList, Link)) {
    Node = BASE_CR (Link, ECIT_ENTRY_NODE, Link);
    if (CompareGuid (&Node->FeatureIdentifier, FeatureIdentifier)) {
      return EFI_ALREADY_STARTED;
    }
  }

  Node = AllocateZeroPool (sizeof (ECIT_ENTRY_NODE));
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

  DEBUG ((DEBUG_INFO, "ECIT: registered feature %g (%u byte payload)\n", FeatureIdentifier, (UINT32)EntryDataSize));
  return EFI_SUCCESS;
}

STATIC EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL  mRegistration = {
  EFI_CRYPTO_INDICATOR_REGISTRATION_PROTOCOL_REVISION,
  EcitRegisterEntry
};

/**
  Publish the assembled ECIT blob as a native ACPI table, if the ACPI table
  protocol is available. Best-effort: absence of the protocol is not an error
  (the configuration table remains the fallback publication path).

  @param[in]  Table      The assembled table (with a valid ACPI SDT header).
  @param[in]  TableSize  Size of the table, in bytes.
**/
STATIC
VOID
EcitInstallAcpiTable (
  IN EFI_CRYPTO_INDICATOR_TABLE  *Table,
  IN UINTN                       TableSize
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  UINTN                    TableKey;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "ECIT: ACPI table protocol not present; config table only.\n"));
    return;
  }

  //
  // InstallAcpiTable copies the buffer into ACPI-reclaim memory, links it into
  // the RSDT/XSDT, and recomputes the ACPI checksum on its own copy.
  //
  TableKey = 0;
  Status   = AcpiTable->InstallAcpiTable (AcpiTable, Table, TableSize, &TableKey);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT: InstallAcpiTable failed - %r\n", Status));
    return;
  }

  DEBUG ((DEBUG_INFO, "ECIT: installed native ACPI table (key %u).\n", (UINT32)TableKey));
}

/**
  Assemble the accumulated entries into an EFI_CRYPTO_INDICATOR_TABLE, install it
  as an EFI_CONFIGURATION_TABLE, and (when available) as a native ACPI table.

  @retval EFI_SUCCESS           The table was assembled and published.
  @retval EFI_OUT_OF_RESOURCES  Table allocation failed.
  @retval other                 InstallConfigurationTable failed.
**/
STATIC
EFI_STATUS
EcitPublishTable (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINTN                       TotalSize;
  UINT8                       Sum;
  UINTN                       Index;
  LIST_ENTRY                  *Link;
  ECIT_ENTRY_NODE             *Node;
  EFI_CRYPTO_INDICATOR_TABLE  *Table;
  EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  UINT8                       *Cursor;
  UINT8                       *TableBytes;

  //
  // Size = header + sum of (entry header + payload) for every node.
  //
  TotalSize = sizeof (EFI_CRYPTO_INDICATOR_TABLE);
  for (Link = GetFirstNode (&mEntryList); !IsNull (&mEntryList, Link); Link = GetNextNode (&mEntryList, Link)) {
    Node       = BASE_CR (Link, ECIT_ENTRY_NODE, Link);
    TotalSize += sizeof (EFI_CRYPTO_INDICATOR_ENTRY) + Node->DataSize;
  }

  if (TotalSize > MAX_UINT32) {
    DEBUG ((DEBUG_ERROR, "ECIT: assembled table too large (%lu bytes)\n", (UINT64)TotalSize));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->AllocatePool (ECIT_TABLE_MEMORY_TYPE, TotalSize, (VOID **)&Table);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Table, TotalSize);

  //
  // ACPI SDT header (shared by the config table and any future ACPI table).
  //
  Table->Signature[0] = 'E';
  Table->Signature[1] = 'C';
  Table->Signature[2] = 'I';
  Table->Signature[3] = 'T';
  Table->Length       = (UINT32)TotalSize;
  Table->Version      = EFI_CRYPTO_INDICATOR_TABLE_VERSION;
  Table->Checksum     = 0;
  CopyMem (Table->OemId, "MSFT  ", sizeof (Table->OemId));
  CopyMem (Table->OemTableId, "ECIT    ", sizeof (Table->OemTableId));
  Table->OemRevision     = 1;
  Table->CreatorId       = SIGNATURE_32 ('M', 'S', 'F', 'T');
  Table->CreatorRevision = 1;
  Table->NumberOfEntries = (UINT8)mNumberOfEntries;

  //
  // Append every entry immediately after the header.
  //
  Cursor = (UINT8 *)Table + sizeof (EFI_CRYPTO_INDICATOR_TABLE);
  for (Link = GetFirstNode (&mEntryList); !IsNull (&mEntryList, Link); Link = GetNextNode (&mEntryList, Link)) {
    Node  = BASE_CR (Link, ECIT_ENTRY_NODE, Link);
    Entry = (EFI_CRYPTO_INDICATOR_ENTRY *)Cursor;
    CopyGuid (&Entry->FeatureIdentifier, &Node->FeatureIdentifier);
    Entry->EntryLength = (UINT16)(sizeof (EFI_CRYPTO_INDICATOR_ENTRY) + Node->DataSize);
    if (Node->DataSize != 0) {
      CopyMem (Cursor + sizeof (EFI_CRYPTO_INDICATOR_ENTRY), Node->Data, Node->DataSize);
    }

    Cursor += Entry->EntryLength;
  }

  //
  // 8-bit checksum: the whole table must sum to zero.
  //
  TableBytes = (UINT8 *)Table;
  Sum        = 0;
  for (Index = 0; Index < TotalSize; Index++) {
    Sum = (UINT8)(Sum + TableBytes[Index]);
  }

  Table->Checksum = (UINT8)(0x100 - Sum);

  Status = gBS->InstallConfigurationTable (&gEfiCryptoIndicatorTableGuid, Table);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT: InstallConfigurationTable failed - %r\n", Status));
    gBS->FreePool (Table);
    return Status;
  }

  //
  // Also publish as a native ACPI table so the OS can find it via the RSDT/XSDT.
  // Best-effort: the configuration table above is the fallback when no ACPI
  // table protocol is present.
  //
  EcitInstallAcpiTable (Table, TotalSize);

  DEBUG ((
    DEBUG_INFO,
    "ECIT: published table with %u entr%a (%u bytes)\n",
    (UINT32)mNumberOfEntries,
    (mNumberOfEntries == 1) ? "y" : "ies",
    (UINT32)TotalSize
    ));
  return EFI_SUCCESS;
}

/**
  Free the accumulated entry nodes (the published table owns its own copy).
**/
STATIC
VOID
EcitFreeEntries (
  VOID
  )
{
  LIST_ENTRY       *Link;
  ECIT_ENTRY_NODE  *Node;

  while (!IsListEmpty (&mEntryList)) {
    Link = GetFirstNode (&mEntryList);
    Node = BASE_CR (Link, ECIT_ENTRY_NODE, Link);
    RemoveEntryList (Link);
    if (Node->Data != NULL) {
      FreePool (Node->Data);
    }

    FreePool (Node);
  }
}

/**
  Ready-to-boot callback: seal registration, assemble, and publish the ECIT.
**/
STATIC
VOID
EFIAPI
EcitOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if (mSealed) {
    return;
  }

  //
  // Seal first so any late RegisterEntry() (e.g. re-entrancy) is rejected.
  //
  mSealed = TRUE;

  EcitPublishTable ();
  EcitFreeEntries ();

  gBS->CloseEvent (Event);
  mReadyToBootEvent = NULL;
}

/**
  Driver entry point: install the registration protocol and arm the
  ready-to-boot publication callback.

  @param[in] ImageHandle  The image handle.
  @param[in] SystemTable  The system table.

  @retval EFI_SUCCESS  The collector is ready to accept registrations.
  @retval other        Protocol installation or event creation failed.
**/
EFI_STATUS
EFIAPI
CryptoIndicatorTableDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiCryptoIndicatorRegistrationProtocolGuid,
                  &mRegistration,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT: failed to install registration protocol - %r\n", Status));
    return Status;
  }

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             EcitOnReadyToBoot,
             NULL,
             &mReadyToBootEvent
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT: failed to create ready-to-boot event - %r\n", Status));
    gBS->UninstallMultipleProtocolInterfaces (
           Handle,
           &gEfiCryptoIndicatorRegistrationProtocolGuid,
           &mRegistration,
           NULL
           );
    return Status;
  }

  DEBUG ((DEBUG_INFO, "ECIT: collector ready.\n"));
  return EFI_SUCCESS;
}
