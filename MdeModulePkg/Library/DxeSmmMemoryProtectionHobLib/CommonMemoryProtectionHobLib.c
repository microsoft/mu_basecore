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
MemoryProtectionSettingsConsistencyCheck (
    VOID
  )
{
  if ((gMPS.HeapGuardPolicy.Fields.UefiPoolGuard || gMPS.HeapGuardPolicy.Fields.UefiPageGuard) &&
       gMPS.HeapGuardPolicy.Fields.UefiFreedMemoryGuard) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - HeapGuardPolicy.UefiFreedMemoryGuard and \
UEFI HeapGuardPolicy.UefiPoolGuard/HeapGuardPolicy.UefiPageGuard \
cannot be active at the same time. Setting all three to ZERO in \
the memory protection settings global.\n",
      __FUNCTION__
      ));
      gMPS.HeapGuardPolicy.Fields.UefiPoolGuard = 0;
      gMPS.HeapGuardPolicy.Fields.UefiPageGuard = 0;
      gMPS.HeapGuardPolicy.Fields.UefiFreedMemoryGuard = 0;
  }

  if (!gMPS.ImageProtectionPolicy.Data && 
        (gMPS.DxeNxProtectionPolicy.Fields.EfiLoaderData       ||
         gMPS.DxeNxProtectionPolicy.Fields.EfiBootServicesData ||
         gMPS.DxeNxProtectionPolicy.Fields.EfiRuntimeServicesData)) {
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

  if (gMPS.HeapGuardPoolType.Data && 
        (!(gMPS.HeapGuardPolicy.Fields.UefiPoolGuard ||
             gMPS.HeapGuardPolicy.Fields.SmmPoolGuard))) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Pool protections are active, \
but neither HeapGuardPolicy.UefiPoolGuard nor \
HeapGuardPolicy.SmmPoolGuard are active.\n",
      __FUNCTION__
      ));
  }

  if (gMPS.HeapGuardPageType.Data &&
        (!(gMPS.HeapGuardPolicy.Fields.UefiPageGuard ||
           gMPS.HeapGuardPolicy.Fields.SmmPageGuard))) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - Heap Guard Page protections are active, \
but neither HeapGuardPolicy.UefiPageGuard nor \
HeapGuardPolicy.SmmPageGuard are active.\n",
      __FUNCTION__
      ));
  }

  if (gMPS.DxeNxProtectionPolicy.Fields.EfiLoaderCode        ||
      gMPS.DxeNxProtectionPolicy.Fields.EfiBootServicesCode  ||
      gMPS.DxeNxProtectionPolicy.Fields.EfiRuntimeServicesCode) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - DxeNxProtectionPolicy.EfiLoaderCode, \
DxeNxProtectionPolicy.EfiBootServicesCode, \
and DxeNxProtectionPolicy.EfiRuntimeServicesCode \
must be set to ZERO. Setting all to ZERO \
in the memory protection settings global.\n",
      __FUNCTION__
      ));
    gMPS.DxeNxProtectionPolicy.Fields.EfiLoaderCode = 0;
    gMPS.DxeNxProtectionPolicy.Fields.EfiBootServicesCode = 0;
    gMPS.DxeNxProtectionPolicy.Fields.EfiRuntimeServicesCode = 0;
  }

  if (gMPS.DxeNxProtectionPolicy.Fields.EfiBootServicesData != gMPS.DxeNxProtectionPolicy.Fields.EfiConventionalMemory) {
    DEBUG ((
      DEBUG_WARN,
      "%a: - DxeNxProtectionPolicy.EfiBootServicesData \
and DxeNxProtectionPolicy.EfiConventionalMemory must have the same value. \
Setting both to ZERO in the memory protection settings global.\n",
      __FUNCTION__
      ));
    gMPS.DxeNxProtectionPolicy.Fields.EfiBootServicesData = 0;
    gMPS.DxeNxProtectionPolicy.Fields.EfiConventionalMemory = 0;
  }

  if (gMPS.NullPointerDetectionPolicy.Fields.UefiNullDetection   &&
      gMPS.NullPointerDetectionPolicy.Fields.DisableReadyToBoot  &&
      gMPS.NullPointerDetectionPolicy.Fields.DisableEndOfDxe) {
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

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
CommonMemoryProtectionHobLibConstructor (
  VOID
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
