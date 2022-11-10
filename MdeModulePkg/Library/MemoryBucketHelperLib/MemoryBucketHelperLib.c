/**@file
Library for defining PEI memory buckets. This keeps track of the
different buckets and the data for the associated HOB.
For internal use only.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryBucketHelperLib.h>
#include <Library/BaseMemoryLib.h>

// Memory types being kept in buckets in PEI.  Currently only runtime types.
EFI_MEMORY_TYPE  MemoryTypes[4] = {
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS
};

// PEI memory bucket statistics.  Can be extended if necessary
EFI_MEMORY_TYPE_STATISTICS  RuntimeMemoryStats[4] = {
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiRuntimeServicesCode, TRUE, TRUE  },   // EfiRuntimeServicesCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiRuntimeServicesData, TRUE, TRUE  },   // EfiRuntimeServicesData
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiACPIReclaimMemory,   TRUE, FALSE },   // EfiACPIReclaimMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiACPIMemoryNVS,       TRUE, FALSE }    // EfiACPIMemoryNVS
};

EFI_PHYSICAL_ADDRESS  CurrentBucketTops[4] = {
  MAX_ALLOC_ADDRESS, // EfiRuntimeServicesCode
  MAX_ALLOC_ADDRESS, // EfiRuntimeServicesData
  MAX_ALLOC_ADDRESS, // EfiACPIReclaimMemory
  MAX_ALLOC_ADDRESS  // EfiACPIMemoryNVS
};

// Information storage for Hob
PEI_MEMORY_BUCKET_INFORMATION  RuntimeBucketHob;

// Number of pages that can fit into each memory bucket.
UINT32  RuntimeMemLength = 50;

// TRUE if we have initialized the runtime memory buckets.
BOOLEAN  RuntimeMemInitialized = FALSE;

/**
  This function initialized the PEI memory buckets.  This should be called
  when the first memory type being tracked in these buckets is allocated.

  @param[in] StartingAddress   The starting address of the memory buckets.
                               All of them are contiguous.
**/
VOID
EFIAPI
InitializeMemoryBuckets (
  EFI_PHYSICAL_ADDRESS  StartingAddress
  )
{
  UINTN   Index;
  UINT32  AddressAdjustment;

  for (Index = 0; Index < NumberOfBuckets; Index++) {
    // Initialize memory locations for buckets
    AddressAdjustment                     = EFI_PAGE_SIZE * RuntimeMemLength * (UINT32)Index;
    RuntimeMemoryStats[Index].BaseAddress = StartingAddress - AddressAdjustment;
    CurrentBucketTops[Index]              = StartingAddress - AddressAdjustment;

    // Initialize Hob to be able to reference it later
    RuntimeBucketHob.RuntimeBuckets[Index]     = RuntimeMemoryStats[Index];
    RuntimeBucketHob.CurrentTopInBucket[Index] = CurrentBucketTops[Index];
  }
}

/**
  This function translates the memory type being allocated into the
  index that it uses in the memory buckets structure.

  @param[in] MemoryType   The type of memory we are interested in.

  @retval    The index associated with the memory type.
**/
UINTN
EFIAPI
MemoryTypeToIndex (
  EFI_MEMORY_TYPE  MemoryType
  )
{
  UINTN  Index;

  switch (MemoryType) {
    case EfiRuntimeServicesCode:
      Index = 0;
      break;
    case EfiRuntimeServicesData:
      Index = 1;
      break;
    case EfiACPIReclaimMemory:
      Index = 2;
      break;
    case EfiACPIMemoryNVS:
      Index = 3;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "We got an incorrect MemoryType\n"));
      ASSERT (FALSE);
      Index = 4;
  }

  return Index;
}

/**
  This function updates the pointer that keeps track of the start of unused
  memory in a bucket.

  @param[in] NewTop       The new start of unallocated memory in a bucket.
  @param[in] MemoryType   The type of memory being updated.
**/
VOID
EFIAPI
UpdateCurrentBucketTop (
  EFI_PHYSICAL_ADDRESS  NewTop,
  EFI_MEMORY_TYPE       MemoryType
  )
{
  UINTN  Index;

  Index = MemoryTypeToIndex (MemoryType);

  CurrentBucketTops[Index] = NewTop;
}

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
  EFI_MEMORY_TYPE  MemoryType
  )
{
  UINTN  Index;

  Index = MemoryTypeToIndex (MemoryType);

  return CurrentBucketTops[Index];
}

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
  EFI_MEMORY_TYPE  MemoryType
  )
{
  EFI_PHYSICAL_ADDRESS  ReturnValue;
  UINTN                 Index;

  Index = MemoryTypeToIndex (MemoryType);

  ReturnValue = (EFI_PHYSICAL_ADDRESS)(RuntimeMemoryStats[Index].BaseAddress) - (EFI_PAGE_SIZE * RuntimeMemLength);
  return ReturnValue;
}

