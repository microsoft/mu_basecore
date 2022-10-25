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

EFI_STATUS
EFIAPI
PeiAllocateRuntimePages (
  IN       EFI_MEMORY_TYPE       MemoryType,
  IN       UINTN                 Pages,
  OUT      EFI_PHYSICAL_ADDRESS  *Memory
  );

BOOLEAN
EFIAPI
IsRuntimeType (
  IN       EFI_MEMORY_TYPE       MemoryType
  );

VOID
EFIAPI
SyncMemoryBuckets (
  VOID
  );

BOOLEAN
EFIAPI
CheckIfInRuntimeBoundary (
  EFI_PHYSICAL_ADDRESS Start
  );

EFI_PHYSICAL_ADDRESS
EFIAPI
GetEndOfBucketsAddress (
  VOID
  );

#endif
