/** @file -- MemoryProtectionSpecialRegion.h

  The required struct definition and GUID for reporting special memory protection
  regions via the HOB.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MEMORY_PROTECTION_SPECIAL_REGION_GUID_H__
#define __MEMORY_PROTECTION_SPECIAL_REGION_GUID_H__

#define MEMORY_PROTECTION_SPECIAL_REGION_GUID \
{ \
  0xBFCC1325, 0x3BFE, 0x469A, { 0x9A, 0xAC, 0x0C, 0x99, 0x0E, 0x4E, 0xC9, 0xF1 } \
}

typedef struct _MEMORY_PROTECTION_SPECIAL_REGION {
  EFI_PHYSICAL_ADDRESS    Start;
  EFI_PHYSICAL_ADDRESS    Length;
  UINT64                  EfiAttributes;
} MEMORY_PROTECTION_SPECIAL_REGION;

extern EFI_GUID  gMemoryProtectionSpecialRegionHobGuid;

#endif