/**
  This function updates the memory bucket structure to include a new memory allocation.

  @param[in] Pages        The number of pages we are allocating in the bucket.
  @param[in] MemoryType   The type of memory we are updating in the memory bucket
                          structure.
**/
VOID
EFIAPI
UpdateRuntimeMemoryStats (
  UINTN            Pages,
  EFI_MEMORY_TYPE  MemoryType
  )
{
  UINTN  Index;

  Index = MemoryTypeToIndex (MemoryType);

  if (RuntimeMemoryStats[Index].NumberOfPages + (UINT64)Pages > RuntimeMemLength) {
    DEBUG ((DEBUG_ERROR, "We have overflowed while allocating PEI pages of index: %d!\n", Index));
    ASSERT (FALSE);
    return;
  }

  RuntimeMemoryStats[Index].NumberOfPages        = (UINT64)RuntimeMemoryStats[Index].NumberOfPages + (UINT64)Pages;
  RuntimeMemoryStats[Index].CurrentNumberOfPages = RuntimeMemoryStats[Index].NumberOfPages;

  RuntimeBucketHob.RuntimeBuckets[Index]     = RuntimeMemoryStats[Index];
  RuntimeBucketHob.CurrentTopInBucket[Index] = CurrentBucketTops[Index];
}

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
  EFI_PHYSICAL_ADDRESS  Start
  )
{
  if (RuntimeMemInitialized &&
      ((Start >= (RuntimeMemoryStats[0].BaseAddress - (NumberOfBuckets * EFI_PAGE_SIZE * RuntimeMemLength))) &&
       (Start <= RuntimeMemoryStats[0].BaseAddress)))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Sets the RuntimeMemInitialized variable to TRUE
**/
VOID
EFIAPI
InitializeRuntimeMemoryBuckets (
  VOID
  )
{
  RuntimeMemInitialized = TRUE;
}

/**
  Checks if we have initialized PEI memory buckets.

  @retval   TRUE   We have initialized PEI memory buckets.
  @retval   FALSE  We have not initialized PEI memory buckets.
**/
BOOLEAN
EFIAPI
IsRuntimeMemoryInitialized (
  VOID
  )
{
  return RuntimeMemInitialized;
}

/**
  This function returns the standard length of a PEI memory bucket.

  @retval   Returns the length of a PEI memory bucket.
**/
UINTN
EFIAPI
GetBucketLength (
  VOID
  )
{
  return RuntimeMemLength;
}

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
  IN       EFI_MEMORY_TYPE  MemoryType
  )
{
  UINTN  Index;

  for (Index = 0; Index < NumberOfBuckets; Index++) {
    if (MemoryType == MemoryTypes[Index]) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  This function returns the Memory Bucket Hob structure.

  @retval Returns the Memory Bucket Hob structure.
**/
PEI_MEMORY_BUCKET_INFORMATION
EFIAPI
GetRuntimeBucketHob (
  VOID
  )
{
  return RuntimeBucketHob;
}

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
  VOID  *MemoryBuckets
  )
{
  PEI_MEMORY_BUCKET_INFORMATION  *TempStats;
  UINTN                          Index;

  if (MemoryBuckets == NULL) {
    DEBUG ((DEBUG_ERROR, "NO PEI RUNTIME MEMORY BUCKETS YET\n"));
    return;
  }

  TempStats = (PEI_MEMORY_BUCKET_INFORMATION *)MemoryBuckets;
  for (Index = 0; Index < NumberOfBuckets; Index++) {
    RuntimeMemoryStats[Index] = TempStats->RuntimeBuckets[Index];
    CurrentBucketTops[Index]  = TempStats->CurrentTopInBucket[Index];
  }

  RuntimeMemInitialized = TRUE;
}
