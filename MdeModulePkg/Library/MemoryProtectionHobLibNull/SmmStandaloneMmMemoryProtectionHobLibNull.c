/**@file
Library defines the gSmmMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/SmmStandaloneMmMemoryProtectionHobLib.h>

// According to the C Specification, a global variable
// which is uninitialized will be zero. The net effect
// is memory protections will be OFF.
SMM_MEMORY_PROTECTION_SETTINGS  gSmmMps;

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
  )
{
  return FALSE;
}
