/**@file
Library for defining PEI memory buckets. This keeps track of the
different buckets and the data for the associated HOB.
This library should not be called by anything outside of the PEI CORE.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PeiMain.h>
#include "MemoryBuckets.h"

// Memory types being kept in buckets in PEI.  Currently only runtime types.
EFI_MEMORY_TYPE  mMemoryTypes[PEI_BUCKETS] = {
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS
};

// Enum for different bucket types to make code easier to understand
enum {
  RuntimeCode,
  RuntimeData,
  ACPIReclaimMemory,
  ACPIMemoryNVS
};

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
  )
{
  UINT64  TotalBucketPages;

  TotalBucketPages = 0;
  PrivateData->PeiMemoryBuckets.RuntimeBuckets[RuntimeCode].NumberOfPages = FixedPcdGet8 (PcdPeiMemoryBucketRuntimeCode);
  TotalBucketPages                                    += PrivateData->PeiMemoryBuckets.RuntimeBuckets[RuntimeCode].NumberOfPages;
  PrivateData->PeiMemoryBuckets.RuntimeBuckets[RuntimeData].NumberOfPages = FixedPcdGet8 (PcdPeiMemoryBucketRuntimeData);
  TotalBucketPages                                    += PrivateData->PeiMemoryBuckets.RuntimeBuckets[RuntimeData].NumberOfPages;
  PrivateData->PeiMemoryBuckets.RuntimeBuckets[ACPIReclaimMemory].NumberOfPages = FixedPcdGet8 (PcdPeiMemoryBucketAcpiReclaimMemory);
  TotalBucketPages                                    += PrivateData->PeiMemoryBuckets.RuntimeBuckets[ACPIReclaimMemory].NumberOfPages;
  PrivateData->PeiMemoryBuckets.RuntimeBuckets[ACPIMemoryNVS].NumberOfPages = FixedPcdGet8 (PcdPeiMemoryBucketAcpiMemoryNvs);
  TotalBucketPages                                    += PrivateData->PeiMemoryBuckets.RuntimeBuckets[ACPIMemoryNVS].NumberOfPages;

  PrivateData->PeiMemoryBuckets.MemoryBucketsDisabled = FALSE;
  // Disable memory buckets if the PCDs are unaltered.
  if (TotalBucketPages == 0) {
    PrivateData->PeiMemoryBuckets.MemoryBucketsDisabled = TRUE;
  }
}

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
  IN EFI_PHYSICAL_ADDRESS  StartingAddress
  )
{
  UINTN   Index;
  UINT32  AddressAdjustment;

  AddressAdjustment = 0;

  for (Index = 0; Index < PEI_BUCKETS; Index++) {
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].BaseAddress = 0;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].MaximumAddress = MAX_ALLOC_ADDRESS;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].CurrentNumberOfPages = 0;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].NumberOfPages = 0;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].InformationIndex = mMemoryTypes[Index];
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].Special = TRUE;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].Runtime = TRUE;
  }

  InitializeMemoryBucketSizes (PrivateData);

  for (Index = 0; Index < PEI_BUCKETS; Index++) {
    // Initialize memory locations for buckets
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].BaseAddress = StartingAddress - AddressAdjustment;
    PrivateData->PeiMemoryBuckets.CurrentTopInBucket[Index]          = StartingAddress - AddressAdjustment;

    if (PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].NumberOfPages != 0) {
      AddressAdjustment += EFI_PAGE_SIZE *
                        ((UINT32)PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].NumberOfPages);
    }
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
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  UINTN  Index;

  switch (MemoryType) {
    case EfiRuntimeServicesCode:
      Index = RuntimeCode;
      break;
    case EfiRuntimeServicesData:
      Index = RuntimeData;
      break;
    case EfiACPIReclaimMemory:
      Index = ACPIReclaimMemory;
      break;
    case EfiACPIMemoryNVS:
      Index = ACPIMemoryNVS;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "[%a] - We got an incorrect MemoryType\n", __FUNCTION__));
      ASSERT (FALSE);
      Index = 4;
  }

  return Index;
}

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
  )
{
  UINTN  Index;

  Index = MemoryTypeToIndex (MemoryType);

  PrivateData->PeiMemoryBuckets.CurrentTopInBucket[Index] = NewTop;
}

/**
  This function gets the address of the startfree memory in the bucket
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
  )
{
  UINTN  Index;

  Index = MemoryTypeToIndex (MemoryType);

  return PrivateData->PeiMemoryBuckets.CurrentTopInBucket[Index];
}

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
  )
{
  EFI_PHYSICAL_ADDRESS  ReturnValue;
  UINTN                 Index;

  Index = MemoryTypeToIndex (MemoryType);

  ReturnValue = (EFI_PHYSICAL_ADDRESS)(PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].BaseAddress) -
                                        (EFI_PAGE_SIZE * PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].NumberOfPages);
  return ReturnValue;
}

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
  )
{
  UINTN  Index;

  for (Index = PEI_BUCKETS-1; Index >= 0; Index--) {
    if (PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].NumberOfPages > 0) {
      return GetCurrentBucketBottom (PrivateData, mMemoryTypes[Index]);
    }
  }

  DEBUG ((DEBUG_ERROR, "[%a] - We should not get here.\n", __FUNCTION__));
  return 0;
}

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
  )
{
  UINTN  Index;

  Index = MemoryTypeToIndex (MemoryType);

  if (PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].CurrentNumberOfPages + (UINT64)Pages
      > PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].NumberOfPages) {
    DEBUG ((DEBUG_ERROR, "We have overflowed while allocating PEI pages of index: %d!\n", Index));
    ASSERT (FALSE);
    return;
  }

  PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].CurrentNumberOfPages = (UINT64)PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].CurrentNumberOfPages + (UINT64)Pages;
}

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
  )
{
  // There is no boundary if the buckets are disabled
  if (PrivateData->PeiMemoryBuckets.MemoryBucketsDisabled) {
    return FALSE;
  }

  if (PrivateData->PeiMemoryBuckets.RuntimeMemInitialized &&
      ((Start >= GetBottomOfBucketsAddress (PrivateData)) &&
       (Start <= PrivateData->PeiMemoryBuckets.RuntimeBuckets[RuntimeCode].BaseAddress)))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Sets the RuntimeMemInitialized variable to TRUE.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
**/
VOID
EFIAPI
InitializeRuntimeMemoryBuckets (
  IN PEI_CORE_INSTANCE     *PrivateData
  )
{
  PrivateData->PeiMemoryBuckets.RuntimeMemInitialized = TRUE;
}

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
  )
{
  return PrivateData->PeiMemoryBuckets.RuntimeMemInitialized;
}

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
  IN       EFI_MEMORY_TYPE  MemoryType
  )
{
  UINTN  Index;

  // If Runtime Buckets are disabled then deny memory bucket operations.
  if (PrivateData->PeiMemoryBuckets.MemoryBucketsDisabled) {
    return FALSE;
  }

  for (Index = 0; Index < PEI_BUCKETS; Index++) {
    if (MemoryType == mMemoryTypes[Index]) {
      return TRUE;
    }
  }

  return FALSE;
}

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
  )
{
  if (PrivateData->PeiMemoryBuckets.MemoryBucketsDisabled) {
    return FALSE;
  }

  return TRUE;
}
