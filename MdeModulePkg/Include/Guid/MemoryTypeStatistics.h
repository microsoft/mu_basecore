/** @file

GUID and structure for storing memory type statistics.
Used for PEI and DXE memory buckets.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef __MEMORY_TYPE_STATISTICS_GUID_H__
#define __MEMORY_TYPE_STATISTICS_GUID_H__

#define MEMORY_TYPE_STATISTICS_GUID \
  { 0x6146C0D6, 0x8E30, 0x4DC2, { 0xA9, 0xCB, 0x5D, 0x85, 0x10, 0xC4, 0x8B, 0x39 }}

extern EFI_GUID  gMemoryTypeStatisticsGuid;

typedef struct {
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  EFI_PHYSICAL_ADDRESS    MaximumAddress;
  UINT64                  CurrentNumberOfPages;
  UINT64                  NumberOfPages;
  UINT32                  InformationIndex;
  BOOLEAN                 Special;
  BOOLEAN                 Runtime;
} EFI_MEMORY_TYPE_STATISTICS;

#endif
