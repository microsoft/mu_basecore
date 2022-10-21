/** @file -- MemoryProtectionSpecialRegion.h

  The memory protection special region protocol enables fetching all special regions
  and adding new ones.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL_H__
#define __MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL_H__

#include <Guid/MemoryProtectionSpecialRegion.h>

#define MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL_GUID \
{ \
  0x47BF7F78, 0x0B53, 0x487B, {0xA2, 0xFE, 0x42, 0xE7, 0x09, 0x87, 0x54, 0x5C } \
}

typedef
EFI_STATUS
(EFIAPI *GET_SPECIAL_REGIONS)(
  MEMORY_PROTECTION_SPECIAL_REGION    **SpecialRegions,
  UINTN                               *Count
  );

typedef
EFI_STATUS
(EFIAPI *ADD_SPECIAL_REGION)(
  EFI_PHYSICAL_ADDRESS  Start,
  EFI_PHYSICAL_ADDRESS  Length,
  UINTN                 Attributes
  );

typedef struct _MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL {
  GET_SPECIAL_REGIONS    GetSpecialRegions;
  ADD_SPECIAL_REGION     AddSpecialRegion;
} MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL;

extern EFI_GUID  gMemoryProtectionSpecialRegionProtocolGuid;

#endif
