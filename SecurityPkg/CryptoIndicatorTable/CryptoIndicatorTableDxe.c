/** @file
  Collects and publishes EFI Crypto Indicator Table records.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Guid/CryptoIndicatorTable.h>
#include <Guid/EcitCapabilityHob.h>
#include <Protocol/CryptoIndicatorRegistration.h>
#include <Protocol/AcpiTable.h>

//
// Keep the configuration table available after ExitBootServices.
//
#define ECIT_TABLE_MEMORY_TYPE  EfiACPIReclaimMemory
#define ECIT_ENTRY_ALIGNMENT    8

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

STATIC
EFI_STATUS
GetAlignedEntryLength (
  IN  UINTN  EntryDataSize,
  OUT UINTN  *EntryLength
  )
{
  UINTN  UnalignedLength;

  if (EntryLength == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (EntryDataSize > (MAX_UINT16 - sizeof (EFI_CRYPTO_INDICATOR_ENTRY))) {
    return EFI_INVALID_PARAMETER;
  }

  UnalignedLength = sizeof (EFI_CRYPTO_INDICATOR_ENTRY) + EntryDataSize;
  if (UnalignedLength > (MAX_UINT16 - (ECIT_ENTRY_ALIGNMENT - 1))) {
    return EFI_INVALID_PARAMETER;
  }

  *EntryLength = ALIGN_VALUE (UnalignedLength, ECIT_ENTRY_ALIGNMENT);
  return EFI_SUCCESS;
}

/**
  Register an ECIT capability record with the collector.

  The collector copies EntryData. FeatureIdentifier must be unique within the
  table.

  @param[in] This               Pointer to the registration protocol.
  @param[in] FeatureIdentifier  GUID identifying the feature.
  @param[in] EntryData          Feature capability payload. May be NULL only
                                when EntryDataSize is zero.
  @param[in] EntryDataSize      Size of EntryData in bytes.

  @retval EFI_SUCCESS            The record was registered.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid or the record exceeds
                                 the format limits.
  @retval EFI_ACCESS_DENIED      The table has already been sealed.
  @retval EFI_ALREADY_STARTED    FeatureIdentifier is already registered.
  @retval EFI_OUT_OF_RESOURCES   Allocation failed or the entry limit was
                                 reached.
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
  UINTN            EntryLength;

  if ((This == NULL) || (FeatureIdentifier == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((EntryData == NULL) && (EntryDataSize != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Each aligned entry length is a UINT16, and the table's NumberOfEntries is
  // a UINT8; reject anything that would not fit.
  //
  if (EFI_ERROR (GetAlignedEntryLength (EntryDataSize, &EntryLength))) {
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
  Import ECIT capability records produced in PEI.

  @retval EFI_SUCCESS            All PEI capability HOBs were imported.
  @retval EFI_COMPROMISED_DATA   A capability HOB is malformed.
  @retval EFI_UNSUPPORTED        A capability HOB revision is unsupported.
  @retval EFI_INVALID_PARAMETER  A capability HOB contains an invalid feature.
  @retval Others                 Registering a capability record failed.
**/
STATIC
EFI_STATUS
EcitImportPeiHobs (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_HOB_GUID_TYPE          *GuidHob;
  EDKII_ECIT_CAPABILITY_HOB  *CapabilityHob;
  UINTN                      HobDataSize;
  UINTN                      ExpectedHobDataSize;
  UINTN                      EntryDataSize;
  CONST VOID                 *EntryData;

  GuidHob = GetFirstGuidHob (&gEdkiiEcitCapabilityHobGuid);
  while (GuidHob != NULL) {
    HobDataSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
    if (HobDataSize < sizeof (EDKII_ECIT_CAPABILITY_HOB)) {
      DEBUG ((DEBUG_ERROR, "ECIT: PEI capability HOB is too small (%u bytes)\n", (UINT32)HobDataSize));
      return EFI_COMPROMISED_DATA;
    }

    CapabilityHob = GET_GUID_HOB_DATA (GuidHob);
    if (CapabilityHob->Revision != EDKII_ECIT_CAPABILITY_HOB_REVISION) {
      DEBUG ((
        DEBUG_ERROR,
        "ECIT: unsupported PEI capability HOB revision %u\n",
        CapabilityHob->Revision
        ));
      return EFI_UNSUPPORTED;
    }

    if ((CapabilityHob->HeaderSize < sizeof (EDKII_ECIT_CAPABILITY_HOB)) ||
        (CapabilityHob->HeaderSize > HobDataSize))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ECIT: invalid PEI capability HOB header size %u (HOB data %u)\n",
        CapabilityHob->HeaderSize,
        (UINT32)HobDataSize
        ));
      return EFI_COMPROMISED_DATA;
    }

    if (CapabilityHob->EntryDataSize > (HobDataSize - CapabilityHob->HeaderSize)) {
      DEBUG ((
        DEBUG_ERROR,
        "ECIT: PEI capability HOB payload size %u exceeds %u\n",
        CapabilityHob->EntryDataSize,
        (UINT32)(HobDataSize - CapabilityHob->HeaderSize)
        ));
      return EFI_COMPROMISED_DATA;
    }

    EntryDataSize       = CapabilityHob->EntryDataSize;
    ExpectedHobDataSize = ALIGN_VALUE (CapabilityHob->HeaderSize + EntryDataSize, 8);
    if (HobDataSize != ExpectedHobDataSize) {
      DEBUG ((
        DEBUG_ERROR,
        "ECIT: PEI capability HOB size %u does not match aligned size %u\n",
        (UINT32)HobDataSize,
        (UINT32)ExpectedHobDataSize
        ));
      return EFI_COMPROMISED_DATA;
    }

    if (IsZeroGuid (&CapabilityHob->FeatureIdentifier)) {
      DEBUG ((DEBUG_ERROR, "ECIT: PEI capability HOB has a zero feature identifier\n"));
      return EFI_INVALID_PARAMETER;
    }

    EntryData = (CONST UINT8 *)CapabilityHob + CapabilityHob->HeaderSize;
    Status    = EcitRegisterEntry (
                  &mRegistration,
                  &CapabilityHob->FeatureIdentifier,
                  EntryData,
                  EntryDataSize
                  );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ECIT: failed to import PEI feature %g - %r\n",
        &CapabilityHob->FeatureIdentifier,
        Status
        ));
      return Status;
    }

    GuidHob = GetNextGuidHob (
                &gEdkiiEcitCapabilityHobGuid,
                GET_NEXT_HOB (GuidHob)
                );
  }

  return EFI_SUCCESS;
}

