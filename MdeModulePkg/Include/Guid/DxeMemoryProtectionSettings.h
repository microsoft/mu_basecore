/** @file

Defines memory protection settings guid and struct

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DXE_MEMORY_PROTECTION_SETTINGS_H__
#define __DXE_MEMORY_PROTECTION_SETTINGS_H__

typedef union {
  UINT8    Data;
  struct {
    UINT8    UefiNullDetection  : 1;
    UINT8    DisableEndOfDxe    : 1;
    UINT8    DisableReadyToBoot : 1;
  } Fields;
} DXE_NULL_DETECTION_POLICY;

typedef union {
  UINT8    Data;
  struct {
    UINT8    UefiPageGuard        : 1;
    UINT8    UefiPoolGuard        : 1;
    UINT8    UefiFreedMemoryGuard : 1;
    UINT8    Direction            : 1;
  } Fields;
} DXE_HEAP_GUARD_POLICY;

typedef union {
  UINT32    Data;
  struct {
    UINT8    EfiReservedMemoryType      : 1;
    UINT8    EfiLoaderCode              : 1;
    UINT8    EfiLoaderData              : 1;
    UINT8    EfiBootServicesCode        : 1;
    UINT8    EfiBootServicesData        : 1;
    UINT8    EfiRuntimeServicesCode     : 1;
    UINT8    EfiRuntimeServicesData     : 1;
    UINT8    EfiConventionalMemory      : 1;
    UINT8    EfiUnusableMemory          : 1;
    UINT8    EfiACPIReclaimMemory       : 1;
    UINT8    EfiACPIMemoryNVS           : 1;
    UINT8    EfiMemoryMappedIO          : 1;
    UINT8    EfiMemoryMappedIOPortSpace : 1;
    UINT8    EfiPalCode                 : 1;
    UINT8    EfiPersistentMemory        : 1;
    UINT8    OEMReserved                : 1;
    UINT8    OSReserved                 : 1;
  } Fields;
} DXE_HEAP_GUARD_MEMORY_TYPES;

typedef union {
  UINT8    Data;
  struct {
    UINT8    ProtectImageFromUnknown        : 1;
    UINT8    ProtectImageFromFv             : 1;
    UINT8    InstallMemoryAttributeProtocol : 1;
    UINT8    RaiseErrorIfProtectionFails    : 1;
    UINT8    BlockImagesWithoutNxFlag       : 1;
  } Fields;
} DXE_IMAGE_PROTECTION_POLICY;

typedef UINT8 DXE_MEMORY_PROTECTION_SETTINGS_VERSION;

#define DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION  2 // Current iteration of DXE_MEMORY_PROTECTION_SETTINGS

//
// Memory Protection Settings struct
//
typedef struct {
  // The current version of the structure definition. This is used to ensure there isn't a definition mismatch
  // if modules have differing iterations of this header. When creating this struct, use the
  // DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION macro.
  DXE_MEMORY_PROTECTION_SETTINGS_VERSION    StructVersion;

  // Indicates if UEFI Stack Guard will be enabled.
  //
  // If enabled, stack overflow in UEFI can be caught.
  //  TRUE  - UEFI Stack Guard will be enabled.
  //  FALSE - UEFI Stack Guard will be disabled.
  BOOLEAN                                   CpuStackGuard;

  // Bitfield to control the NULL address detection in code for different phases.
  // If enabled, accessing NULL address in UEFI or SMM code can be caught by marking
  // the NULL page as not present.
  //   .UefiNullDetection   : Enable NULL pointer detection for UEFI.
  //   .DisableEndOfDxe     : Disable NULL pointer detection just after EndOfDxe.
  //                          This is a workaround for those unsolvable NULL access issues in
  //                          OptionROM, boot loader, etc. It can also help to avoid unnecessary
  //                          exception caused by legacy memory (0-4095) access after EndOfDxe,
  //                          such as Windows 7 boot on Qemu.
  //   .DisableReadyToBoot  : Disable NULL pointer detection just after ReadyToBoot.
  DXE_NULL_DETECTION_POLICY    NullPointerDetectionPolicy;

  // Bitfield to control Heap Guard behavior.
  //
  // Note:
  //  a) Due to the limit of pool memory implementation and the alignment
  //     requirement of UEFI spec, HeapGuardPolicy.Direction is a try-best
  //     setting which cannot guarantee that the returned pool is exactly
  //     adjacent to head guard page or tail guard page.
  //  b) UEFI freed-memory guard and UEFI pool/page guard cannot be enabled
  //     at the same time.
  //
  //  .UefiPageGuard         : Enable UEFI page guard.
  //  .UefiPoolGuard         : Enable UEFI pool guard.
  //  .UefiFreedMemoryGuard  : Enable UEFI freed-memory guard (Use-After-Free memory detection).
  //  .Direction             : The direction of Guard Page for Pool Guard.
  //                           0 - The returned pool is near the tail guard page.
  //                           1 - The returned pool is near the head guard page.
  DXE_HEAP_GUARD_POLICY    HeapGuardPolicy;

  // Set image protection policy.
  //
  //  .ProtectImageFromUnknown          : If set, images from unknown devices will be protected by DxeCore
  //                                      if they are aligned. The code section becomes read-only, and the data
  //                                      section becomes non-executable.
  //  .ProtectImageFromFv               : If set, images from firmware volumes will be protected by DxeCore
  //                                      if they are aligned. The code section becomes read-only, and the data
  //                                      section becomes non-executable.
  //  .InstallMemoryAttributeProtocol   : If set, the MemoryAttributeProtocol (definition in CpuPageTable) will
  //                                      be installed.
  //  .RaiseErrorIfProtectionFails      : If set, the image protection logic will return an error if protection fails.
  //  .BlockImagesWithoutNxFlag         : If set, images of subsystem type EFI_APPLICATION
  //                                      which don't utilize the /NXCOMPAT DLL flag will not be loaded.
  //
  // Note: If a bit is cleared, an image data section could be still non-executable if
  // NxProtectionPolicy is enabled for EfiLoaderData, EfiBootServicesData or EfiRuntimeServicesData.
  DXE_IMAGE_PROTECTION_POLICY    ImageProtectionPolicy;

  // Indicates which type allocation need guard page.
  //
  // If bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages which the allocated pool occupies,
  // if there's enough free memory for all of them. The pool allocation for the
  // type related to cleared bits keeps the same as ususal.
  //
  // This bitfield is only valid if UefiPoolGuard and/or SmmPoolGuard are set in HeapGuardPolicy.
  DXE_HEAP_GUARD_MEMORY_TYPES    HeapGuardPoolType;

  // Indicates which type allocation need guard page.
  //
  // If a bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages allocated if there's enough
  // free pages for all of them. The page allocation for the type related to
  // cleared bits keeps the same as ususal.
  //
  // This bitfield is only valid if UefiPageGuard is set in HeapGuardPolicy.
  DXE_HEAP_GUARD_MEMORY_TYPES    HeapGuardPageType;

  // DXE no execute memory protection policy.
  //
  // If a bit is set, memory regions of the associated type will be mapped
  // non-executable. If a bit is cleared, nothing will be done to associated type of memory.
  //
  //  NOTE: - User must NOT set NX protection for EfiLoaderCode / EfiBootServicesCode / EfiRuntimeServicesCode.
  //        - User MUST set the same NX protection for EfiBootServicesData and EfiConventionalMemory.
  DXE_HEAP_GUARD_MEMORY_TYPES    NxProtectionPolicy;
} DXE_MEMORY_PROTECTION_SETTINGS;

#define HOB_DXE_MEMORY_PROTECTION_SETTINGS_GUID \
  { \
    { 0x9ABFD639, 0xD1D0, 0x4EFF, { 0xBD, 0xB6, 0x7E, 0xC4, 0x19, 0x0D, 0x17, 0xD5 } } \
  }

extern GUID  gDxeMemoryProtectionSettingsGuid;

// HeapGuardPolicy.Fields.Direction value indicating tail alignment
#define HEAP_GUARD_ALIGNED_TO_TAIL  0

// HeapGuardPolicy.Fields.Direction value indicating head alignment
#define HEAP_GUARD_ALIGNED_TO_HEAD  1

//
//  A memory profile with strict settings. This will likely add to the
//  total boot time but will catch more configuration and memory errors.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_DEBUG                    \
          {                                                     \
            DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
            TRUE,   /* Stack Guard On */                        \
            {                                                   \
              .Fields.UefiNullDetection               = 1,      \
              .Fields.DisableEndOfDxe                 = 0,      \
              .Fields.DisableReadyToBoot              = 0       \
            },                                                  \
            {                                                   \
              .Fields.UefiPageGuard                   = 1,      \
              .Fields.UefiPoolGuard                   = 1,      \
              .Fields.UefiFreedMemoryGuard            = 0,      \
              .Fields.Direction                       = 0       \
            },                                                  \
            {                                                   \
              .Fields.ProtectImageFromUnknown         = 0,      \
              .Fields.ProtectImageFromFv              = 1,      \
              .Fields.InstallMemoryAttributeProtocol  = 1,      \
              .Fields.RaiseErrorIfProtectionFails     = 1,      \
              .Fields.BlockImagesWithoutNxFlag        = 1       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 0,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 0,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 1,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 1,      \
              .Fields.EfiConventionalMemory           = 0,      \
              .Fields.EfiUnusableMemory               = 0,      \
              .Fields.EfiACPIReclaimMemory            = 0,      \
              .Fields.EfiACPIMemoryNVS                = 0,      \
              .Fields.EfiMemoryMappedIO               = 0,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 0,      \
              .Fields.EfiPalCode                      = 0,      \
              .Fields.EfiPersistentMemory             = 0,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 0,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 0,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 1,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 1,      \
              .Fields.EfiConventionalMemory           = 0,      \
              .Fields.EfiUnusableMemory               = 0,      \
              .Fields.EfiACPIReclaimMemory            = 0,      \
              .Fields.EfiACPIMemoryNVS                = 0,      \
              .Fields.EfiMemoryMappedIO               = 0,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 0,      \
              .Fields.EfiPalCode                      = 0,      \
              .Fields.EfiPersistentMemory             = 0,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 1,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 1,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 1,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 1,      \
              .Fields.EfiConventionalMemory           = 1,      \
              .Fields.EfiUnusableMemory               = 1,      \
              .Fields.EfiACPIReclaimMemory            = 1,      \
              .Fields.EfiACPIMemoryNVS                = 1,      \
              .Fields.EfiMemoryMappedIO               = 1,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 1,      \
              .Fields.EfiPalCode                      = 1,      \
              .Fields.EfiPersistentMemory             = 1,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            }                                                   \
          }

