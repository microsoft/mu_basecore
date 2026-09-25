/** @file
  Publishes the Reserved-Memory Reporting (RMEM) ACPI table.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

#include <Pi/PiHob.h>

#include <Guid/EventGroup.h>
#include <Guid/ReservedMemoryReportingHob.h>
#include <Guid/ReservedMemoryReportingTable.h>
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
STATIC EFI_EVENT   mPublicationEvent;
STATIC UINT64      mMaximumPhysicalAddress = MAX_UINT64;

/**
  Sets the maximum physical address from the CPU HOB when it is available.
**/
STATIC
VOID
RmemInitializeMaximumPhysicalAddress (
  VOID
  )
{
  EFI_HOB_CPU  *CpuHob;
  UINT8        PhysicalAddressBits;

  CpuHob = (EFI_HOB_CPU *)GetFirstHob (EFI_HOB_TYPE_CPU);
  if (CpuHob == NULL) {
    DEBUG ((DEBUG_WARN, "RMEM: CPU HOB not found; using the full physical-address type range\n"));
    return;
  }

  PhysicalAddressBits = CpuHob->SizeOfMemorySpace;
  if ((PhysicalAddressBits == 0) || (PhysicalAddressBits > 64)) {
    DEBUG ((DEBUG_ERROR, "RMEM: CPU HOB contains invalid physical-address width %u\n", PhysicalAddressBits));
    ASSERT ((PhysicalAddressBits > 0) && (PhysicalAddressBits <= 64));
    return;
  }

  if (PhysicalAddressBits < 64) {
    mMaximumPhysicalAddress = LShiftU64 (1, PhysicalAddressBits) - 1;
  } else {
    mMaximumPhysicalAddress = MAX_UINT64;
  }
}

/**
  Checks that a range is page-aligned, nonempty, and contained within the
  platform physical address space.
**/
STATIC
BOOLEAN
RmemRangeIsValid (
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN UINT64                Size
  )
{
  if ((Size == 0) ||
      ((Base & EFI_PAGE_MASK) != 0) ||
      ((Size & EFI_PAGE_MASK) != 0) ||
      ((Size - 1) > mMaximumPhysicalAddress))
  {
    return FALSE;
  }

  return Base <= (mMaximumPhysicalAddress - (Size - 1));
}

