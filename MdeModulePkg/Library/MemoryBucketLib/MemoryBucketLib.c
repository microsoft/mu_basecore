/**@file
Library that defines runtime memory buckets for use during PEI.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/MemoryBucketLib.h>
#include <Library/MemoryBucketHelperLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Core/Pei/PeiMain.h>

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
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;

  //
  // Search unused(freed) memory allocation HOB.
  //
  MemoryAllocationHob = NULL;
  Hob.Raw             = GetFirstHob (EFI_HOB_TYPE_UNUSED);
  while (Hob.Raw != NULL) {
    if (Hob.Header->HobLength == sizeof (EFI_HOB_MEMORY_ALLOCATION)) {
      MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_UNUSED, Hob.Raw);
  }

  if (MemoryAllocationHob != NULL) {
    //
    // Reuse the unused(freed) memory allocation HOB.
    //
    MemoryAllocationHob->Header.HobType = EFI_HOB_TYPE_MEMORY_ALLOCATION;
    ZeroMem (&(MemoryAllocationHob->AllocDescriptor.Name), sizeof (EFI_GUID));
    MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress = BaseAddress;
    MemoryAllocationHob->AllocDescriptor.MemoryLength      = Length;
    MemoryAllocationHob->AllocDescriptor.MemoryType        = MemoryType;
    //
    // Zero the reserved space to match HOB spec
    //
    ZeroMem (MemoryAllocationHob->AllocDescriptor.Reserved, sizeof (MemoryAllocationHob->AllocDescriptor.Reserved));
  } else {
    //
    // No unused(freed) memory allocation HOB found.
    // Build memory allocation HOB normally.
    //
    BuildMemoryAllocationHob (
      BaseAddress,
      Length,
      MemoryType
      );
  }
}

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
  )
{
  PEI_MEMORY_BUCKET_INFORMATION  RuntimeBucketHob;

  RuntimeBucketHob = GetRuntimeBucketHob ();
  BuildGuidDataHob (
    &gMemoryBucketInformationGuid,
    &RuntimeBucketHob,
    (sizeof (PEI_MEMORY_BUCKET_INFORMATION))
    );
}

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
  )
{
  // EFI_STATUS              Status;
  CONST EFI_PEI_SERVICES         **PeiServices;
  PEI_CORE_INSTANCE              *PrivateData;
  EFI_PEI_HOB_POINTERS           Hob;
  EFI_PHYSICAL_ADDRESS           FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS           FreeMemoryBottom;
  UINTN                          RemainingPages;
  UINTN                          Granularity;
  UINTN                          Padding;
  EFI_HOB_GUID_TYPE              *MemBucketHob;
  PEI_MEMORY_BUCKET_INFORMATION  RuntimeBucketHob;

  PeiServices = GetPeiServicesTablePointer ();

  Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  Hob.Raw     = PrivateData->HobList.Raw;

  if (Hob.Raw == NULL) {
    //
    // HOB is not initialized yet.
    //
    return EFI_NOT_AVAILABLE_YET;
  }

  if (RUNTIME_PAGE_ALLOCATION_GRANULARITY > DEFAULT_PAGE_ALLOCATION_GRANULARITY) {
    Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
  }

  MemBucketHob = GetFirstGuidHob (&gMemoryBucketInformationGuid);
  SyncMemoryBuckets ();

  // Check to see if Runtime memory has been initialized yet.
  if (IsRuntimeMemoryInitialized ()) {
    FreeMemoryTop    = GetCurrentBucketTop (MemoryType);
    FreeMemoryBottom = GetCurrentBucketEnd (MemoryType);
  } else if (!PrivateData->PeiMemoryInstalled && PrivateData->SwitchStackSignal) {
    //
    // When PeiInstallMemory is called but temporary memory has *not* been moved to permanent memory,
    // the AllocatePage will depend on the field of PEI_CORE_INSTANCE structure.
    //
    InitializeMemoryBuckets ((EFI_PHYSICAL_ADDRESS)(PrivateData->FreePhysicalMemoryTop));
    FreeMemoryTop    = GetCurrentBucketTop (MemoryType);
    FreeMemoryBottom = GetCurrentBucketEnd (MemoryType);
  } else {
    InitializeMemoryBuckets ((EFI_PHYSICAL_ADDRESS)(Hob.HandoffInformationTable->EfiFreeMemoryTop));
    FreeMemoryTop    = GetCurrentBucketTop (MemoryType);
    FreeMemoryBottom = GetCurrentBucketEnd (MemoryType);
  }

  //
  // Check to see if on correct boundary for the memory type.
  // If not aligned, make the allocation aligned.
  //
  Padding = (FreeMemoryTop) & (Granularity - 1);
  if ((UINTN)(FreeMemoryTop - FreeMemoryBottom) < Padding) {
    DEBUG ((DEBUG_ERROR, "AllocateRuntimePages failed: Out of space after padding.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  (FreeMemoryTop) -= Padding;
  if (Padding >= EFI_PAGE_SIZE) {
    //
    // Create a memory allocation HOB to cover
    // the pages that we will lose to rounding
    //
    InternalBuildRuntimeMemoryAllocationHob (
      (FreeMemoryTop),
      Padding & ~(UINTN)EFI_PAGE_MASK,
      EfiConventionalMemory
      );
  }

  //
  // Verify that there is sufficient memory to satisfy the allocation.
  //
  RemainingPages = (UINTN)(FreeMemoryTop - FreeMemoryBottom) >> EFI_PAGE_SHIFT;
  //
  // The number of remaining pages needs to be greater than or equal to that of the request pages.
  //
  Pages = ALIGN_VALUE (Pages, EFI_SIZE_TO_PAGES (Granularity));
  if (RemainingPages < Pages) {
    //
    // Try to find free memory by searching memory allocation HOBs.
    //
    // Status = FindFreeMemoryFromMemoryAllocationHob (MemoryType, Pages, Granularity, Memory);
    // if (!EFI_ERROR (Status)) {
    // return Status;
    // }

    DEBUG ((DEBUG_ERROR, "AllocateRuntimePages failed: No 0x%lx Pages is available.\n", (UINT64)Pages));
    DEBUG ((DEBUG_ERROR, "There is only left 0x%lx pages memory resource to be allocated.\n", (UINT64)RemainingPages));
    return EFI_OUT_OF_RESOURCES;
  } else {
    //
    // Update the PHIT to reflect the memory usage
    //
    (FreeMemoryTop) -= Pages * EFI_PAGE_SIZE;

    //
    // Update the value for the caller
    //
    *Memory = FreeMemoryTop;
    UpdateCurrentBucketTop (FreeMemoryTop, MemoryType);

    //
    // Create a memory allocation HOB.
    //
    InternalBuildRuntimeMemoryAllocationHob (
      FreeMemoryTop,
      Pages * EFI_PAGE_SIZE,
      MemoryType
      );

    UpdateRuntimeMemoryStats (Pages, MemoryType);

    if (MemBucketHob == NULL) {
      InternalBuildRuntimeMemoryAllocationInfoHob ();
    } else {
      RuntimeBucketHob = GetRuntimeBucketHob ();
      CopyMem (
        GET_GUID_HOB_DATA (MemBucketHob),
        &RuntimeBucketHob,
        (sizeof (PEI_MEMORY_BUCKET_INFORMATION))
        );
    }

    InitializeRuntimeMemoryBuckets ();

    return EFI_SUCCESS;
  }
}

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
  )
{
  return IsRuntimeTypeInternal (MemoryType);
}

/**
  Function that makes pulls the memory bucket hob information locally
  if necessary.  This is so it can be more easily referenced.

**/
VOID
EFIAPI
SyncMemoryBuckets (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *MemBucketHob;

  MemBucketHob = GetFirstGuidHob (&gMemoryBucketInformationGuid);
  if (!IsRuntimeMemoryInitialized () && (MemBucketHob != NULL)) {
    SetMemoryBucketsFromHob (GET_GUID_HOB_DATA (MemBucketHob));
  }
}

/**
  Function to check if a memory region is within the memory bucket structure.

  @param[in] Start              The start of the memory region we are checking

  @retval    TRUE               The region is in the memory bucket structure.
  @retval    FALSE              The region is not in the memory bucket structure.

**/
BOOLEAN
EFIAPI
CheckIfInRuntimeBoundary (
  EFI_PHYSICAL_ADDRESS  Start
  )
{
  return CheckIfInRuntimeBoundaryInternal (Start);
}

/**
  Function that returns the address associated with the end of the
  memory bucket structure.

  @retval    The memory address below the memory bucket structure.

**/
EFI_PHYSICAL_ADDRESS
EFIAPI
GetEndOfBucketsAddress (
  VOID
  )
{
  return GetCurrentBucketEnd (EfiACPIMemoryNVS);
}