//
//  A memory profile recommended for SHIP_MODE. Compared to the debug
//  settings, this removes the pool guards and doesn't unload images
//  which fail protection.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_SHIP_MODE                \
          {                                                     \
            DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
            TRUE,   /* Stack Guard On */                        \
            {                                                   \
              .Fields.UefiNullDetection               = 1,      \
              .Fields.DisableEndOfDxe                 = 0,      \
              .Fields.DisableReadyToBoot              = 0       \
            },                                                  \
            {                                                   \
              .Fields.UefiPageGuard                   = 1,      \
              .Fields.UefiPoolGuard                   = 0,      \
              .Fields.UefiFreedMemoryGuard            = 0,      \
              .Fields.Direction                       = 0       \
            },                                                  \
            {                                                   \
              .Fields.ProtectImageFromUnknown         = 0,      \
              .Fields.ProtectImageFromFv              = 1,      \
              .Fields.InstallMemoryAttributeProtocol  = 1,      \
              .Fields.RaiseErrorIfProtectionFails     = 0,      \
              .Fields.BlockImagesWithoutNxFlag        = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 0,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 0,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 0,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 0,      \
              .Fields.EfiConventionalMemory           = 0,      \
              .Fields.EfiUnusableMemory               = 0,      \
              .Fields.EfiACPIReclaimMemory            = 0,      \
              .Fields.EfiACPIMemoryNVS                = 0,      \
              .Fields.EfiMemoryMappedIO               = 0,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 0,      \
              .Fields.EfiPalCode                      = 0,      \
              .Fields.EfiPersistentMemory             = 0,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 0,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 0,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 1,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 1,      \
              .Fields.EfiConventionalMemory           = 0,      \
              .Fields.EfiUnusableMemory               = 0,      \
              .Fields.EfiACPIReclaimMemory            = 0,      \
              .Fields.EfiACPIMemoryNVS                = 0,      \
              .Fields.EfiMemoryMappedIO               = 0,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 0,      \
              .Fields.EfiPalCode                      = 0,      \
              .Fields.EfiPersistentMemory             = 0,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 1,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 1,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 1,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 1,      \
              .Fields.EfiConventionalMemory           = 1,      \
              .Fields.EfiUnusableMemory               = 1,      \
              .Fields.EfiACPIReclaimMemory            = 1,      \
              .Fields.EfiACPIMemoryNVS                = 1,      \
              .Fields.EfiMemoryMappedIO               = 1,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 1,      \
              .Fields.EfiPalCode                      = 1,      \
              .Fields.EfiPersistentMemory             = 1,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            }                                                   \
          }

