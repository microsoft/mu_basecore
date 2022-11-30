/** @file

Library for defining PEI memory buckets. This keeps track of the
different buckets and the data for the associated HOB.
For internal use only.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEMORY_BUCKET_HELPER_LIB_H__
#define __MEMORY_BUCKET_HELPER_LIB_H__

#include <Uefi.h>
#include <Base.h>
#include <Guid/MemoryBucketInformation.h>

/**
  This function initialized the PEI memory buckets.  This should be called
  when the first memory type being tracked in these buckets is allocated.

  @param[in] StartingAddress   The starting address of the memory buckets.
                               All of them are contiguous.
**/
VOID
EFIAPI
InitializeMemoryBuckets (
  IN EFI_PHYSICAL_ADDRESS  StartingAddress
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
  This function gets the address of the startfree memory in the bucket
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
  This function gets the address of the end of the bucket specified by
  MemoryType.

  @param[in] MemoryType   The type of memory we are interested in.

  @retval    Returns an address that points to the end of memory in a
             memory bucket that is specified by MemoryType
**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetCurrentBucketEnd (
  IN EFI_MEMORY_TYPE  MemoryType
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

VOID
MemoryBucketLibInitialize (
  VOID
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
CheckIfInRuntimeBoundaryInternal (
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
  This function returns the standard length of a PEI memory bucket.

  @retval   Returns the length of a PEI memory bucket.
**/
UINTN
EFIAPI
GetBucketLength (
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
IsRuntimeTypeInternal (
  IN EFI_MEMORY_TYPE  MemoryType
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
  IN VOID  *MemoryBuckets
  );

#endif
