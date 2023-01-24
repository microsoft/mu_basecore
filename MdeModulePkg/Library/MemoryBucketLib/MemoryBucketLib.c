/**@file
Library for defining PEI memory buckets. This keeps track of the
different buckets and the data for the associated HOB.
For internal use only.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/MemoryBucketLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>

// Memory types being kept in buckets in PEI.  Currently only runtime types.
EFI_MEMORY_TYPE  mMemoryTypes[PEI_BUCKETS] = {
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS
};

enum {
  RuntimeCode,
  RuntimeData,
  ACPIReclaimMemory,
  ACPIMemoryNVS
};

// PEI memory bucket statistics.  Can be extended if necessary
EFI_MEMORY_TYPE_STATISTICS  mRuntimeMemoryStats[PEI_BUCKETS] = {
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiRuntimeServicesCode, TRUE, TRUE  },   // EfiRuntimeServicesCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiRuntimeServicesData, TRUE, TRUE  },   // EfiRuntimeServicesData
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiACPIReclaimMemory,   TRUE, FALSE },   // EfiACPIReclaimMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiACPIMemoryNVS,       TRUE, FALSE }    // EfiACPIMemoryNVS
};

EFI_PHYSICAL_ADDRESS  mCurrentBucketTops[PEI_BUCKETS] = {
  MAX_ALLOC_ADDRESS, // EfiRuntimeServicesCode
  MAX_ALLOC_ADDRESS, // EfiRuntimeServicesData
  MAX_ALLOC_ADDRESS, // EfiACPIReclaimMemory
  MAX_ALLOC_ADDRESS  // EfiACPIMemoryNVS
};

// Information storage for Hob
PEI_MEMORY_BUCKET_INFORMATION  mRuntimeBucketHob;

// TRUE if the memory buckets are currently allocated in temp memory.
BOOLEAN  mPreMemPeiAllocation = FALSE;

// TRUE if PEI memory buckets are disabled
BOOLEAN  mMemoryBucketsDisabled = FALSE;

// TRUE if we have initialized the runtime memory buckets.
BOOLEAN  mRuntimeMemInitialized = FALSE;

/**
  This function figures out the size of the memory buckets based on the PEI
  memory bucket PCDs.  If the PCDs are not set by the platform then the memory
  buckets are disabled.
**/
VOID
EFIAPI
InitializeMemoryBucketSizes (
  VOID
  )
{
  UINT64  TotalBucketPages;

  TotalBucketPages = 0;

  mRuntimeMemoryStats[RuntimeCode].NumberOfPages = FixedPcdGet8 (PcdPeiMemoryBucketRuntimeCode);
  TotalBucketPages                    += mRuntimeMemoryStats[RuntimeCode].NumberOfPages;
  mRuntimeMemoryStats[RuntimeData].NumberOfPages = FixedPcdGet8 (PcdPeiMemoryBucketRuntimeData);
  TotalBucketPages                    += mRuntimeMemoryStats[RuntimeData].NumberOfPages;
  mRuntimeMemoryStats[ACPIReclaimMemory].NumberOfPages = FixedPcdGet8 (PcdPeiMemoryBucketAcpiReclaimMemory);
  TotalBucketPages                    += mRuntimeMemoryStats[ACPIReclaimMemory].NumberOfPages;
  mRuntimeMemoryStats[ACPIMemoryNVS].NumberOfPages = FixedPcdGet8 (PcdPeiMemoryBucketAcpiMemoryNvs);
  TotalBucketPages                    += mRuntimeMemoryStats[ACPIMemoryNVS].NumberOfPages;

  // Disable memory buckets if the PCDs are unaltered.
  if (TotalBucketPages == 0) {
    mMemoryBucketsDisabled = TRUE;
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

  @param[in] StartingAddress   The starting address of the memory buckets.
                               All of them are contiguous.
  @param[in] UsingPreMem       TRUE if we're allocating memory buckets in
                               PEI pre mem.
**/
VOID
EFIAPI
InitializeMemoryBuckets (
  EFI_PHYSICAL_ADDRESS  StartingAddress,
  BOOLEAN               UsingPreMem
  )
{
  UINTN   Index;
  UINTN   AdjustedIndex;
  UINT32  AddressAdjustment;

  InitializeMemoryBucketSizes ();
  AdjustedIndex        = 0;
  mPreMemPeiAllocation = UsingPreMem;

  for (Index = 0; Index < PEI_BUCKETS; Index++) {
    // Initialize memory locations for buckets
    AddressAdjustment = EFI_PAGE_SIZE *
                        ((UINT32)mRuntimeMemoryStats[Index].NumberOfPages * (UINT32)AdjustedIndex);
    mRuntimeMemoryStats[Index].BaseAddress = StartingAddress - AddressAdjustment;
    mCurrentBucketTops[Index]              = StartingAddress - AddressAdjustment;

    // Initialize Hob to be able to reference it later
    mRuntimeBucketHob.RuntimeBuckets[Index]     = mRuntimeMemoryStats[Index];
    mRuntimeBucketHob.CurrentTopInBucket[Index] = mCurrentBucketTops[Index];
    if (mRuntimeMemoryStats[Index].NumberOfPages != 0) {
      AdjustedIndex += 1;
    }
  }

  mRuntimeBucketHob.MemoryBucketsDisabled = mMemoryBucketsDisabled;
  mRuntimeBucketHob.PreMemAllocation      = mPreMemPeiAllocation;
}

/**
  This function updates the base memory addresses of each bucket if they
  were initially created in pre-mem PEI to their new post-mem locations.

  @param[in] OldMemoryTop   The top of memory for pre-mem PEI
  @param[in] NewMemoryTop   The top of memory for post-mem PEI
**/
VOID
EFIAPI
MigrateMemoryBuckets (
  EFI_PHYSICAL_ADDRESS  OldMemoryTop,
  EFI_PHYSICAL_ADDRESS  NewMemoryTop
  )
{
  UINTN   Index;
  UINT64  Offset;

  if (!mPreMemPeiAllocation) {
    return;
  }

  for (Index = 0; Index < PEI_BUCKETS; Index++) {
    if (mRuntimeMemoryStats[Index].NumberOfPages != 0) {
      Offset                                 = OldMemoryTop - mRuntimeMemoryStats[Index].BaseAddress;
      mRuntimeMemoryStats[Index].BaseAddress = NewMemoryTop - Offset;
    }
  }

  mPreMemPeiAllocation = FALSE;
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

  mCurrentBucketTops[Index] = NewTop;
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

  return mCurrentBucketTops[Index];
}

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
  EFI_MEMORY_TYPE  MemoryType
  )
{
  EFI_PHYSICAL_ADDRESS  ReturnValue;
  UINTN                 Index;

  Index = MemoryTypeToIndex (MemoryType);

  ReturnValue = (EFI_PHYSICAL_ADDRESS)(mRuntimeMemoryStats[Index].BaseAddress) - (EFI_PAGE_SIZE * mRuntimeMemoryStats[Index].NumberOfPages);
  return ReturnValue;
}

/**
  Function that returns the address associated with the end of the
  memory bucket structure.

  @retval    The memory address below the memory bucket structure.

**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetBottomOfBucketsAddress (
  VOID
  )
{
  UINTN  Index;

  for (Index = PEI_BUCKETS-1; Index >= 0; Index--) {
    if (mRuntimeMemoryStats[Index].NumberOfPages > 0) {
      return GetCurrentBucketBottom (mMemoryTypes[Index]);
    }
  }

  DEBUG ((DEBUG_ERROR, "[%a] - We should not get here.\n", __FUNCTION__));
  return 0;
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

  if (mRuntimeMemoryStats[Index].CurrentNumberOfPages + (UINT64)Pages > mRuntimeMemoryStats[Index].NumberOfPages) {
    DEBUG ((DEBUG_ERROR, "We have overflowed while allocating PEI pages of index: %d!\n", Index));
    ASSERT (FALSE);
    return;
  }

  mRuntimeMemoryStats[Index].CurrentNumberOfPages = (UINT64)mRuntimeMemoryStats[Index].CurrentNumberOfPages + (UINT64)Pages;

  mRuntimeBucketHob.RuntimeBuckets[Index]     = mRuntimeMemoryStats[Index];
  mRuntimeBucketHob.CurrentTopInBucket[Index] = mCurrentBucketTops[Index];
}

/**
  This function checks to see if the given address lies within the currently defined
  runtime memory bucket range.

  @param[in] Start   The start of the address we are trying to allocate memory in.

  @retval    TRUE    If the memory address is within the PEI memory bucket region.
  @retval    FALSE   If the memory address is not within the PEI memory bucket region.
**/
BOOLEAN
EFIAPI
CheckIfInRuntimeBoundary (
  EFI_PHYSICAL_ADDRESS  Start
  )
{
  // There is no boundary if the buckets are disabled
  if (mMemoryBucketsDisabled) {
    return FALSE;
  }

  if (mRuntimeMemInitialized &&
      ((Start >= GetBottomOfBucketsAddress ()) &&
       (Start <= mRuntimeMemoryStats[RuntimeCode].BaseAddress)))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Sets the mRuntimeMemInitialized variable to TRUE.
**/
VOID
EFIAPI
InitializeRuntimeMemoryBuckets (
  VOID
  )
{
  mRuntimeMemInitialized = TRUE;
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
  return mRuntimeMemInitialized;
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
IsRuntimeType (
  IN       EFI_MEMORY_TYPE  MemoryType
  )
{
  UINTN  Index;

  // If Runtime Buckets are disabled then deny memory bucket operations.
  if (mMemoryBucketsDisabled) {
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

  @retval    TRUE         PEI memory buckets are enabled
  @retval    FALSE        PEI memory buckets are disabled
**/
BOOLEAN
EFIAPI
AreMemoryBucketsEnabled (
  VOID
  )
{
  if (mMemoryBucketsDisabled) {
    return FALSE;
  }

  return TRUE;
}

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
  )
{
  BuildGuidDataHob (
    &gMemoryBucketInformationGuid,
    &mRuntimeBucketHob,
    (sizeof (PEI_MEMORY_BUCKET_INFORMATION))
    );
}

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
  )
{
  if (!IsRuntimeMemoryInitialized () && (MemBucketHob != NULL)) {
    SetMemoryBucketsFromHob (GET_GUID_HOB_DATA (MemBucketHob));
  }
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
  return mRuntimeBucketHob;
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
  for (Index = 0; Index < PEI_BUCKETS; Index++) {
    mRuntimeMemoryStats[Index] = TempStats->RuntimeBuckets[Index];
    mCurrentBucketTops[Index]  = TempStats->CurrentTopInBucket[Index];
  }

  mMemoryBucketsDisabled = TempStats->MemoryBucketsDisabled;
  mPreMemPeiAllocation   = TempStats->PreMemAllocation;

  mRuntimeMemInitialized = TRUE;
}
