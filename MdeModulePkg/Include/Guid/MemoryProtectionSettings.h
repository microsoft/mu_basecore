/** @file

Defines memory protection settings guid and struct

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEMORY_PROTECTION_SETTINGS_H__
#define __MEMORY_PROTECTION_SETTINGS_H__

typedef struct {
  UINT8   UefiNullDetection   : 1;
  UINT8   SmmNullDetection    : 1;
  UINT8   NonstopMode         : 1;
  UINT8   DisableEndOfDxe     : 1;
  UINT8   DisableReadyToBoot  : 1;
} NULL_DETECTION_POLICY;

typedef struct {
  UINT8 UefiPageGuard         : 1;
  UINT8 UefiPoolGuard         : 1;
  UINT8 SmmPageGuard          : 1;
  UINT8 SmmPoolGuard          : 1;
  UINT8 UefiFreedMemoryGuard  : 1;
  UINT8 NonstopMode           : 1;
  UINT8 Direction             : 1;
} HEAP_GUARD_POLICY;

typedef struct {
  UINT8 EfiReservedMemoryType      : 1;
  UINT8 EfiLoaderCode              : 1;
  UINT8 EfiLoaderData              : 1;
  UINT8 EfiBootServicesCode        : 1;
  UINT8 EfiBootServicesData        : 1;
  UINT8 EfiRuntimeServicesCode     : 1;
  UINT8 EfiRuntimeServicesData     : 1;
  UINT8 EfiConventionalMemory      : 1;
  UINT8 EfiUnusableMemory          : 1;
  UINT8 EfiACPIReclaimMemory       : 1;
  UINT8 EfiACPIMemoryNVS           : 1;
  UINT8 EfiMemoryMappedIO          : 1;
  UINT8 EfiMemoryMappedIOPortSpace : 1;
  UINT8 EfiPalCode                 : 1;
  UINT8 EfiPersistentMemory        : 1;
  UINT8 OEMReserved                : 1;
  UINT8 OSReserved                 : 1;
} HEAP_GUARD_MEMORY_TYPES;

typedef struct {
  UINT8 FromUnknown     : 1;
  UINT8 FromFv          : 1;
} IMAGE_PROTECTION_POLICY;

//
// Memory Protection Settings struct
//
typedef struct {
  // Indicates if to set NX for stack.
  // This is assumed true in PEI forcing page tables to always be built. DXE will reuse the stack
  // allocated in PEI if this and/or CpuStackGuard are set to TRUE.
  //
  //  TRUE  - Set NX for stack.
  //  FALSE - Do nothing for stack.
  //
  // Note: If this value is set to FALSE, NX could be still applied to stack due to 
  //  DxeNxProtectionPolicy.EfiBootServicesData.
  BOOLEAN  SetNxForStack;

  // Indicates if UEFI Stack Guard will be enabled.
  //
  // If enabled, stack overflow in UEFI can be caught.
  //  TRUE  - UEFI Stack Guard will be enabled.
  //  FALSE - UEFI Stack Guard will be disabled.
  BOOLEAN  CpuStackGuard;

  // Bitfield to control the NULL address detection in code for different phases.
  // If enabled, accessing NULL address in UEFI or SMM code can be caught by marking
  // page zero as not present.
  //   .UefiNullDetection   : Enable NULL pointer detection for UEFI.
  //   .SmmNullDetection    : Enable NULL pointer detection for SMM.
  //   .NonstopMode         : Enable non-stop mode.
  //   .DisableEndOfDxe     : Disable NULL pointer detection just after EndOfDxe.
  //                          This is a workaround for those unsolvable NULL access issues in
  //                          OptionROM, boot loader, etc. It can also help to avoid unnecessary
  //                          exception caused by legacy memory (0-4095) access after EndOfDxe,
  //                          such as Windows 7 boot on Qemu.
  //   .DisableReadyToBoot  : Disable NULL pointer detection just after ReadyToBoot.
  NULL_DETECTION_POLICY    NullPointerDetectionPolicy;

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
  //  .SmmPageGuard          : Enable SMM page guard.
  //  .SmmPoolGuard          : Enable SMM pool guard.
  //  .UefiFreedMemoryGuard  : Enable UEFI freed-memory guard (Use-After-Free memory detection).
  //  .NonstopMode           : Enable non-stop mode.
  //  .Direction             : The direction of Guard Page for Pool Guard.
  //                           0 - The returned pool is near the tail guard page.
  //                           1 - The returned pool is near the head guard page.
  HEAP_GUARD_POLICY    HeapGuardPolicy;

  // Set image protection policy. The policy is bitwise.
  //
  // If a bit is set, the image will be protected by DxeCore if it is aligned.
  // The code section becomes read-only, and the data section becomes non-executable.
  // If a bit is clear, nothing will be done to image code/data sections.
  //  .FromUnknown  - Image from unknown device. 
  //  .FromFv       - Image from firmware volume.
  // 
  // Note: If a bit is cleared, the data section could be still non-executable if
  // DxeNxProtectionPolicy is enabled for EfiLoaderData, EfiBootServicesData
  // and/or EfiRuntimeServicesData.
  IMAGE_PROTECTION_POLICY   ImageProtectionPolicy;

  // Indicates which type allocation need guard page.
  //
  // If bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages which the allocated pool occupies,
  // if there's enough free memory for all of them. The pool allocation for the
  // type related to cleared bits keeps the same as ususal.
  //
  // This bitfield is only valid if UefiPoolGuard and/or SmmPoolGuard are set in HeapGuardPolicy.
  HEAP_GUARD_MEMORY_TYPES   HeapGuardPoolType;

  // Indicates which type allocation need guard page.
  //
  // If a bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages allocated if there's enough
  // free pages for all of them. The page allocation for the type related to
  // cleared bits keeps the same as ususal.
  //
  // This bitfield is only valid if UefiPageGuard and/or SmmPageGuard are set in HeapGuardPolicy.
  HEAP_GUARD_MEMORY_TYPES   HeapGuardPageType;

  // DXE no execute memory protection policy.
  //
  // If a bit is set, memory regions of the associated type will be mapped
  // non-executable. If a bit is cleared, nothing will be done to associated type of memory.
  //
  //  NOTE: - User must NOT set NX protection for EfiLoaderCode / EfiBootServicesCode / EfiRuntimeServicesCode.
  //        - User MUST set the same NX protection for EfiBootServicesData and EfiConventionalMemory.
  HEAP_GUARD_MEMORY_TYPES   DxeNxProtectionPolicy;
} MEMORY_PROTECTION_SETTINGS;

#define HOB_MEMORY_PROTECTION_SETTINGS_GUID \
  { \
    { 0x9ABFD639, 0xD1D0, 0x4EFF, { 0xBD, 0xB6, 0x7E, 0xC4, 0x19, 0x0D, 0x17, 0xD5 } } \
  }

extern GUID gMemoryProtectionSettingsGuid;

// HeapGuardPolicy.Direction value indicating tail alignment
#define HEAP_GUARD_ALIGNED_TO_TAIL 0

// HeapGuardPolicy.Direction value indicating head alignment
#define HEAP_GUARD_ALIGNED_TO_HEAD 1

//
//  A memory profile for "standard" memory protection settings.
//
#define MEMORY_PROTECTION_SETTINGS_STANDARD                       \
          {                                                       \
            TRUE,   /* NX Bit Set for Stack */                    \
            TRUE,   /* Stack Guard On */                          \
            {                                                     \
              .UefiNullDetection          = 1,                    \
              .SmmNullDetection           = 1,                    \
              .NonstopMode                = 0,                    \
              .DisableEndOfDxe            = 0,                    \
              .DisableReadyToBoot         = 0                     \
            },                                                    \
            {                                                     \
              .UefiPageGuard              = 1,                    \
              .UefiPoolGuard              = 1,                    \
              .SmmPageGuard               = 0,                    \
              .SmmPoolGuard               = 0,                    \
              .UefiFreedMemoryGuard       = 0,                    \
              .NonstopMode                = 0,                    \
              .Direction                  = 0                     \
            },                                                    \
            {                                                     \
              .FromUnknown                = 0,                    \
              .FromFv                     = 1                     \
            },                                                    \
            {                                                     \
              .EfiReservedMemoryType      = 0,                    \
              .EfiLoaderCode              = 0,                    \
              .EfiLoaderData              = 0,                    \
              .EfiBootServicesCode        = 0,                    \
              .EfiBootServicesData        = 1,                    \
              .EfiRuntimeServicesCode     = 0,                    \
              .EfiRuntimeServicesData     = 1,                    \
              .EfiConventionalMemory      = 0,                    \
              .EfiUnusableMemory          = 0,                    \
              .EfiACPIReclaimMemory       = 0,                    \
              .EfiACPIMemoryNVS           = 0,                    \
              .EfiMemoryMappedIO          = 0,                    \
              .EfiMemoryMappedIOPortSpace = 0,                    \
              .EfiPalCode                 = 0,                    \
              .EfiPersistentMemory        = 0,                    \
              .OEMReserved                = 0,                    \
              .OSReserved                 = 0                     \
            },                                                    \
            {                                                     \
              .EfiReservedMemoryType      = 0,                    \
              .EfiLoaderCode              = 0,                    \
              .EfiLoaderData              = 0,                    \
              .EfiBootServicesCode        = 0,                    \
              .EfiBootServicesData        = 1,                    \
              .EfiRuntimeServicesCode     = 0,                    \
              .EfiRuntimeServicesData     = 1,                    \
              .EfiConventionalMemory      = 0,                    \
              .EfiUnusableMemory          = 0,                    \
              .EfiACPIReclaimMemory       = 0,                    \
              .EfiACPIMemoryNVS           = 0,                    \
              .EfiMemoryMappedIO          = 0,                    \
              .EfiMemoryMappedIOPortSpace = 0,                    \
              .EfiPalCode                 = 0,                    \
              .EfiPersistentMemory        = 0,                    \
              .OEMReserved                = 0,                    \
              .OSReserved                 = 0                     \
            },                                                    \
            {                                                     \
              .EfiReservedMemoryType      = 1,                    \
              .EfiLoaderCode              = 0,                    \
              .EfiLoaderData              = 1,                    \
              .EfiBootServicesCode        = 0,                    \
              .EfiBootServicesData        = 1,                    \
              .EfiRuntimeServicesCode     = 0,                    \
              .EfiRuntimeServicesData     = 1,                    \
              .EfiConventionalMemory      = 1,                    \
              .EfiUnusableMemory          = 1,                    \
              .EfiACPIReclaimMemory       = 1,                    \
              .EfiACPIMemoryNVS           = 1,                    \
              .EfiMemoryMappedIO          = 1,                    \
              .EfiMemoryMappedIOPortSpace = 1,                    \
              .EfiPalCode                 = 1,                    \
              .EfiPersistentMemory        = 1,                    \
              .OEMReserved                = 0,                    \
              .OSReserved                 = 0                     \
            }                                                     \
          }
  
