/**@file
Library defines the gMPS global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi/UefiMultiPhase.h>
#include <Library/MemoryProtectionHobLib.h>

// According to the C Specification, a global variable
// which is uninitialized will be zero. The net effect
// is memory protections will be OFF.
MEMORY_PROTECTION_SETTINGS   gMPS;

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
  )
{
    return FALSE;
}
