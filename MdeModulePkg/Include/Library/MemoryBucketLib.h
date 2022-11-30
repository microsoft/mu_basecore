/** @file

Library for defining memory buckets.  Can be referenced externally.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEMORY_BUCKET_LIB_H__
#define __MEMORY_BUCKET_LIB_H__

#include <Uefi.h>
#include <Base.h>
#include <Guid/MemoryBucketInformation.h>

/**
  Internal function to build a HOB for the memory allocation.
  It will search and reuse the unused(freed) memory allocation HOB,
  or build memory allocation HOB normally if no unused(freed) memory allocation HOB found.

  @param[in] BaseAddress        The 64 bit physical address of the memory.
  @param[in] Length             The length of the memory allocation in bytes.
  @param[in] MemoryType         The type of memory allocated by this HOB.

**/
VOID
InternalBuildRuntimeMemoryAllocationHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN EFI_MEMORY_TYPE       MemoryType
  );

/**
  Internal function to build a HOB for the memory allocation.
  It will search and reuse the unused(freed) memory allocation HOB,
  or build memory allocation HOB normally if no unused(freed) memory allocation HOB found.

  @param[in] BaseAddress        The 64 bit physical address of the memory.
  @param[in] Length             The length of the memory allocation in bytes.

**/
VOID
InternalBuildRuntimeMemoryAllocationInfoHob (
  VOID
  );

/**
  This function allocates pages in the PEI memory bucket regions.
  When this is called for the first time the memory bucket ranges are initialized.

  If this is called prior to InstallPeiMemory() the memory bucket ranges will be
  on the heap.  If this is called after InstallPeiMemory() the memory bucket
  ranges will be in the region provided by InstallPeiMemory().

  @param  MemoryType       The type of memory to allocate.
  @param  Pages            The number of contiguous 4 KB pages to allocate.
  @param  Memory           Pointer to a physical address. On output, the address is set to the base
                           of the page range that was allocated.

  @retval EFI_SUCCESS           The memory range was successfully allocated.
  @retval EFI_OUT_OF_RESOURCES  The pages could not be allocated.

**/
EFI_STATUS
EFIAPI
PeiAllocateRuntimePages (
  IN       EFI_MEMORY_TYPE       MemoryType,
  IN       UINTN                 Pages,
  OUT      EFI_PHYSICAL_ADDRESS  *Memory
  );

/**
  Function to check if MemoryType is one of the types included in the PEI
  memory bucket structure.

  @param[in] MemoryType         The memory type we are checking.

  @retval    TRUE               The memory type is in the PEI memory bucket
                                structure.
  @retval    FALSE              The memory type is not in the PEI memory
                                bucket structure.

**/
BOOLEAN
EFIAPI
IsRuntimeType (
  IN       EFI_MEMORY_TYPE  MemoryType
  );

/**
  Function that makes pulls the memory bucket hob information locally
  if necessary.  This is so it can be more easily referenced.

**/
VOID
EFIAPI
SyncMemoryBuckets (
  VOID
  );

/**
  Function to check if a memory region is within the memory bucket structure.

  @param[in] Start              The start of the memory region we are checking

  @retval    TRUE               The region is in the memory bucket structure.
  @retval    FALSE              The region is not in the memory bucket structure.

**/
BOOLEAN
EFIAPI
CheckIfInRuntimeBoundary (
  IN EFI_PHYSICAL_ADDRESS  Start
  );

/**
  Function that returns the address associated with the end of the
  memory bucket structure.

  @retval    The memory address below the memory bucket structure.

**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetEndOfBucketsAddress (
  VOID
  );

#endif
