/** @file
  Defines the Reserved-Memory Reporting (RMEM) GUID HOB.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Guid/ReservedMemoryReportingTable.h>

#pragma pack(1)

///
/// Carries one reserved-memory range from a pre-DXE producer to the RMEM DXE
/// publisher. Category values and flag bits are defined in
/// ReservedMemoryReportingTable.h.
///
typedef struct {
  UINT64    Base;
  UINT64    Size;
  UINT16    Category;
  UINT16    Flags;
  CHAR8     Label[RMEM_LABEL_MAX_LEN];
} RMEM_HOB_RECORD;

#pragma pack()

STATIC_ASSERT (sizeof (RMEM_HOB_RECORD) == 48, "Unexpected RMEM HOB record size");
STATIC_ASSERT (OFFSET_OF (RMEM_HOB_RECORD, Category) == 16, "Unexpected RMEM HOB category offset");
STATIC_ASSERT (OFFSET_OF (RMEM_HOB_RECORD, Flags) == 18, "Unexpected RMEM HOB flags offset");
STATIC_ASSERT (OFFSET_OF (RMEM_HOB_RECORD, Label) == 20, "Unexpected RMEM HOB label offset");

extern EFI_GUID  gEdkiiRmemRecordHobGuid;