//
//  A memory profile which mirrors DXE_MEMORY_PROTECTION_SETTINGS_SHIP_MODE
//  but doesn't include page guards.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_SHIP_MODE_NO_PAGE_GUARDS \
          {                                                     \
            DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
            TRUE,   /* Stack Guard On */                        \
            {                                                   \
              .Fields.UefiNullDetection               = 1,      \
              .Fields.DisableEndOfDxe                 = 0,      \
              .Fields.DisableReadyToBoot              = 0       \
            },                                                  \
            {                                                   \
              .Fields.UefiPageGuard                   = 0,      \
              .Fields.UefiPoolGuard                   = 0,      \
              .Fields.UefiFreedMemoryGuard            = 0,      \
              .Fields.Direction                       = 0       \
            },                                                  \
            {                                                   \
              .Fields.ProtectImageFromUnknown         = 0,      \
              .Fields.ProtectImageFromFv              = 1,      \
              .Fields.InstallMemoryAttributeProtocol  = 1,      \
              .Fields.RaiseErrorIfProtectionFails     = 0,      \
              .Fields.BlockImagesWithoutNxFlag        = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 0,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 0,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 0,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 0,      \
              .Fields.EfiConventionalMemory           = 0,      \
              .Fields.EfiUnusableMemory               = 0,      \
              .Fields.EfiACPIReclaimMemory            = 0,      \
              .Fields.EfiACPIMemoryNVS                = 0,      \
              .Fields.EfiMemoryMappedIO               = 0,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 0,      \
              .Fields.EfiPalCode                      = 0,      \
              .Fields.EfiPersistentMemory             = 0,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 0,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 0,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 0,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 0,      \
              .Fields.EfiConventionalMemory           = 0,      \
              .Fields.EfiUnusableMemory               = 0,      \
              .Fields.EfiACPIReclaimMemory            = 0,      \
              .Fields.EfiACPIMemoryNVS                = 0,      \
              .Fields.EfiMemoryMappedIO               = 0,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 0,      \
              .Fields.EfiPalCode                      = 0,      \
              .Fields.EfiPersistentMemory             = 0,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 1,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 1,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 1,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 1,      \
              .Fields.EfiConventionalMemory           = 1,      \
              .Fields.EfiUnusableMemory               = 1,      \
              .Fields.EfiACPIReclaimMemory            = 1,      \
              .Fields.EfiACPIMemoryNVS                = 1,      \
              .Fields.EfiMemoryMappedIO               = 1,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 1,      \
              .Fields.EfiPalCode                      = 1,      \
              .Fields.EfiPersistentMemory             = 1,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            }                                                   \
          }