//
//  A memory profile for "loose" memory protection settings. Mirrors the strict
//  profile minus page and pool guards.
//
#define MEMORY_PROTECTION_SETTINGS_LOOSE                          \
          {                                                       \
            TRUE,   /* NX Bit Set for Stack */                    \
            TRUE,   /* Stack Guard On */                          \
            {                                                     \
              .UefiNullDetection          = 1,                    \
              .SmmNullDetection           = 1,                    \
              .NonstopMode                = 0,                    \
              .DisableEndOfDxe            = 0,                    \
              .DisableReadyToBoot         = 0                     \
            },                                                    \
            {                                                     \
              .UefiPageGuard              = 0,                    \
              .UefiPoolGuard              = 0,                    \
              .SmmPageGuard               = 0,                    \
              .SmmPoolGuard               = 0,                    \
              .UefiFreedMemoryGuard       = 0,                    \
              .NonstopMode                = 0,                    \
              .Direction                  = 0                     \
            },                                                    \
            {                                                     \
              .FromUnknown                = 0,                    \
              .FromFv                     = 1                     \
            },                                                    \
            {                                                     \
              .EfiReservedMemoryType      = 0,                    \
              .EfiLoaderCode              = 0,                    \
              .EfiLoaderData              = 0,                    \
              .EfiBootServicesCode        = 0,                    \
              .EfiBootServicesData        = 0,                    \
              .EfiRuntimeServicesCode     = 0,                    \
              .EfiRuntimeServicesData     = 0,                    \
              .EfiConventionalMemory      = 0,                    \
              .EfiUnusableMemory          = 0,                    \
              .EfiACPIReclaimMemory       = 0,                    \
              .EfiACPIMemoryNVS           = 0,                    \
              .EfiMemoryMappedIO          = 0,                    \
              .EfiMemoryMappedIOPortSpace = 0,                    \
              .EfiPalCode                 = 0,                    \
              .EfiPersistentMemory        = 0,                    \
              .OEMReserved                = 0,                    \
              .OSReserved                 = 0                     \
            },                                                    \
            {                                                     \
              .EfiReservedMemoryType      = 0,                    \
              .EfiLoaderCode              = 0,                    \
              .EfiLoaderData              = 0,                    \
              .EfiBootServicesCode        = 0,                    \
              .EfiBootServicesData        = 0,                    \
              .EfiRuntimeServicesCode     = 0,                    \
              .EfiRuntimeServicesData     = 0,                    \
              .EfiConventionalMemory      = 0,                    \
              .EfiUnusableMemory          = 0,                    \
              .EfiACPIReclaimMemory       = 0,                    \
              .EfiACPIMemoryNVS           = 0,                    \
              .EfiMemoryMappedIO          = 0,                    \
              .EfiMemoryMappedIOPortSpace = 0,                    \
              .EfiPalCode                 = 0,                    \
              .EfiPersistentMemory        = 0,                    \
              .OEMReserved                = 0,                    \
              .OSReserved                 = 0                     \
            },                                                    \
            {                                                     \
              .EfiReservedMemoryType      = 1,                    \
              .EfiLoaderCode              = 0,                    \
              .EfiLoaderData              = 1,                    \
              .EfiBootServicesCode        = 0,                    \
              .EfiBootServicesData        = 1,                    \
              .EfiRuntimeServicesCode     = 0,                    \
              .EfiRuntimeServicesData     = 1,                    \
              .EfiConventionalMemory      = 1,                    \
              .EfiUnusableMemory          = 1,                    \
              .EfiACPIReclaimMemory       = 1,                    \
              .EfiACPIMemoryNVS           = 1,                    \
              .EfiMemoryMappedIO          = 1,                    \
              .EfiMemoryMappedIOPortSpace = 1,                    \
              .EfiPalCode                 = 1,                    \
              .EfiPersistentMemory        = 1,                    \
              .OEMReserved                = 0,                    \
              .OSReserved                 = 0                     \
            }                                                     \
          }

