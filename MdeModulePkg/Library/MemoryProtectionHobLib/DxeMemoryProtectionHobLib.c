/**@file
Library fills out gDxeMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Uefi/UefiMultiPhase.h>

#include <Library/DxeMemoryProtectionHobLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

DXE_MEMORY_PROTECTION_SETTINGS  gDxeMps;

/**
  Gets the input EFI_MEMORY_TYPE from the input DXE_HEAP_GUARD_MEMORY_TYPES bitfield

  @param[in]  MemoryType            Memory type to check.
  @param[in]  HeapGuardMemoryType   DXE_HEAP_GUARD_MEMORY_TYPES bitfield

  @return TRUE  The given EFI_MEMORY_TYPE is TRUE in the given DXE_HEAP_GUARD_MEMORY_TYPES
  @return FALSE The given EFI_MEMORY_TYPE is FALSE in the given DXE_HEAP_GUARD_MEMORY_TYPES
**/
BOOLEAN
EFIAPI
GetDxeMemoryTypeSettingFromBitfield (
  IN EFI_MEMORY_TYPE              MemoryType,
  IN DXE_HEAP_GUARD_MEMORY_TYPES  HeapGuardMemoryType
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
    case EfiUnacceptedMemoryType:
      return HeapGuardMemoryType.Fields.EfiUnacceptedMemoryType;
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
DxeMemoryProtectionSettingsConsistencyCheck (
  VOID
  )
{
  if ((gDxeMps.HeapGuardPolicy.Fields.UefiPoolGuard || gDxeMps.HeapGuardPolicy.Fields.UefiPageGuard) &&
      gDxeMps.HeapGuardPolicy.Fields.UefiFreedMemoryGuard)
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - HeapGuardPolicy.UefiFreedMemoryGuard and \
UEFI HeapGuardPolicy.UefiPoolGuard/HeapGuardPolicy.UefiPageGuard \
cannot be active at the same time. Setting all three to ZERO in \
the memory protection settings global.\n",
      __FUNCTION__
      ));
    gDxeMps.HeapGuardPolicy.Fields.UefiPoolGuard        = 0;
    gDxeMps.HeapGuardPolicy.Fields.UefiPageGuard        = 0;
    gDxeMps.HeapGuardPolicy.Fields.UefiFreedMemoryGuard = 0;
  }

  if (!gDxeMps.ImageProtectionPolicy.Data &&
      (gDxeMps.NxProtectionPolicy.Fields.EfiLoaderData       ||
       gDxeMps.NxProtectionPolicy.Fields.EfiBootServicesData ||
       gDxeMps.NxProtectionPolicy.Fields.EfiRuntimeServicesData))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Image Protection is inactive, but one or more of \
NxProtectionPolicy.EfiLoaderData \
NxProtectionPolicy.EfiBootServicesData \
NxProtectionPolicy.EfiRuntimeServicesData are active. \
Image data sections could still be non-executable.\n",
      __FUNCTION__
      ));
  }

  if (gDxeMps.HeapGuardPoolType.Data &&
      (!(gDxeMps.HeapGuardPolicy.Fields.UefiPoolGuard)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Pool protections are active, \
but neither HeapGuardPolicy.UefiPoolGuard nor \
HeapGuardPolicy.MmPoolGuard are active.\n",
      __FUNCTION__
      ));
  }

  if (gDxeMps.HeapGuardPageType.Data &&
      (!(gDxeMps.HeapGuardPolicy.Fields.UefiPageGuard)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Page protections are active, \
but neither HeapGuardPolicy.UefiPageGuard nor \
HeapGuardPolicy.MmPageGuard are active.\n",
      __FUNCTION__
      ));
  }

  if (gDxeMps.NxProtectionPolicy.Fields.EfiBootServicesData != gDxeMps.NxProtectionPolicy.Fields.EfiConventionalMemory) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - NxProtectionPolicy.EfiBootServicesData \
and NxProtectionPolicy.EfiConventionalMemory must have the same value. \
Setting both to ZERO in the memory protection settings global.\n",
      __FUNCTION__
      ));
    gDxeMps.NxProtectionPolicy.Fields.EfiBootServicesData   = 0;
    gDxeMps.NxProtectionPolicy.Fields.EfiConventionalMemory = 0;
  }

  if (gDxeMps.NullPointerDetectionPolicy.Fields.UefiNullDetection   &&
      gDxeMps.NullPointerDetectionPolicy.Fields.DisableReadyToBoot  &&
      gDxeMps.NullPointerDetectionPolicy.Fields.DisableEndOfDxe)
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - NULL detection disablement at both ReadyToBoot and EndOfDxe are active. \
NULL detection will be disabled at EndOfDxe.\n",
      __FUNCTION__
      ));
  }

  if (!(gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromFv || gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromUnknown) &&
      gDxeMps.ImageProtectionPolicy.Fields.RaiseErrorIfProtectionFails)
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - ProtectImageFromFv and/or ProtectImageFromUnknown are \
inactive but RaiseErrorIfProtectionFails is active. RaiseErrorIfProtectionFails would have no effect\n",
      __FUNCTION__
      ));
    gDxeMps.ImageProtectionPolicy.Fields.RaiseErrorIfProtectionFails = 0;
  }
}

/**
  Populates gDxeMps global with the data present in the HOB. If the HOB entry does not exist,
  this constructor will zero the memory protection settings.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
DxeMemoryProtectionHobLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID  *Ptr;

  Ptr = GetFirstGuidHob (&gDxeMemoryProtectionSettingsGuid);

  //
  // Cache the Memory Protection Settings HOB entry
  //
  if (Ptr != NULL) {
    if (*((UINT8 *)GET_GUID_HOB_DATA (Ptr)) != (UINT8)DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
      DEBUG ((
        DEBUG_INFO,
        "%a: - Version number of the Memory Protection Settings HOB is invalid.\n",
        __FUNCTION__
        ));
      ASSERT (FALSE);
      ZeroMem (&gDxeMps, sizeof (gDxeMps));
      return EFI_SUCCESS;
    }

    CopyMem (&gDxeMps, GET_GUID_HOB_DATA (Ptr), sizeof (DXE_MEMORY_PROTECTION_SETTINGS));
    DxeMemoryProtectionSettingsConsistencyCheck ();
  } else {
    DEBUG ((
      DEBUG_INFO,
      "DxeMemoryProtectionHobLibConstructor - Unable to fetch memory protection HOB. \
Zero-ing memory protection settings\n"
      ));
    ZeroMem (&gDxeMps, sizeof (gDxeMps));
  }

  return EFI_SUCCESS;
}
