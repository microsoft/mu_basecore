/**@file
  This keeps track of the different buckets and the data for the associated HOB.

  This library should not be called by anything outside of the PEI CORE.

  // MU_CHANGE - WHOLE_FILE - Save memory allocations for the PEI memory buckets

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"
#include "MemoryBuckets.h"

// Memory types being kept in buckets in PEI.  Currently only runtime types.
EFI_MEMORY_TYPE  mMemoryTypes[] = {
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS
};

/**
  This function figures out the size of the memory buckets based on the PEI
  memory bucket PCDs.  If the PCDs are not set by the platform then the memory
  buckets are disabled.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
**/
UINTN
EFIAPI
InitializeMemoryBucketSizes (
  IN PEI_CORE_INSTANCE  *PrivateData
  )
{
  UINT64                       TotalBucketPages;
  UINT64                       Index;
  EFI_HOB_GUID_TYPE            *GuidHob;
  EFI_MEMORY_TYPE_INFORMATION  *MemInfo;

  TotalBucketPages = 0;
  GuidHob          = GetFirstGuidHob (&gEfiMemoryTypeInformationGuid);

  if (GuidHob != NULL) {
    DEBUG ((DEBUG_INFO, "[%a] - The Memory Type Information exists!\n", __FUNCTION__));

    MemInfo = GET_GUID_HOB_DATA (GuidHob);

    for (Index = 0; Index < ARRAY_SIZE (mMemoryTypes); Index++) {
      PrivateData->PeiMemoryBuckets.RuntimeBuckets[mMemoryTypes[Index]].NumberOfPages = \
        (UINT64)GetBucketSizeFromMemoryInfoHob (PrivateData, MemInfo, mMemoryTypes[Index]);
      TotalBucketPages += PrivateData->PeiMemoryBuckets.RuntimeBuckets[mMemoryTypes[Index]].NumberOfPages;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "[%a] - The Memory Type Information doesn't exist!\n", __FUNCTION__));
  }

  return (UINTN)TotalBucketPages;
}

/**
  This function gets the number of pages associated with the memory type
  that is saved within the MEMORY_TYPE_INFORMATION Hob.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] MemInfo       Pointer the the MEMORY_TYPE_INFORMATION object
                           with the relevant memory bucket sizes.
  @param[in] MemoryType    The type of memory we are interested in.

  @retval    Returns the number of pages to use for the bucket in the memory
             memory type inputted.
**/
UINT32
EFIAPI
GetBucketSizeFromMemoryInfoHob (
  IN PEI_CORE_INSTANCE            *PrivateData,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemInfo,
  IN EFI_MEMORY_TYPE              MemoryType
  )
{
  UINTN  Index;

  for (Index = 0; MemInfo[Index].Type != EfiMaxMemoryType; Index++) {
    if (MemInfo[Index].Type == (UINT32)MemoryType) {
      DEBUG ((DEBUG_INFO, "[%a] - Index = %d\n", __FUNCTION__, Index));
      return MemInfo[Index].NumberOfPages;
    }
  }

  DEBUG ((DEBUG_ERROR, "[%a] - The memory type wasn't found in the MEMORY_TYPE_INFORMATION hob!\n", __FUNCTION__));

  return 0;
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

  @param[in] PrivateData          Pointer to PeiCore's private data structure.
  @param[in] StartingAddress      The starting address of the memory buckets.
                                  All of them are contiguous.
  @param[in] LengthAvailable      Number of bytes available for memory buckets from StartingAddress.

**/
UINTN
EFIAPI
InitializeMemoryBuckets (
  IN PEI_CORE_INSTANCE     *PrivateData,
  IN EFI_PHYSICAL_ADDRESS  StartingAddress,
  IN UINTN                 LengthAvailable
  )
{
  UINTN   Index;
  UINTN   TotalBucketPages;
  UINT64  AddressAdjustment;

  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].BaseAddress          = 0;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].MaximumAddress       = (EFI_PHYSICAL_ADDRESS)-1;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].CurrentNumberOfPages = 0;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].NumberOfPages        = 0;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].InformationIndex     = (UINT32)Index;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].Special              = TRUE;
    PrivateData->PeiMemoryBuckets.RuntimeBuckets[Index].Runtime              = TRUE;
    PrivateData->PeiMemoryBuckets.CurrentTopInBucket[Index]                  = (EFI_PHYSICAL_ADDRESS)-1;
  }

  TotalBucketPages = InitializeMemoryBucketSizes (PrivateData);

  if (TotalBucketPages > 0) {
    for (Index = ARRAY_SIZE (mMemoryTypes), AddressAdjustment = 0; Index > 0; Index--) {
      // Initialize memory locations for buckets
      PrivateData->PeiMemoryBuckets.RuntimeBuckets[mMemoryTypes[Index-1]].MaximumAddress = \
        StartingAddress - AddressAdjustment;
      PrivateData->PeiMemoryBuckets.CurrentTopInBucket[mMemoryTypes[Index-1]] = \
        StartingAddress - AddressAdjustment;
      AddressAdjustment += \
        (PrivateData->PeiMemoryBuckets.RuntimeBuckets[mMemoryTypes[Index-1]].NumberOfPages * EFI_PAGE_SIZE);
      PrivateData->PeiMemoryBuckets.RuntimeBuckets[mMemoryTypes[Index-1]].BaseAddress = \
        StartingAddress - AddressAdjustment;
    }

    PrivateData->PeiMemoryBuckets.RuntimeMemInitialized = TRUE;
    PrivateData->PeiMemoryBuckets.MemoryBucketsDisabled = FALSE;
  } else {
    PrivateData->PeiMemoryBuckets.MemoryBucketsDisabled = TRUE;
  }

  return TotalBucketPages;
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
  PrivateData->PeiMemoryBuckets.CurrentTopInBucket[MemoryType] = NewTop;
}