//
//  A memory profile which disables memory protection settings.
//
#define MEMORY_PROTECTION_SETTINGS_OOF                            \
          {                                                       \
            FALSE,   /* NX Bit Set for Stack */                   \
            FALSE,   /* Stack Guard On */                         \
            {                                                     \
              .UefiNullDetection          = 0,                    \
              .SmmNullDetection           = 0,                    \
              .NonstopMode                = 0,                    \
              .DisableEndOfDxe            = 0,                    \
              .DisableReadyToBoot         = 0                     \
            },                                                    \
            {                                                     \
              .UefiPageGuard              = 0,                    \
              .UefiPoolGuard              = 0,                    \
              .SmmPageGuard               = 0,                    \
              .SmmPoolGuard               = 0,                    \
              .UefiFreedMemoryGuard       = 0,                    \
              .NonstopMode                = 0,                    \
              .Direction                  = 0                     \
            },                                                    \
            {                                                     \
              .FromUnknown                = 0,                    \
              .FromFv                     = 0                     \
            },                                                    \
            {                                                     \
              .EfiReservedMemoryType      = 0,                    \
              .EfiLoaderCode              = 0,                    \
              .EfiLoaderData              = 0,                    \
              .EfiBootServicesCode        = 0,                    \
              .EfiBootServicesData        = 0,                    \
              .EfiRuntimeServicesCode     = 0,                    \
              .EfiRuntimeServicesData     = 0,                    \
              .EfiConventionalMemory      = 0,                    \
              .EfiUnusableMemory          = 0,                    \
              .EfiACPIReclaimMemory       = 0,                    \
              .EfiACPIMemoryNVS           = 0,                    \
              .EfiMemoryMappedIO          = 0,                    \
              .EfiMemoryMappedIOPortSpace = 0,                    \
              .EfiPalCode                 = 0,                    \
              .EfiPersistentMemory        = 0,                    \
              .OEMReserved                = 0,                    \
              .OSReserved                 = 0                     \
            },                                                    \
            {                                                     \
              .EfiReservedMemoryType      = 0,                    \
              .EfiLoaderCode              = 0,                    \
              .EfiLoaderData              = 0,                    \
              .EfiBootServicesCode        = 0,                    \
              .EfiBootServicesData        = 0,                    \
              .EfiRuntimeServicesCode     = 0,                    \
              .EfiRuntimeServicesData     = 0,                    \
              .EfiConventionalMemory      = 0,                    \
              .EfiUnusableMemory          = 0,                    \
              .EfiACPIReclaimMemory       = 0,                    \
              .EfiACPIMemoryNVS           = 0,                    \
              .EfiMemoryMappedIO          = 0,                    \
              .EfiMemoryMappedIOPortSpace = 0,                    \
              .EfiPalCode                 = 0,                    \
              .EfiPersistentMemory        = 0,                    \
              .OEMReserved                = 0,                    \
              .OSReserved                 = 0                     \
            },                                                    \
            {                                                     \
              .EfiReservedMemoryType      = 0,                    \
              .EfiLoaderCode              = 0,                    \
              .EfiLoaderData              = 0,                    \
              .EfiBootServicesCode        = 0,                    \
              .EfiBootServicesData        = 0,                    \
              .EfiRuntimeServicesCode     = 0,                    \
              .EfiRuntimeServicesData     = 0,                    \
              .EfiConventionalMemory      = 0,                    \
              .EfiUnusableMemory          = 0,                    \
              .EfiACPIReclaimMemory       = 0,                    \
              .EfiACPIMemoryNVS           = 0,                    \
              .EfiMemoryMappedIO          = 0,                    \
              .EfiMemoryMappedIOPortSpace = 0,                    \
              .EfiPalCode                 = 0,                    \
              .EfiPersistentMemory        = 0,                    \
              .OEMReserved                = 0,                    \
              .OSReserved                 = 0                     \
            }                                                     \
          }

#endif
