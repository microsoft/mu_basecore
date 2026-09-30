/** @file
  Root definition file for the Reserved-Memory Reporting (RMEM) ACPI table.

  This file defines the RMEM wire-format structures, categories, and flags used
  by RMEM producers, publishers, and consumers.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <IndustryStandard/Acpi.h>

#define RMEM_TABLE_SIGNATURE  SIGNATURE_32 ('R', 'M', 'E', 'M')
#define RMEM_TABLE_REVISION   1

///
/// RMEM_ENTRY.Flags values.
///
/// When RMEM_ENTRY_FLAG_ADDRESS_HIDDEN is set, the producer still supplies the
/// actual physical address for validation. The publisher writes zero to the
/// serialized RMEM_ENTRY.Base field to avoid exposing that address.
///
#define RMEM_ENTRY_FLAG_ADDRESS_HIDDEN  BIT0
#define RMEM_ENTRY_FLAG_VALID_MASK      RMEM_ENTRY_FLAG_ADDRESS_HIDDEN

///
/// Maximum serialized label size in bytes, including the null terminator.
///
#define RMEM_LABEL_MAX_LEN  28

///
/// Values stored in the UINT16 RMEM_ENTRY.Category field.
///
/// RmemCategoryUnknown is an invalid zero-value sentinel. Valid serialized
/// category values must fit in UINT16 and be less than RmemCategoryMax.
///
typedef enum {
  ///
  /// Invalid sentinel for a missing or uninitialized category.
  ///
  RmemCategoryUnknown = 0,

  ///
  /// Memory used for isolated execution, security processors, or protected
  /// services.
  ///
  RmemCategorySecurity = 1,

  ///
  /// Memory regions or buffers shared between firmware execution environments
  /// or components.
  ///
  RmemCategorySharedMemory = 2,

  ///
  /// Memory used for a pre-OS or persistent display framebuffer.
  ///
  RmemCategoryDisplayFramebuffer = 3,

  ///
  /// Memory reserved for graphics processing.
  ///
  RmemCategoryGpuReserved = 4,

  ///
  /// Memory reserved for AI accelerator processing.
  ///
  RmemCategoryAiAcceleratorReserved = 5,

  ///
  /// Memory used for firmware runtime data, services, or crash diagnostics.
  ///
  RmemCategoryFirmwareRuntime = 6,

  ///
  /// A valid reservation that does not fit another defined category.
  ///
  RmemCategoryOther = 7,
  RmemCategoryMax   = 8
} RMEM_CATEGORY;

#pragma pack(1)

///
/// RMEM ACPI table header. EntryCount RMEM_ENTRY structures begin at
/// EntryOffset.
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT16                         EntryCount;
  UINT16                         EntryOffset;
} RMEM_TABLE_HEADER;

///
/// Describes one reserved physical-memory range.
///
typedef struct {
  UINT64    Base;
  UINT64    Size;
  UINT16    Category;
  UINT16    Flags;
  CHAR8     Label[RMEM_LABEL_MAX_LEN];
} RMEM_ENTRY;

#pragma pack()

STATIC_ASSERT (RmemCategoryMax <= MAX_UINT16, "RMEM categories do not fit in the wire-format field");
STATIC_ASSERT (RMEM_ENTRY_FLAG_VALID_MASK <= MAX_UINT16, "RMEM flags do not fit in the wire-format field");
STATIC_ASSERT (sizeof (RMEM_TABLE_HEADER) == 40, "Unexpected RMEM table header size");
STATIC_ASSERT (OFFSET_OF (RMEM_TABLE_HEADER, EntryCount) == 36, "Unexpected RMEM entry count offset");
STATIC_ASSERT (OFFSET_OF (RMEM_TABLE_HEADER, EntryOffset) == 38, "Unexpected RMEM entry offset field");
STATIC_ASSERT (sizeof (RMEM_ENTRY) == 48, "Unexpected RMEM entry size");
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Category) == 16, "Unexpected RMEM category offset");
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Flags) == 18, "Unexpected RMEM flags offset");
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Label) == 20, "Unexpected RMEM label offset");
