/** @file

Library for defining PEI memory buckets. This keeps track of the
different buckets and the data for the associated HOB.
For internal use only.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEMORY_BUCKET_LIB_H__
#define __MEMORY_BUCKET_LIB_H__

#include <PeiMain.h>
#include <Guid/MemoryTypeStatistics.h>
#include <Guid/MemoryTypeInformation.h>

/**
  This function figures out the size of the memory buckets based on the PEI
  memory bucket PCDs.  If the PCDs are not set by the platform then the memory
  buckets are disabled.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
**/
VOID
EFIAPI
InitializeMemoryBucketSizes (
  IN PEI_CORE_INSTANCE     *PrivateData
  );

/**
  This function gets the number of pages associated with the memory type
  that is saved within the MEMORY_TYPE_INFORMATION Hob.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] DataSize      The size of the data in the MEMORY_TYPE_INFORMATION
                           hob.
  @param[in] MemInfo       Pointer the the MEMORY_TYPE_INFORMATION object
                           with the relevant memory bucket sizes.
  @param[in] MemoryType    The type of memory we are interested in.

  @retval    Returns the number of pages to use for the bucket in the memory
             memory type inputted.
**/
UINT32
EFIAPI
GetBucketSizeFromMemoryInfoHob (
  IN PEI_CORE_INSTANCE     *PrivateData,
  IN UINTN                  DataSize,
  IN EFI_MEMORY_TYPE_INFORMATION *MemInfo,
  IN EFI_MEMORY_TYPE       MemoryType
  );

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

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] StartingAddress   The starting address of the memory buckets.
                               All of them are contiguous.
**/
VOID
EFIAPI
InitializeMemoryBuckets (
  IN PEI_CORE_INSTANCE     *PrivateData,
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

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] NewTop       The new start of unallocated memory in a bucket.
  @param[in] MemoryType   The type of memory being updated.
**/
VOID
EFIAPI
UpdateCurrentBucketTop (
  IN PEI_CORE_INSTANCE     *PrivateData,
  IN EFI_PHYSICAL_ADDRESS  NewTop,
  IN EFI_MEMORY_TYPE       MemoryType
  );

/**
  This function gets the address of the start free memory in the bucket
  specified by MemoryType.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] MemoryType   The type of memory we are interested in.

  @retval    Returns an address that points to the start of free memory in a
             memory bucket that is specified by MemoryType
**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetCurrentBucketTop (
  IN PEI_CORE_INSTANCE     *PrivateData,
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  This function gets the address of the bottom of the bucket specified by
  MemoryType.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] MemoryType   The type of memory we are interested in.

  @retval    Returns an address that points to the bottom of memory in a
             memory bucket that is specified by MemoryType
**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetCurrentBucketBottom (
  IN PEI_CORE_INSTANCE     *PrivateData,
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Function that returns the address associated with the end of the
  memory bucket structure.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.

  @retval    The memory address below the memory bucket structure.

**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetBottomOfBucketsAddress (
  IN PEI_CORE_INSTANCE     *PrivateData
  );

/**
  This function updates the memory bucket structure to include a new memory allocation.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] Pages        The number of pages we are allocating in the bucket.
  @param[in] MemoryType   The type of memory we are updating in the memory bucket
                          structure.
**/
VOID
EFIAPI
UpdateRuntimeMemoryStats (
  IN PEI_CORE_INSTANCE     *PrivateData,
  IN UINTN            Pages,
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  This function checks to see if the given address lies within the currently defined
  runtime memory bucket range.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] Start   The start of the address we are trying to allocate memory in.

  @retval    TRUE    If the memory address is within the PEI memory bucket region.
  @retval    FALSE   If the memory address is not within the PEI memory bucket region.
**/
BOOLEAN
EFIAPI
CheckIfInRuntimeBoundary (
  IN PEI_CORE_INSTANCE     *PrivateData,
  IN EFI_PHYSICAL_ADDRESS  Start
  );

/**
  Sets the RuntimeMemInitialized variable to TRUE.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
**/
VOID
EFIAPI
InitializeRuntimeMemoryBuckets (
  IN PEI_CORE_INSTANCE     *PrivateData
  );

/**
  Checks if we have initialized PEI memory buckets.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.

  @retval   TRUE   We have initialized PEI memory buckets.
  @retval   FALSE  We have not initialized PEI memory buckets.
**/
BOOLEAN
EFIAPI
IsRuntimeMemoryInitialized (
  IN PEI_CORE_INSTANCE     *PrivateData
  );

/**
  This function checks if the memory type we are allocating is being kept
  track of within the PEI memory bucket structure.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] MemoryType   The type of memory we are interest in allocating.

  @retval    TRUE         The type of memory we are allocating is included
                          in the PEI memory bucket structure.
  @retval    FALSE        The type of memory we are allocating is not
                          included in the PEI memory bucket structure.
**/
BOOLEAN
EFIAPI
IsRuntimeType (
  IN PEI_CORE_INSTANCE     *PrivateData,
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Function that checks if PEI memory buckets are enabled.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.

  @retval    TRUE         PEI memory buckets are enabled
  @retval    FALSE        PEI memory buckets are disabled
**/
BOOLEAN
EFIAPI
AreMemoryBucketsEnabled (
  IN PEI_CORE_INSTANCE     *PrivateData
  );

/**
  Function that builds and updates the memory bucket hob that will be
  consumed in DXE.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
**/
VOID
EFIAPI
UpdateMemoryBucketHob (
  IN PEI_CORE_INSTANCE     *PrivateData
  );

#endif
