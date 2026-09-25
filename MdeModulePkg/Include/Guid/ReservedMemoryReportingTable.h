/** @file
  Defines the Reserved-Memory Reporting (RMEM) ACPI table.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RESERVED_MEMORY_REPORTING_TABLE_H_
#define RESERVED_MEMORY_REPORTING_TABLE_H_

#include <IndustryStandard/Acpi.h>

#define RMEM_TABLE_SIGNATURE  SIGNATURE_32 ('R', 'M', 'E', 'M')
#define RMEM_TABLE_REVISION   1

#define RMEM_ENTRY_FLAG_ADDRESS_HIDDEN  BIT0
#define RMEM_ENTRY_FLAG_VALID_MASK      RMEM_ENTRY_FLAG_ADDRESS_HIDDEN

///
/// Maximum serialized label size in bytes, including the null terminator.
///
#define RMEM_LABEL_MAX_LEN  28

///
/// Identifies the purpose of a reserved-memory range.
///
typedef enum {
  RmemCategoryUnknown               = 0,
  RmemCategorySecurity              = 1,
  RmemCategorySharedMemory          = 2,
  RmemCategoryDisplayFramebuffer    = 3,
  RmemCategoryGpuReserved           = 4,
  RmemCategoryAiAcceleratorReserved = 5,
  RmemCategoryFirmwareRuntime       = 6,
  RmemCategoryOther                 = 7,
  RmemCategoryMax                   = 8
} RMEM_CATEGORY;

#pragma pack(1)

///
/// RMEM ACPI table header followed by EntryCount RMEM_ENTRY structures.
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         EntryCount;
  UINT8                          Reserved[24];
} RMEM_TABLE_HEADER;

///
/// Describes one reserved physical-memory range.
///
typedef struct {
  UINT64    Base;
  UINT64    Size;
  UINT8     Category;
  UINT8     Flags;
  UINT8     Reserved[2];
  CHAR8     Label[RMEM_LABEL_MAX_LEN];
  UINT8     Reserved2[16];
} RMEM_ENTRY;

#pragma pack()

STATIC_ASSERT (sizeof (RMEM_TABLE_HEADER) == 64, "Unexpected RMEM table header size");
STATIC_ASSERT (OFFSET_OF (RMEM_TABLE_HEADER, EntryCount) == 36, "Unexpected RMEM entry count offset");
STATIC_ASSERT (OFFSET_OF (RMEM_TABLE_HEADER, Reserved) == 40, "Unexpected RMEM header reserved offset");
STATIC_ASSERT (sizeof (RMEM_ENTRY) == 64, "Unexpected RMEM entry size");
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Category) == 16, "Unexpected RMEM category offset");
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Flags) == 17, "Unexpected RMEM flags offset");
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Label) == 20, "Unexpected RMEM label offset");
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Reserved2) == 48, "Unexpected RMEM reserved offset");

#endif