/**
  Publish ECIT as a native ACPI table when ACPI support is available.

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
  // InstallAcpiTable copies the table and adds the copy to the RSDT/XSDT.
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
  Assemble and publish the EFI Crypto Indicator Table.

  @retval EFI_SUCCESS           The table was assembled and published.
  @retval EFI_OUT_OF_RESOURCES  Table allocation failed.
  @retval Others                InstallConfigurationTable failed.
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
  UINT64                      OemTableId;
  UINTN                       EntryLength;

  TotalSize = sizeof (EFI_CRYPTO_INDICATOR_TABLE);
  for (Link = GetFirstNode (&mEntryList); !IsNull (&mEntryList, Link); Link = GetNextNode (&mEntryList, Link)) {
    Node   = BASE_CR (Link, ECIT_ENTRY_NODE, Link);
    Status = GetAlignedEntryLength (Node->DataSize, &EntryLength);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status) || (TotalSize > (MAX_UINTN - EntryLength))) {
      return EFI_OUT_OF_RESOURCES;
    }

    TotalSize += EntryLength;
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
  // Populate the common ACPI SDT header fields.
  //
  Table->Signature[0] = 'E';
  Table->Signature[1] = 'C';
  Table->Signature[2] = 'I';
  Table->Signature[3] = 'T';
  Table->Length       = (UINT32)TotalSize;
  Table->Version      = EFI_CRYPTO_INDICATOR_TABLE_VERSION;
  Table->Checksum     = 0;
  CopyMem (Table->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (Table->OemId));
  OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (Table->OemTableId, &OemTableId, sizeof (Table->OemTableId));
  Table->OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  Table->CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  Table->CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  Table->NumberOfEntries = (UINT8)mNumberOfEntries;

  Cursor = (UINT8 *)Table + sizeof (EFI_CRYPTO_INDICATOR_TABLE);
  for (Link = GetFirstNode (&mEntryList); !IsNull (&mEntryList, Link); Link = GetNextNode (&mEntryList, Link)) {
    Node  = BASE_CR (Link, ECIT_ENTRY_NODE, Link);
    Entry = (EFI_CRYPTO_INDICATOR_ENTRY *)Cursor;
    CopyGuid (&Entry->FeatureIdentifier, &Node->FeatureIdentifier);
    Status = GetAlignedEntryLength (Node->DataSize, &EntryLength);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      gBS->FreePool (Table);
      return EFI_COMPROMISED_DATA;
    }

    Entry->EntryLength = (UINT16)EntryLength;
    if (Node->DataSize != 0) {
      CopyMem (Cursor + sizeof (EFI_CRYPTO_INDICATOR_ENTRY), Node->Data, Node->DataSize);
    }

    Cursor += EntryLength;
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
  Seal registration and publish ECIT.

  @param[in] Event    Event that triggered this notification.
  @param[in] Context  Pointer to the notification context.
**/
STATIC
VOID
EFIAPI
EcitOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  if (mSealed) {
    return;
  }

  //
  // Seal first so any late RegisterEntry() (e.g. re-entrancy) is rejected.
  //
  mSealed = TRUE;

  Status = EcitPublishTable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT: failed to publish table - %r\n", Status));
  }

  EcitFreeEntries ();

  gBS->CloseEvent (Event);
  mReadyToBootEvent = NULL;
}

/**
  Install the registration protocol and create the publication event.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The collector is ready to accept registrations.
  @retval Others       Protocol installation or event creation failed.
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

  Status = EcitImportPeiHobs ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT: failed to import PEI capability HOBs - %r\n", Status));
    EcitFreeEntries ();
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
    EcitFreeEntries ();
    return Status;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiCryptoIndicatorRegistrationProtocolGuid,
                  &mRegistration,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT: failed to install registration protocol - %r\n", Status));
    gBS->CloseEvent (mReadyToBootEvent);
    mReadyToBootEvent = NULL;
    EcitFreeEntries ();
    return Status;
  }

  DEBUG ((DEBUG_INFO, "ECIT: collector ready.\n"));
  return EFI_SUCCESS;
}
