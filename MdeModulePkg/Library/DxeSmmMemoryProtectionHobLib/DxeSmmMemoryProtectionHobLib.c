/**@file
Library fills out gMPS global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Uefi/UefiMultiPhase.h>

#include <Library/MemoryProtectionHobLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

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
  switch (MemoryType) {
    case EfiReservedMemoryType:
      return HeapGuardMemoryType.EfiReservedMemoryType;
    case EfiLoaderCode:
      return HeapGuardMemoryType.EfiLoaderCode;
    case EfiLoaderData:
      return HeapGuardMemoryType.EfiLoaderData;
    case EfiBootServicesCode:
      return HeapGuardMemoryType.EfiBootServicesCode;
    case EfiBootServicesData:
      return HeapGuardMemoryType.EfiBootServicesData;
    case EfiRuntimeServicesCode:
      return HeapGuardMemoryType.EfiRuntimeServicesCode;
    case EfiRuntimeServicesData:
      return HeapGuardMemoryType.EfiRuntimeServicesData;
    case EfiConventionalMemory:
      return HeapGuardMemoryType.EfiConventionalMemory;
    case EfiUnusableMemory:
      return HeapGuardMemoryType.EfiUnusableMemory;
    case EfiACPIReclaimMemory:
      return HeapGuardMemoryType.EfiACPIReclaimMemory;
    case EfiACPIMemoryNVS:
      return HeapGuardMemoryType.EfiACPIMemoryNVS;
    case EfiMemoryMappedIO:
      return HeapGuardMemoryType.EfiMemoryMappedIO;
    case EfiMemoryMappedIOPortSpace:
      return HeapGuardMemoryType.EfiMemoryMappedIOPortSpace;
    case EfiPalCode:
      return HeapGuardMemoryType.EfiPalCode;
    case EfiPersistentMemory:
      return HeapGuardMemoryType.EfiPersistentMemory;
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
MemoryProtectionSettingsConsistencyCheck (
    VOID
  )
{
  if (!gMPS.SetNxForStack && gMPS.DxeNxProtectionPolicy.EfiBootServicesData) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - SetNxForStack is FALSE but \
DxeNxProtectionPolicy.EfiBootServicesData is active. \
NX could still be applied to the stack.\n",
      __FUNCTION__
      ));
  }

  if ((gMPS.HeapGuardPolicy.UefiPoolGuard || gMPS.HeapGuardPolicy.UefiPageGuard) &&
       gMPS.HeapGuardPolicy.UefiFreedMemoryGuard) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - HeapGuardPolicy.UefiFreedMemoryGuard and \
UEFI HeapGuardPolicy.UefiPoolGuard/HeapGuardPolicy.UefiPageGuard \
cannot be active at the same time. Setting all three to ZERO in \
the memory protection settings global.\n",
      __FUNCTION__
      ));
      gMPS.HeapGuardPolicy.UefiPoolGuard = 0;
      gMPS.HeapGuardPolicy.UefiPageGuard = 0;
      gMPS.HeapGuardPolicy.UefiFreedMemoryGuard = 0;
  }

  if (!IMAGE_PROTECTION_ACTIVE && 
        (gMPS.DxeNxProtectionPolicy.EfiLoaderData       ||
         gMPS.DxeNxProtectionPolicy.EfiBootServicesData ||
         gMPS.DxeNxProtectionPolicy.EfiRuntimeServicesData)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Image Protection is inactive, but one or more of \
DxeNxProtectionPolicy.EfiLoaderData \
DxeNxProtectionPolicy.EfiBootServicesData \
DxeNxProtectionPolicy.EfiRuntimeServicesData are active. \
Image data sections could still be non-executable.\n",
      __FUNCTION__
      ));
  }

  if (HEAP_GUARD_POOL_PROTECTION_ACTIVE       && 
        (!(gMPS.HeapGuardPolicy.UefiPoolGuard ||
             gMPS.HeapGuardPolicy.SmmPoolGuard))) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Pool protections are active, \
but neither HeapGuardPolicy.UefiPoolGuard nor \
HeapGuardPolicy.SmmPoolGuard are active.\n",
      __FUNCTION__
      ));
  }

  if (HEAP_GUARD_PAGE_PROTECTION_ACTIVE       &&
        (!(gMPS.HeapGuardPolicy.UefiPageGuard ||
           gMPS.HeapGuardPolicy.SmmPageGuard))) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Page protections are active, \
but neither HeapGuardPolicy.UefiPageGuard nor \
HeapGuardPolicy.SmmPageGuard are active.\n",
      __FUNCTION__
      ));
  }

  if (gMPS.DxeNxProtectionPolicy.EfiLoaderCode        ||
      gMPS.DxeNxProtectionPolicy.EfiBootServicesCode  ||
      gMPS.DxeNxProtectionPolicy.EfiRuntimeServicesCode) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - DxeNxProtectionPolicy.EfiLoaderCode, \
DxeNxProtectionPolicy.EfiBootServicesCode, \
and DxeNxProtectionPolicy.EfiRuntimeServicesCode \
must be set to ZERO. Setting all to ZERO \
in the memory protection settings global.\n",
      __FUNCTION__
      ));
    gMPS.DxeNxProtectionPolicy.EfiLoaderCode = 0;
    gMPS.DxeNxProtectionPolicy.EfiBootServicesCode = 0;
    gMPS.DxeNxProtectionPolicy.EfiRuntimeServicesCode = 0;
  }

  if (gMPS.DxeNxProtectionPolicy.EfiBootServicesData != gMPS.DxeNxProtectionPolicy.EfiConventionalMemory) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - DxeNxProtectionPolicy.EfiBootServicesData \
and DxeNxProtectionPolicy.EfiConventionalMemory must have the same value. \
Setting both to ZERO in the memory protection settings global.\n",
      __FUNCTION__
      ));
    gMPS.DxeNxProtectionPolicy.EfiBootServicesData = 0;
    gMPS.DxeNxProtectionPolicy.EfiConventionalMemory = 0;
  }

  if (gMPS.NullPointerDetectionPolicy.UefiNullDetection   &&
      gMPS.NullPointerDetectionPolicy.DisableReadyToBoot  &&
      gMPS.NullPointerDetectionPolicy.DisableEndOfDxe) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - NULL detection disablement at both ReadyToBoot and EndOfDxe are active. \
NULL detection will be disabled at EndOfDxe.\n",
      __FUNCTION__
      ));
  }
}

/**
  Populates gMPS global with the data present in the HOB. If the HOB entry does not exist,
  this constructor will zero the memory protection settings.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
MemoryProtectionHobLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID *Ptr;

  Ptr = GetFirstGuidHob(&gMemoryProtectionSettingsGuid);

  //
  // Cache the Memory Protection Settings HOB entry
  //
  if (Ptr != NULL) {
    if (*((UINT8*) GET_GUID_HOB_DATA(Ptr)) != (UINT8) MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
      DEBUG ((
        DEBUG_INFO,
        "%a: - Version number of the Memory Protection Settings HOB is invalid.\n",
        __FUNCTION__
        ));
      ASSERT (FALSE);
      ZeroMem(&gMPS, sizeof(gMPS));
      return EFI_SUCCESS;
    }
    CopyMem (&gMPS, GET_GUID_HOB_DATA(Ptr), sizeof(MEMORY_PROTECTION_SETTINGS));
    MemoryProtectionSettingsConsistencyCheck ();
  } else {
    DEBUG ((
      DEBUG_INFO,
      "MemoryProtectionHobLibDxeConstructor - Unable to fetch memory protection HOB. \
Zero-ing memory protection settings\n"
      ));
    ZeroMem (&gMPS, sizeof (gMPS));
  }

  return EFI_SUCCESS;
}