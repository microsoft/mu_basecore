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
#define RMEM_LABEL_MAX_LEN    32

///
/// Identifies the purpose of a reserved-memory range.
///
typedef enum {
  RmemCategoryUnknown            = 0,
  RmemCategorySecurity           = 1,
  RmemCategorySharedComms        = 2,
  RmemCategoryDisplayFramebuffer = 3,
  RmemCategoryGpuReserved        = 4,
  RmemCategoryNpuReserved        = 5,
  RmemCategoryFirmwareRuntime    = 6,
  RmemCategoryOther              = 7,
  RmemCategoryMax                = 8
} RMEM_CATEGORY;

#pragma pack(1)

///
/// RMEM ACPI table header followed by EntryCount RMEM_ENTRY structures.
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         EntryCount;
} RMEM_TABLE_HEADER;

///
/// Describes one reserved physical-memory range.
///
typedef struct {
  UINT64    Base;
  UINT64    Size;
  UINT32    Category;
  CHAR8     Label[RMEM_LABEL_MAX_LEN];
} RMEM_ENTRY;

#pragma pack()

STATIC_ASSERT (sizeof (RMEM_TABLE_HEADER) == 40, "Unexpected RMEM table header size");
STATIC_ASSERT (sizeof (RMEM_ENTRY) == 52, "Unexpected RMEM entry size");

#endif
