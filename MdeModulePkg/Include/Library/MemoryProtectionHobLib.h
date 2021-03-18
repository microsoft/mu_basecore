/** @file

Library for controlling hob-backed memory protection settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEMORY_PROTECTION_HOB_HELPER_LIB_H__
#define __MEMORY_PROTECTION_HOB_HELPER_LIB_H__

#include <Guid/MemoryProtectionSettings.h>

//
//  The global used to access current Memory Protection Settings
//
extern MEMORY_PROTECTION_SETTINGS  gMPS;

/**
  Gets the input EFI_MEMORY_TYPE from the input HEAP_GUARD_MEMORY_TYPES bitfield

  @param[in]  MemoryType            Memory type to check.
  @param[in]  HeapGuardMemoryType   HEAP_GUARD_MEMORY_TYPES bitfield

  @return TRUE  The given EFI_MEMORY_TYPE is TRUE in the given HEAP_GUARD_MEMORY_TYPES
  @return FALSE The given EFI_MEMORY_TYPE is FALSE in the given HEAP_GUARD_MEMORY_TYPES
**/
BOOLEAN
EFIAPI
GetMemoryTypeSettingFromBitfield (
  IN EFI_MEMORY_TYPE          MemoryType,
  IN HEAP_GUARD_MEMORY_TYPES  HeapGuardMemoryType
  );

#endif
