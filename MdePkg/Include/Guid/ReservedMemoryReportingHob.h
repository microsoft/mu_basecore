/** @file
  Defines the Reserved-Memory Reporting (RMEM) GUID HOB.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RESERVED_MEMORY_REPORTING_HOB_H_
#define RESERVED_MEMORY_REPORTING_HOB_H_

#include <IndustryStandard/ReservedMemoryReportingTable.h>

#define EDKII_RMEM_RECORD_HOB_GUID \
  { 0x089dcec6, 0x097b, 0x4dbf, { 0x8d, 0xb4, 0x68, 0x23, 0x61, 0xc8, 0x4a, 0x1e } }

#define RMEM_HOB_REVISION  1

#pragma pack(1)

///
/// Carries one reserved-memory range from a pre-DXE producer to the RMEM DXE
/// publisher.
///
typedef struct {
  UINT32    Revision;
  UINT32    Reserved;
  UINT64    Base;
  UINT64    Size;
  UINT32    Category;
  UINT32    Reserved2;
  CHAR8     Label[RMEM_LABEL_MAX_LEN];
} RMEM_HOB_RECORD;

#pragma pack()

STATIC_ASSERT (sizeof (RMEM_HOB_RECORD) == 64, "Unexpected RMEM HOB record size");

extern EFI_GUID  gEdkiiRmemRecordHobGuid;

#endif