/**
  This function gets the address of the startfree memory in the bucket
  specified by MemoryType.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] MemoryType   The type of memory we are interested in.

  @retval    Returns an address that points to the start of free memory in a
             memory bucket that is specified by MemoryType
**/
EFI_PHYSICAL_ADDRESS *
EFIAPI
GetCurrentBucketTop (
  IN PEI_CORE_INSTANCE  *PrivateData,
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  return &PrivateData->PeiMemoryBuckets.CurrentTopInBucket[MemoryType];
}

/**
  This function gets the address of the bottom of the bucket specified by
  MemoryType.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
  @param[in] MemoryType   The type of memory we are interested in.

  @retval    Returns an address that points to the bottom of memory in a
             memory bucket that is specified by MemoryType
**/
EFI_PHYSICAL_ADDRESS *
EFIAPI
GetCurrentBucketBottom (
  IN PEI_CORE_INSTANCE  *PrivateData,
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  return &PrivateData->PeiMemoryBuckets.RuntimeBuckets[MemoryType].BaseAddress;
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
  IN PEI_CORE_INSTANCE  *PrivateData
  )
{
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  *Address;

  for (Index = 0; Index < ARRAY_SIZE (mMemoryTypes); Index++) {
    if (PrivateData->PeiMemoryBuckets.RuntimeBuckets[mMemoryTypes[Index]].NumberOfPages > 0) {
      Address = GetCurrentBucketBottom (PrivateData, mMemoryTypes[Index]);
      if (Address != NULL) {
        return *Address;
      }

      ASSERT (Address != NULL);
    }
  }

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
  IN PEI_CORE_INSTANCE  *PrivateData,
  IN UINTN              Pages,
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  if (PrivateData->PeiMemoryBuckets.RuntimeBuckets[MemoryType].CurrentNumberOfPages + (UINT64)Pages
      > PrivateData->PeiMemoryBuckets.RuntimeBuckets[MemoryType].NumberOfPages)
  {
    DEBUG ((DEBUG_ERROR, "We have overflowed while allocating PEI pages of index: %d!\n", MemoryType));
    ASSERT (FALSE);
    return;
  }

  PrivateData->PeiMemoryBuckets.RuntimeBuckets[MemoryType].CurrentNumberOfPages = (UINT64)PrivateData->PeiMemoryBuckets.RuntimeBuckets[MemoryType].CurrentNumberOfPages + (UINT64)Pages;
  UpdateMemoryBucketHob (PrivateData);
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
  if (!AreMemoryBucketsEnabled (PrivateData)) {
    return FALSE;
  }

  if (PrivateData->PeiMemoryBuckets.RuntimeMemInitialized &&
      ((Start >= GetBottomOfBucketsAddress (PrivateData)) &&
       (Start <= *GetCurrentBucketTop (PrivateData, EfiACPIMemoryNVS))))
  {
    return TRUE;
  }

  return FALSE;
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
  IN PEI_CORE_INSTANCE  *PrivateData
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
  IN PEI_CORE_INSTANCE      *PrivateData,
  IN       EFI_MEMORY_TYPE  MemoryType
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mMemoryTypes); Index++) {
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
  IN PEI_CORE_INSTANCE  *PrivateData
  )
{
  return !PrivateData->PeiMemoryBuckets.MemoryBucketsDisabled;
}

/**
  Function that builds and updates the memory bucket hob that will be
  consumed in DXE.

  @param[in] PrivateData   Pointer to PeiCore's private data structure.
**/
VOID
EFIAPI
UpdateMemoryBucketHob (
  IN PEI_CORE_INSTANCE  *PrivateData
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&gMemoryTypeStatisticsGuid);
  if (GuidHob != NULL) {
    CopyMem (
      GET_GUID_HOB_DATA (GuidHob),
      PrivateData->PeiMemoryBuckets.RuntimeBuckets,
      (sizeof (PrivateData->PeiMemoryBuckets.RuntimeBuckets))
      );
  } else {
    BuildGuidDataHob (
      &gMemoryTypeStatisticsGuid,
      PrivateData->PeiMemoryBuckets.RuntimeBuckets,
      sizeof (PrivateData->PeiMemoryBuckets.RuntimeBuckets)
      );
  }
}