/**
  Checks whether two valid ranges share at least one byte. Ranges that only
  touch at adjacent endpoints do not overlap.
**/
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
EFI_STATUS
EFIAPI
RmemAddReservedRange (
  IN EDKII_RMEM_REGISTRATION_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS              Base,
  IN UINT64                            Size,
  IN RMEM_CATEGORY                     Category,
  IN UINT8                             Flags,
  IN CONST CHAR8                       *Label OPTIONAL
  )
{
  CONST CHAR8  *EffectiveLabel;
  UINTN        LabelLength;
  UINT32       Index;

  if (This == NULL) {
    DEBUG ((DEBUG_ERROR, "RMEM: Registration protocol pointer is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (This->Revision != EDKII_RMEM_REGISTRATION_PROTOCOL_REVISION) {
    DEBUG ((
      DEBUG_ERROR,
      "RMEM: Unsupported registration protocol revision %u\n",
      This->Revision
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (mFinalized) {
    DEBUG ((DEBUG_WARN, "RMEM: Registration attempted after table finalization\n"));
    return EFI_ACCESS_DENIED;
  }

  if (!RmemRangeIsValid (Base, Size)) {
    DEBUG ((
      DEBUG_ERROR,
      "RMEM: Invalid range base=0x%lx size=0x%lx maximum=0x%lx\n",
      Base,
      Size,
      mMaximumPhysicalAddress
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (((UINT32)Category <= (UINT32)RmemCategoryUnknown) ||
      ((UINT32)Category >= (UINT32)RmemCategoryMax))
  {
    DEBUG ((DEBUG_ERROR, "RMEM: Invalid category %u\n", (UINT32)Category));
    return EFI_INVALID_PARAMETER;
  }

  if ((Flags & ~RMEM_ENTRY_FLAG_VALID_MASK) != 0) {
    DEBUG ((DEBUG_ERROR, "RMEM: Unsupported flags 0x%02x\n", Flags));
    return EFI_INVALID_PARAMETER;
  }

  EffectiveLabel = (Label == NULL) ? "" : Label;
  LabelLength    = AsciiStrnLenS (EffectiveLabel, RMEM_LABEL_MAX_LEN);
  if (LabelLength >= RMEM_LABEL_MAX_LEN) {
    DEBUG ((DEBUG_ERROR, "RMEM: Label exceeds %u bytes including its terminator\n", RMEM_LABEL_MAX_LEN));
    return EFI_BAD_BUFFER_SIZE;
  }

  for (Index = 0; Index < mEntryCount; Index++) {
    if (RmemRangesOverlap (
          mEntries[Index].Base,
          mEntries[Index].Size,
          Base,
          Size
          ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "RMEM: Range base=0x%lx size=0x%lx overlaps entry %u\n",
        Base,
        Size,
        Index
        ));
      return EFI_ACCESS_DENIED;
    }
  }

  if (mEntryCount >= RMEM_MAX_ENTRIES) {
    DEBUG ((DEBUG_ERROR, "RMEM: Registration capacity of %u entries has been reached\n", RMEM_MAX_ENTRIES));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (&mEntries[mEntryCount], sizeof (mEntries[mEntryCount]));
  mEntries[mEntryCount].Base     = Base;
  mEntries[mEntryCount].Size     = Size;
  mEntries[mEntryCount].Category = (UINT8)Category;
  mEntries[mEntryCount].Flags    = Flags;
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
VOID
RmemImportHobs (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  EFI_HOB_GUID_TYPE  *NextGuidHob;
  RMEM_HOB_RECORD    *Record;
  EFI_STATUS         Status;
  UINT32             HobIndex;

  HobIndex = 0;
  GuidHob  = GetFirstGuidHob (&gEdkiiRmemRecordHobGuid);
  while (GuidHob != NULL) {
    NextGuidHob = GetNextGuidHob (
                    &gEdkiiRmemRecordHobGuid,
                    GET_NEXT_HOB (GuidHob)
                    );

    if (GET_GUID_HOB_DATA_SIZE (GuidHob) != sizeof (RMEM_HOB_RECORD)) {
      DEBUG ((
        DEBUG_ERROR,
        "RMEM: HOB %u has invalid payload size %u\n",
        HobIndex,
        (UINT32)GET_GUID_HOB_DATA_SIZE (GuidHob)
        ));
      ASSERT (GET_GUID_HOB_DATA_SIZE (GuidHob) == sizeof (RMEM_HOB_RECORD));
      GuidHob = NextGuidHob;
      HobIndex++;
      continue;
    }

    Record = (RMEM_HOB_RECORD *)GET_GUID_HOB_DATA (GuidHob);
    if (Record->Revision != RMEM_HOB_REVISION) {
      DEBUG ((DEBUG_ERROR, "RMEM: HOB %u has unsupported revision %u\n", HobIndex, Record->Revision));
      ASSERT (Record->Revision == RMEM_HOB_REVISION);
      GuidHob = NextGuidHob;
      HobIndex++;
      continue;
    }

    if ((Record->Reserved != 0) ||
        !IsZeroBuffer (Record->Reserved2, sizeof (Record->Reserved2)) ||
        (Record->Reserved3 != 0))
    {
      DEBUG ((DEBUG_ERROR, "RMEM: HOB %u has nonzero reserved fields\n", HobIndex));
      ASSERT (
        (Record->Reserved == 0) &&
        IsZeroBuffer (Record->Reserved2, sizeof (Record->Reserved2)) &&
        (Record->Reserved3 == 0)
        );
      GuidHob = NextGuidHob;
      HobIndex++;
      continue;
    }

    Status = RmemAddReservedRange (
               &mRmemProtocol,
               Record->Base,
               Record->Size,
               (RMEM_CATEGORY)Record->Category,
               Record->Flags,
               Record->Label
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "RMEM: HOB %u was rejected: %r\n", HobIndex, Status));
      ASSERT_EFI_ERROR (Status);
    }

    GuidHob = NextGuidHob;
    HobIndex++;
  }
}

STATIC
EFI_STATUS
RmemPublishTable (
  VOID
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;
  RMEM_TABLE_HEADER        *Table;
  RMEM_ENTRY               *TableEntries;
  EFI_STATUS               Status;
  UINT32                   Index;
  UINTN                    TableKey;
  UINTN                    TableSize;

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
  TableEntries = (RMEM_ENTRY *)((UINT8 *)Table + sizeof (RMEM_TABLE_HEADER));
  for (Index = 0; Index < mEntryCount; Index++) {
    if ((TableEntries[Index].Flags & RMEM_ENTRY_FLAG_ADDRESS_HIDDEN) != 0) {
      TableEntries[Index].Base = 0;
    }
  }

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

  // Required by the UEFI driver entry-point ABI but unused by this driver.
  (VOID)ImageHandle;
  (VOID)SystemTable;

  RmemInitializeMaximumPhysicalAddress ();

  RmemImportHobs ();

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
