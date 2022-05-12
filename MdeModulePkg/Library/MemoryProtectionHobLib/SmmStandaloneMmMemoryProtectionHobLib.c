/**@file
Library fills out gSmmMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Uefi/UefiMultiPhase.h>

#include <Library/SmmStandaloneMmMemoryProtectionHobLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

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
  switch (MemoryType) {
    case EfiReservedMemoryType:
      return HeapGuardMemoryType.Fields.EfiReservedMemoryType;
    case EfiLoaderCode:
      return HeapGuardMemoryType.Fields.EfiLoaderCode;
    case EfiLoaderData:
      return HeapGuardMemoryType.Fields.EfiLoaderData;
    case EfiBootServicesCode:
      return HeapGuardMemoryType.Fields.EfiBootServicesCode;
    case EfiBootServicesData:
      return HeapGuardMemoryType.Fields.EfiBootServicesData;
    case EfiRuntimeServicesCode:
      return HeapGuardMemoryType.Fields.EfiRuntimeServicesCode;
    case EfiRuntimeServicesData:
      return HeapGuardMemoryType.Fields.EfiRuntimeServicesData;
    case EfiConventionalMemory:
      return HeapGuardMemoryType.Fields.EfiConventionalMemory;
    case EfiUnusableMemory:
      return HeapGuardMemoryType.Fields.EfiUnusableMemory;
    case EfiACPIReclaimMemory:
      return HeapGuardMemoryType.Fields.EfiACPIReclaimMemory;
    case EfiACPIMemoryNVS:
      return HeapGuardMemoryType.Fields.EfiACPIMemoryNVS;
    case EfiMemoryMappedIO:
      return HeapGuardMemoryType.Fields.EfiMemoryMappedIO;
    case EfiMemoryMappedIOPortSpace:
      return HeapGuardMemoryType.Fields.EfiMemoryMappedIOPortSpace;
    case EfiPalCode:
      return HeapGuardMemoryType.Fields.EfiPalCode;
    case EfiPersistentMemory:
      return HeapGuardMemoryType.Fields.EfiPersistentMemory;
    default:
      return FALSE;
  }
}

/**
  This function checks the memory protection settings and provides warnings of conflicts and/or
  potentially unforseen consequences from the settings. This logic will only ever turn off
  protections to create consistency, never turn others on.
**/
VOID
SmmMemoryProtectionSettingsConsistencyCheck (
  VOID
  )
{
  if (gSmmMps.HeapGuardPoolType.Data &&
      (!(gSmmMps.HeapGuardPolicy.Fields.SmmPoolGuard)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Pool protections are active, \
but neither HeapGuardPolicy.UefiPoolGuard nor \
HeapGuardPolicy.SmmPoolGuard are active.\n",
      __FUNCTION__
      ));
  }

  if (gSmmMps.HeapGuardPageType.Data &&
      (!(gSmmMps.HeapGuardPolicy.Fields.SmmPageGuard)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Page protections are active, \
but neither HeapGuardPolicy.UefiPageGuard nor \
HeapGuardPolicy.SmmPageGuard are active.\n",
      __FUNCTION__
      ));
  }
}

/**
  Abstraction layer for library constructor of Standalone MM and SMM instances.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
MmMemoryProtectionHobLibConstructorCommon (
  VOID
  )
{
  VOID  *Ptr;

  Ptr = GetFirstGuidHob (&gSmmMemoryProtectionSettingsGuid);

  //
  // Cache the Memory Protection Settings HOB entry
  //
  if (Ptr != NULL) {
    if (*((UINT8 *)GET_GUID_HOB_DATA (Ptr)) != (UINT8)SMM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
      DEBUG ((
        DEBUG_INFO,
        "%a: - Version number of the Memory Protection Settings HOB is invalid.\n",
        __FUNCTION__
        ));
      ASSERT (FALSE);
      ZeroMem (&gSmmMps, sizeof (gSmmMps));
      return EFI_SUCCESS;
    }

    CopyMem (&gSmmMps, GET_GUID_HOB_DATA (Ptr), sizeof (SMM_MEMORY_PROTECTION_SETTINGS));
    SmmMemoryProtectionSettingsConsistencyCheck ();
  } else {
    DEBUG ((
      DEBUG_INFO,
      "SmmStandaloneMmMemoryProtectionHobLibConstructor - Unable to fetch memory protection HOB. \
Zero-ing SMM memory protection settings\n"
      ));
    ZeroMem (&gSmmMps, sizeof (gSmmMps));
  }

  return EFI_SUCCESS;
}
