/** @file

Library for defining PEI memory buckets. This keeps track of the
different buckets and the data for the associated HOB.
For internal use only.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEMORY_BUCKET_LIB_H__
#define __MEMORY_BUCKET_LIB_H__

#include <Uefi.h>
#include <Base.h>
#include <Guid/MemoryBucketInformation.h>
#include <Guid/MemoryTypeStatistics.h>

/**
  This function initialized the PEI memory buckets.  This should be called
  when the first memory type being tracked in these buckets is allocated.

      +-----------------------------+  <-- Top of free memory
      |  Memory Bucket Allocation 1 |
      +-----------------------------+  <-- Top of free memory + 1st non-zero memory bucket pcd
      |  Memory Bucket Allocation 2 |
      +-----------------------------+  <-- Top of free memory + 1st and 2nd non-zero memory bucket pcds
      |  Memory Bucket Allocation 3 |
      +-----------------------------+  <-- Top of free memory + 1st, 2nd, 3rd non-zero memory bucket pcds
      |   . . . . . . . . . . . .   |
      +-----------------------------+  <-- Top of free memory + 1-n non-zero memory bucket pcds
      |         FREE MEMORY         |      For PEI allocations all new non bucket free memory starts here.
      +-----------------------------+

  NOTE: The memory buckets can be created in Pre-mem PEI if runtime pages are
  allocated in pre-mem PEI.

  @param[in] StartingAddress   The starting address of the memory buckets.
                               All of them are contiguous.
**/
VOID
EFIAPI
InitializeMemoryBuckets (
  EFI_PHYSICAL_ADDRESS  StartingAddress
  );

/**
  This function translates the memory type being allocated into the
  index that it uses in the memory buckets structure.

  @param[in] MemoryType   The type of memory we are interested in.

  @retval    The index associated with the memory type.
**/
UINTN
EFIAPI
MemoryTypeToIndex (
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  This function updates the pointer that keeps track of the start of unused
  memory in a bucket.

  @param[in] NewTop       The new start of unallocated memory in a bucket.
  @param[in] MemoryType   The type of memory being updated.
**/
VOID
EFIAPI
UpdateCurrentBucketTop (
  IN EFI_PHYSICAL_ADDRESS  NewTop,
  IN EFI_MEMORY_TYPE       MemoryType
  );

/**
  This function gets the address of the start free memory in the bucket
  specified by MemoryType.

  @param[in] MemoryType   The type of memory we are interested in.

  @retval    Returns an address that points to the start of free memory in a
             memory bucket that is specified by MemoryType
**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetCurrentBucketTop (
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  This function gets the address of the bottom of the bucket specified by
  MemoryType.

  @param[in] MemoryType   The type of memory we are interested in.

  @retval    Returns an address that points to the bottom of memory in a
             memory bucket that is specified by MemoryType
**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetCurrentBucketBottom (
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Function that returns the address associated with the end of the
  memory bucket structure.

  @retval    The memory address below the memory bucket structure.

**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetBottomOfBucketsAddress (
  VOID
  );

/**
  This function updates the memory bucket structure to include a new memory allocation.

  @param[in] Pages        The number of pages we are allocating in the bucket.
  @param[in] MemoryType   The type of memory we are updating in the memory bucket
                          structure.
**/
VOID
EFIAPI
UpdateRuntimeMemoryStats (
  IN UINTN            Pages,
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  This function checks to see if there is an attempt to allocate memory in the runtime
  memory buckets that we have defined.

  @param[in] Start   The start of the address we are trying to allocate memory in.

  @retval    TRUE    If the memory address is within the PEI memory bucket region.
  @retval    FALSE   If the memory address is not within the PEI memory bucket region.
**/
BOOLEAN
EFIAPI
CheckIfInRuntimeBoundary (
  IN EFI_PHYSICAL_ADDRESS  Start
  );

/**
  Sets the RuntimeMemInitialized variable to TRUE
**/
VOID
EFIAPI
InitializeRuntimeMemoryBuckets (
  VOID
  );

/**
  Checks if we have initialized PEI memory buckets.

  @retval   TRUE   We have initialized PEI memory buckets.
  @retval   FALSE  We have not initialized PEI memory buckets.
**/
BOOLEAN
EFIAPI
IsRuntimeMemoryInitialized (
  VOID
  );

/**
  This function checks if the memory type we are allocating is being kept
  track of within the PEI memory bucket structure.

  @param[in] MemoryType   The type of memory we are interest in allocating.

  @retval    TRUE         The type of memory we are allocating is included
                          in the PEI memory bucket structure.
  @retval    FALSE        The type of memory we are allocating is not
                          included in the PEI memory bucket structure.
**/
BOOLEAN
EFIAPI
IsRuntimeType (
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Function that checks if PEI memory buckets are enabled.

  @retval    TRUE         PEI memory buckets are enabled
  @retval    FALSE        PEI memory buckets are disabled
**/
BOOLEAN
EFIAPI
AreMemoryBucketsEnabled (
  VOID
  );

/**
  Internal function to build a HOB for the memory allocation.
  It will search and reuse the unused(freed) memory allocation HOB,
  or build memory allocation HOB normally if no unused(freed) memory allocation HOB found.

  @param[in] BaseAddress        The 64 bit physical address of the memory.
  @param[in] Length             The length of the memory allocation in bytes.
**/
VOID
BuildRuntimeMemoryAllocationInfoHob (
  VOID
  );

/**
  Function that makes pulls the memory bucket hob information locally
  if necessary.  This is so it can be more easily referenced.

  @param[in] MemBucketHob   A passed in hob that's associated with the
                            PEI_MEMORY_BUCKET_INFORMATION GUID.
**/
VOID
EFIAPI
SyncMemoryBuckets (
  IN VOID  *MemBucketHob
  );

/**
  This function returns the Memory Bucket Hob structure.

  @retval Returns the Memory Bucket Hob structure.
**/
PEI_MEMORY_BUCKET_INFORMATION
EFIAPI
GetRuntimeBucketHob (
  VOID
  );

/**
  This function gets the PEI memory bucket hob and populates the locally kept data here.
  This is to keep the memory buckets coherent.

  @param[in] MemoryBuckets   The data associated with the memory bucket hob.  This gets
                             the already allocated Memory Bucket Hob to keep coherency
                             with the PEI memory buckets.
**/
VOID
EFIAPI
SetMemoryBucketsFromHob (
  IN PEI_MEMORY_BUCKET_INFORMATION  *MemoryBuckets
  );

#endif
