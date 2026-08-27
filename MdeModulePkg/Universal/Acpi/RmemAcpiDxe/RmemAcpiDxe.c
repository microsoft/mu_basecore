/** @file
  Publishes the Reserved-Memory Reporting (RMEM) ACPI table.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

#include <Pi/PiHob.h>

#include <Guid/EventGroup.h>
#include <Guid/ReservedMemoryReportingHob.h>
#include <IndustryStandard/ReservedMemoryReportingTable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/ReservedMemoryReporting.h>

#define RMEM_MAX_ENTRIES  64

STATIC RMEM_ENTRY  mEntries[RMEM_MAX_ENTRIES];
STATIC UINT32      mEntryCount;
STATIC BOOLEAN     mFinalized;
STATIC BOOLEAN     mRegistrationFailed;
STATIC EFI_EVENT   mPublicationEvent;

STATIC
BOOLEAN
RmemRangeIsValid (
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN UINT64                Size
  )
{
  return (Size != 0) && (Base <= (MAX_UINT64 - (Size - 1)));
}

STATIC
BOOLEAN
RmemRangesOverlap (
  IN EFI_PHYSICAL_ADDRESS  FirstBase,
  IN UINT64                FirstSize,
  IN EFI_PHYSICAL_ADDRESS  SecondBase,
  IN UINT64                SecondSize
  )
{
  return (FirstBase <= (SecondBase + SecondSize - 1)) &&
         (SecondBase <= (FirstBase + FirstSize - 1));
}

STATIC
BOOLEAN
RmemEntriesAreIdentical (
  IN CONST RMEM_ENTRY  *Existing,
  IN UINT64            Base,
  IN UINT64            Size,
  IN RMEM_CATEGORY     Category,
  IN CONST CHAR8       *Label
  )
{
  return (Existing->Base == Base) &&
         (Existing->Size == Size) &&
         (Existing->Category == (UINT32)Category) &&
         (AsciiStrCmp (Existing->Label, Label) == 0);
}

STATIC
EFI_STATUS
EFIAPI
RmemAddReservedRange (
  IN EDKII_RMEM_REGISTRATION_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS              Base,
  IN UINT64                            Size,
  IN RMEM_CATEGORY                     Category,
  IN CONST CHAR8                       *Label OPTIONAL
  )
{
  CONST CHAR8  *EffectiveLabel;
  UINTN        LabelLength;
  UINT32       Index;

  if ((This == NULL) ||
      (This->Revision != EDKII_RMEM_REGISTRATION_PROTOCOL_REVISION))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (mFinalized) {
    return EFI_ACCESS_DENIED;
  }

  if (!RmemRangeIsValid (Base, Size) ||
      ((UINT32)Category <= (UINT32)RmemCategoryUnknown) ||
      ((UINT32)Category >= (UINT32)RmemCategoryMax))
  {
    mRegistrationFailed = TRUE;
    return EFI_INVALID_PARAMETER;
  }

  EffectiveLabel = (Label == NULL) ? "" : Label;
  LabelLength    = AsciiStrnLenS (EffectiveLabel, RMEM_LABEL_MAX_LEN);
  if (LabelLength >= RMEM_LABEL_MAX_LEN) {
    mRegistrationFailed = TRUE;
    return EFI_BAD_BUFFER_SIZE;
  }

  for (Index = 0; Index < mEntryCount; Index++) {
    if (RmemEntriesAreIdentical (
          &mEntries[Index],
          Base,
          Size,
          Category,
          EffectiveLabel
          ))
    {
      return EFI_ALREADY_STARTED;
    }

    if (RmemRangesOverlap (
          mEntries[Index].Base,
          mEntries[Index].Size,
          Base,
          Size
          ))
    {
      mRegistrationFailed = TRUE;
      return EFI_ACCESS_DENIED;
    }
  }

  if (mEntryCount >= RMEM_MAX_ENTRIES) {
    mRegistrationFailed = TRUE;
    return EFI_OUT_OF_RESOURCES;
  }

  mEntries[mEntryCount].Base     = Base;
  mEntries[mEntryCount].Size     = Size;
  mEntries[mEntryCount].Category = (UINT32)Category;
  CopyMem (
    mEntries[mEntryCount].Label,
    EffectiveLabel,
    LabelLength + 1
    );
  mEntryCount++;

  return EFI_SUCCESS;
}

STATIC EDKII_RMEM_REGISTRATION_PROTOCOL  mRmemProtocol = {
  EDKII_RMEM_REGISTRATION_PROTOCOL_REVISION,
  RmemAddReservedRange
};

STATIC
EFI_STATUS
RmemImportHobs (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  RMEM_HOB_RECORD    *Record;
  EFI_STATUS         Status;

  GuidHob = GetFirstGuidHob (&gEdkiiRmemRecordHobGuid);
  while (GuidHob != NULL) {
    if (GET_GUID_HOB_DATA_SIZE (GuidHob) != sizeof (RMEM_HOB_RECORD)) {
      return EFI_COMPROMISED_DATA;
    }

    Record = (RMEM_HOB_RECORD *)GET_GUID_HOB_DATA (GuidHob);
    if (Record->Revision != RMEM_HOB_REVISION) {
      return EFI_INCOMPATIBLE_VERSION;
    }

    if ((Record->Reserved != 0) || (Record->Reserved2 != 0)) {
      return EFI_COMPROMISED_DATA;
    }

    Status = RmemAddReservedRange (
               &mRmemProtocol,
               Record->Base,
               Record->Size,
               (RMEM_CATEGORY)Record->Category,
               Record->Label
               );
    if ((Status != EFI_ALREADY_STARTED) && EFI_ERROR (Status)) {
      return Status;
    }

    GuidHob = GetNextGuidHob (
                &gEdkiiRmemRecordHobGuid,
                GET_NEXT_HOB (GuidHob)
                );
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
RmemPublishTable (
  VOID
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;
  RMEM_TABLE_HEADER        *Table;
  EFI_STATUS               Status;
  UINTN                    TableKey;
  UINTN                    TableSize;

  if (mRegistrationFailed) {
    return EFI_COMPROMISED_DATA;
  }

  if (mEntryCount == 0) {
    return EFI_NOT_FOUND;
  }

  TableSize = sizeof (RMEM_TABLE_HEADER) +
              ((UINTN)mEntryCount * sizeof (RMEM_ENTRY));
  Table = AllocateZeroPool (TableSize);
  if (Table == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Table->Header.Signature = RMEM_TABLE_SIGNATURE;
  Table->Header.Length    = (UINT32)TableSize;
  Table->Header.Revision  = RMEM_TABLE_REVISION;
  CopyMem (
    Table->Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    MIN (PcdGetSize (PcdAcpiDefaultOemId), sizeof (Table->Header.OemId))
    );
  WriteUnaligned64 (
    &Table->Header.OemTableId,
    PcdGet64 (PcdAcpiDefaultOemTableId)
    );
  Table->Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  Table->Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  Table->Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  Table->EntryCount             = mEntryCount;

  CopyMem (
    (UINT8 *)Table + sizeof (RMEM_TABLE_HEADER),
    mEntries,
    (UINTN)mEntryCount * sizeof (RMEM_ENTRY)
    );
  Table->Header.Checksum = CalculateCheckSum8 ((UINT8 *)Table, TableSize);

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (!EFI_ERROR (Status)) {
    Status = AcpiTableProtocol->InstallAcpiTable (
                                  AcpiTableProtocol,
                                  Table,
                                  TableSize,
                                  &TableKey
                                  );
  }

  FreePool (Table);
  return Status;
}

STATIC
VOID
EFIAPI
RmemOnPublicationEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  (VOID)Context;

  mFinalized = TRUE;
  Status     = RmemPublishTable ();
  if ((Status != EFI_NOT_FOUND) && EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RMEM: Failed to publish table: %r\n", Status));
  }

  gBS->CloseEvent (Event);
  mPublicationEvent = NULL;
}

EFI_STATUS
EFIAPI
RmemAcpiDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HANDLE  ProtocolHandle;
  EFI_STATUS  Status;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  Status = RmemImportHobs ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RMEM: Failed to import HOB records: %r\n", Status));
    return Status;
  }

  ProtocolHandle = NULL;
  Status         = gBS->InstallProtocolInterface (
                          &ProtocolHandle,
                          &gEdkiiRmemRegistrationProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          &mRmemProtocol
                          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RmemOnPublicationEvent,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &mPublicationEvent
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
           ProtocolHandle,
           &gEdkiiRmemRegistrationProtocolGuid,
           &mRmemProtocol
           );
  }

  return Status;
}
