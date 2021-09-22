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
extern MEMORY_PROTECTION_SETTINGS   gMPS;

#define HEAP_GUARD_ACTIVE (                             \
          gMPS.HeapGuardPolicy.UefiPageGuard         || \
          gMPS.HeapGuardPolicy.UefiPoolGuard         || \
          gMPS.HeapGuardPolicy.SmmPageGuard          || \
          gMPS.HeapGuardPolicy.SmmPoolGuard          || \
          gMPS.HeapGuardPolicy.UefiFreedMemoryGuard  || \
          gMPS.HeapGuardPolicy.NonstopMode           || \
          gMPS.HeapGuardPolicy.Direction )

#define IMAGE_PROTECTION_ACTIVE (                 \
        gMPS.ImageProtectionPolicy.FromUnknown || \
        gMPS.ImageProtectionPolicy.FromFv )

#define NX_PROTECTION_ACTIVE (                                       \
          gMPS.DxeNxProtectionPolicy.EfiReservedMemoryType       ||  \
          gMPS.DxeNxProtectionPolicy.EfiLoaderCode               ||  \
          gMPS.DxeNxProtectionPolicy.EfiLoaderData               ||  \
          gMPS.DxeNxProtectionPolicy.EfiBootServicesCode         ||  \
          gMPS.DxeNxProtectionPolicy.EfiBootServicesData         ||  \
          gMPS.DxeNxProtectionPolicy.EfiRuntimeServicesCode      ||  \
          gMPS.DxeNxProtectionPolicy.EfiRuntimeServicesData      ||  \
          gMPS.DxeNxProtectionPolicy.EfiConventionalMemory       ||  \
          gMPS.DxeNxProtectionPolicy.EfiUnusableMemory           ||  \
          gMPS.DxeNxProtectionPolicy.EfiACPIReclaimMemory        ||  \
          gMPS.DxeNxProtectionPolicy.EfiACPIMemoryNVS            ||  \
          gMPS.DxeNxProtectionPolicy.EfiMemoryMappedIO           ||  \
          gMPS.DxeNxProtectionPolicy.EfiMemoryMappedIOPortSpace  ||  \
          gMPS.DxeNxProtectionPolicy.EfiPalCode                  ||  \
          gMPS.DxeNxProtectionPolicy.EfiPersistentMemory )

#define HEAP_GUARD_POOL_PROTECTION_ACTIVE (                      \
          gMPS.HeapGuardPoolType.EfiReservedMemoryType       ||  \
          gMPS.HeapGuardPoolType.EfiLoaderCode               ||  \
          gMPS.HeapGuardPoolType.EfiLoaderData               ||  \
          gMPS.HeapGuardPoolType.EfiBootServicesCode         ||  \
          gMPS.HeapGuardPoolType.EfiBootServicesData         ||  \
          gMPS.HeapGuardPoolType.EfiRuntimeServicesCode      ||  \
          gMPS.HeapGuardPoolType.EfiRuntimeServicesData      ||  \
          gMPS.HeapGuardPoolType.EfiConventionalMemory       ||  \
          gMPS.HeapGuardPoolType.EfiUnusableMemory           ||  \
          gMPS.HeapGuardPoolType.EfiACPIReclaimMemory        ||  \
          gMPS.HeapGuardPoolType.EfiACPIMemoryNVS            ||  \
          gMPS.HeapGuardPoolType.EfiMemoryMappedIO           ||  \
          gMPS.HeapGuardPoolType.EfiMemoryMappedIOPortSpace  ||  \
          gMPS.HeapGuardPoolType.EfiPalCode                  ||  \
          gMPS.HeapGuardPoolType.EfiPersistentMemory )

#define HEAP_GUARD_PAGE_PROTECTION_ACTIVE (                      \
          gMPS.HeapGuardPageType.EfiReservedMemoryType       ||  \
          gMPS.HeapGuardPageType.EfiLoaderCode               ||  \
          gMPS.HeapGuardPageType.EfiLoaderData               ||  \
          gMPS.HeapGuardPageType.EfiBootServicesCode         ||  \
          gMPS.HeapGuardPageType.EfiBootServicesData         ||  \
          gMPS.HeapGuardPageType.EfiRuntimeServicesCode      ||  \
          gMPS.HeapGuardPageType.EfiRuntimeServicesData      ||  \
          gMPS.HeapGuardPageType.EfiConventionalMemory       ||  \
          gMPS.HeapGuardPageType.EfiUnusableMemory           ||  \
          gMPS.HeapGuardPageType.EfiACPIReclaimMemory        ||  \
          gMPS.HeapGuardPageType.EfiACPIMemoryNVS            ||  \
          gMPS.HeapGuardPageType.EfiMemoryMappedIO           ||  \
          gMPS.HeapGuardPageType.EfiMemoryMappedIOPortSpace  ||  \
          gMPS.HeapGuardPageType.EfiPalCode                  ||  \
          gMPS.HeapGuardPageType.EfiPersistentMemory )

#define NULL_POINTER_DETECTION_ACTIVE (                          \
          gMPS.NullPointerDetectionPolicy.UefiNullDetection  ||  \
          gMPS.NullPointerDetectionPolicy.SmmNullDetection   ||  \
          gMPS.NullPointerDetectionPolicy.NonstopMode        ||  \
          gMPS.NullPointerDetectionPolicy.DisableEndOfDxe )

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