//
//  A memory profile which disables all memory protection settings.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_OFF                      \
          {                                                     \
            DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
            FALSE,   /* Stack Guard On */                       \
            {                                                   \
              .Fields.UefiNullDetection               = 0,      \
              .Fields.DisableEndOfDxe                 = 0,      \
              .Fields.DisableReadyToBoot              = 0       \
            },                                                  \
            {                                                   \
              .Fields.UefiPageGuard                   = 0,      \
              .Fields.UefiPoolGuard                   = 0,      \
              .Fields.UefiFreedMemoryGuard            = 0,      \
              .Fields.Direction                       = 0       \
            },                                                  \
            {                                                   \
              .Fields.ProtectImageFromUnknown         = 0,      \
              .Fields.ProtectImageFromFv              = 0,      \
              .Fields.InstallMemoryAttributeProtocol  = 0,      \
              .Fields.RaiseErrorIfProtectionFails     = 0,      \
              .Fields.BlockImagesWithoutNxFlag        = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 0,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 0,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 0,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 0,      \
              .Fields.EfiConventionalMemory           = 0,      \
              .Fields.EfiUnusableMemory               = 0,      \
              .Fields.EfiACPIReclaimMemory            = 0,      \
              .Fields.EfiACPIMemoryNVS                = 0,      \
              .Fields.EfiMemoryMappedIO               = 0,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 0,      \
              .Fields.EfiPalCode                      = 0,      \
              .Fields.EfiPersistentMemory             = 0,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 0,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 0,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 0,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 0,      \
              .Fields.EfiConventionalMemory           = 0,      \
              .Fields.EfiUnusableMemory               = 0,      \
              .Fields.EfiACPIReclaimMemory            = 0,      \
              .Fields.EfiACPIMemoryNVS                = 0,      \
              .Fields.EfiMemoryMappedIO               = 0,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 0,      \
              .Fields.EfiPalCode                      = 0,      \
              .Fields.EfiPersistentMemory             = 0,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            },                                                  \
            {                                                   \
              .Fields.EfiReservedMemoryType           = 0,      \
              .Fields.EfiLoaderCode                   = 0,      \
              .Fields.EfiLoaderData                   = 0,      \
              .Fields.EfiBootServicesCode             = 0,      \
              .Fields.EfiBootServicesData             = 0,      \
              .Fields.EfiRuntimeServicesCode          = 0,      \
              .Fields.EfiRuntimeServicesData          = 0,      \
              .Fields.EfiConventionalMemory           = 0,      \
              .Fields.EfiUnusableMemory               = 0,      \
              .Fields.EfiACPIReclaimMemory            = 0,      \
              .Fields.EfiACPIMemoryNVS                = 0,      \
              .Fields.EfiMemoryMappedIO               = 0,      \
              .Fields.EfiMemoryMappedIOPortSpace      = 0,      \
              .Fields.EfiPalCode                      = 0,      \
              .Fields.EfiPersistentMemory             = 0,      \
              .Fields.OEMReserved                     = 0,      \
              .Fields.OSReserved                      = 0       \
            }                                                   \
          }

#endif
