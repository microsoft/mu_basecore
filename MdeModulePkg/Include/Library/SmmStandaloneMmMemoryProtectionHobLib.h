/** @file

Library for controlling hob-backed memory protection settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_MEMORY_PROTECTION_HOB_HELPER_LIB_H__
#define __SMM_MEMORY_PROTECTION_HOB_HELPER_LIB_H__

#include <Guid/SmmMemoryProtectionSettings.h>

//
//  The global used to access current Memory Protection Settings
//
extern SMM_MEMORY_PROTECTION_SETTINGS  gSmmMps;

/**
  Gets the input EFI_MEMORY_TYPE from the input SMM_HEAP_GUARD_MEMORY_TYPES bitfield

  @param[in]  MemoryType            Memory type to check.
  @param[in]  HeapGuardMemoryType   SMM_HEAP_GUARD_MEMORY_TYPES bitfield

  @return TRUE  The given EFI_MEMORY_TYPE is TRUE in the given SMM_HEAP_GUARD_MEMORY_TYPES
  @return FALSE The given EFI_MEMORY_TYPE is FALSE in the given SMM_HEAP_GUARD_MEMORY_TYPES
**/
BOOLEAN
EFIAPI
GetSmmMemoryTypeSettingFromBitfield (
  IN EFI_MEMORY_TYPE              MemoryType,
  IN SMM_HEAP_GUARD_MEMORY_TYPES  HeapGuardMemoryType
  );

#endif
