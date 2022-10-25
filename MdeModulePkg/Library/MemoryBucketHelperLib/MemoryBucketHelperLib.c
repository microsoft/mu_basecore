/**@file
Library the defines Runtime memory buckets for use during PEI

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryBucketHelperLib.h>
#include <Library/BaseMemoryLib.h>

// Memory types being kept in buckets in PEI
EFI_MEMORY_TYPE MemoryTypes[4] = {
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS
};

// PEI memory bucket statistics.  Can be extended if necessary
EFI_MEMORY_TYPE_STATISTICS RuntimeMemoryStats[4] = {
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiRuntimeServicesCode, TRUE,  TRUE  },  // EfiRuntimeServicesCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiRuntimeServicesData, TRUE,  TRUE  },  // EfiRuntimeServicesData
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiACPIReclaimMemory,   TRUE,  FALSE },  // EfiACPIReclaimMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiACPIMemoryNVS,       TRUE,  FALSE }   // EfiACPIMemoryNVS
};

EFI_PHYSICAL_ADDRESS CurrentBucketTops[4] = {
  MAX_ALLOC_ADDRESS, // EfiRuntimeServicesCode
  MAX_ALLOC_ADDRESS, // EfiRuntimeServicesData
  MAX_ALLOC_ADDRESS, // EfiACPIReclaimMemory
  MAX_ALLOC_ADDRESS  // EfiACPIMemoryNVS
};

// Information storage for Hob
PEI_MEMORY_BUCKET_INFORMATION RuntimeBucketHob;

UINT32                RuntimeMemLength = 50;
BOOLEAN               RuntimeMemInitialized = FALSE;

VOID
EFIAPI
InitializeMemoryBuckets (
  EFI_PHYSICAL_ADDRESS StartingAddress
  )
{
  UINTN  Index;
  UINT32 AddressAdjustment;

  for (Index = 0; Index < NumberOfBuckets; Index++) {
    // Initialize memory locations for buckets
    AddressAdjustment = EFI_PAGE_SIZE * RuntimeMemLength * (UINT32)Index;
    RuntimeMemoryStats[Index].BaseAddress = StartingAddress - AddressAdjustment;
    CurrentBucketTops[Index] = StartingAddress - AddressAdjustment;

    // Initialize Hob to be able to reference it later
    RuntimeBucketHob.RuntimeBuckets[Index] = RuntimeMemoryStats[Index];
    RuntimeBucketHob.CurrentTopInBucket[Index] = CurrentBucketTops[Index];
  }

}

VOID
EFIAPI
UpdateCurrentBucketTop (
  EFI_PHYSICAL_ADDRESS NewTop,
  EFI_MEMORY_TYPE      MemoryType
  )
{
  UINTN Index;
  switch (MemoryType) {
    case EfiRuntimeServicesCode:
      Index = 0;
    case EfiRuntimeServicesData:
      Index = 1;
    case EfiACPIReclaimMemory:
      Index = 2;
    case EfiACPIMemoryNVS:
      Index = 3;
  }

  CurrentBucketTops[Index] = NewTop;
}

VOID
EFIAPI
InitializeRuntimeMemoryBuckets (
  VOID
  )
{
  RuntimeMemInitialized = TRUE;
}

EFI_PHYSICAL_ADDRESS
EFIAPI
GetCurrentBucketTop (
  EFI_MEMORY_TYPE MemoryType
  )
{
  UINTN Index;
  switch (MemoryType) {
    case EfiRuntimeServicesCode:
      Index = 0;
    case EfiRuntimeServicesData:
      Index = 1;
    case EfiACPIReclaimMemory:
      Index = 2;
    case EfiACPIMemoryNVS:
      Index = 3;
  }

  return CurrentBucketTops[Index];
}

EFI_PHYSICAL_ADDRESS
EFIAPI
GetCurrentBucketEnd (
  EFI_MEMORY_TYPE MemoryType
  )
{
  EFI_PHYSICAL_ADDRESS ReturnValue;
  UINTN Index;
  switch (MemoryType) {
    case EfiRuntimeServicesCode:
      Index = 0;
    case EfiRuntimeServicesData:
      Index = 1;
    case EfiACPIReclaimMemory:
      Index = 2;
    case EfiACPIMemoryNVS:
      Index = 3;
  }

  ReturnValue = (EFI_PHYSICAL_ADDRESS) (CurrentBucketTops[Index]) - (EFI_PAGE_SIZE * RuntimeMemLength);
  return ReturnValue;
}

VOID
EFIAPI
UpdateRuntimeMemoryStats (
  UINTN Pages,
  EFI_MEMORY_TYPE MemoryType
  )
{
  UINTN Index;
  switch (MemoryType) {
    case EfiRuntimeServicesCode:
      Index = 0;
    case EfiRuntimeServicesData:
      Index = 1;
    case EfiACPIReclaimMemory:
      Index = 2;
    case EfiACPIMemoryNVS:
      Index = 3;
  }
  if (RuntimeMemoryStats[Index].NumberOfPages + (UINT64)Pages > RuntimeMemLength) {
    DEBUG ((DEBUG_ERROR, "We have overflowed while allocating PEI pages of index: %d!\n", Index));
    ASSERT (FALSE);
    return;
  }
  RuntimeMemoryStats[Index].NumberOfPages = (UINT64)RuntimeMemoryStats[Index].NumberOfPages + (UINT64)Pages;
  RuntimeMemoryStats[Index].CurrentNumberOfPages = RuntimeMemoryStats[Index].NumberOfPages;

  RuntimeBucketHob.RuntimeBuckets[Index] = RuntimeMemoryStats[Index];
  RuntimeBucketHob.CurrentTopInBucket[Index] = CurrentBucketTops[Index];
}

BOOLEAN
EFIAPI
CheckIfInRuntimeBoundaryInternal (
  EFI_PHYSICAL_ADDRESS Start
  )
{
  if (RuntimeMemInitialized && 
    ((Start >= (RuntimeMemoryStats[0].BaseAddress - (NumberOfBuckets * EFI_PAGE_SIZE * RuntimeMemLength))) && 
    (Start <= RuntimeMemoryStats[0].BaseAddress))) {
    return TRUE;
  }
  return FALSE;
}

BOOLEAN
EFIAPI
IsRuntimeMemoryInitialized (
  VOID
  )
{
  return RuntimeMemInitialized;
}

UINTN
EFIAPI
GetBucketLength (
  VOID
  )
{
  return RuntimeMemLength;
}

PEI_MEMORY_BUCKET_INFORMATION
EFIAPI
GetRuntimeBucketHob (
  VOID
  )
{
  return RuntimeBucketHob;
}

BOOLEAN
EFIAPI
IsRuntimeTypeInternal (
  IN       EFI_MEMORY_TYPE       MemoryType
  )
{
  UINTN Index;

  for (Index = 0; Index < NumberOfBuckets; Index++) {
    if (MemoryType == MemoryTypes[Index]) {
        return TRUE;
    }
  }
  return FALSE;
}

VOID
EFIAPI
SetMemoryBucketsFromHob (
  VOID* MemoryBuckets
  )
{
  PEI_MEMORY_BUCKET_INFORMATION *TempStats;
  UINTN                         Index;

  if (MemoryBuckets == NULL) {
    DEBUG((DEBUG_ERROR, "NO PEI RUNTIME MEMORY BUCKETS YET\n"));
    return;
  }
  TempStats = (PEI_MEMORY_BUCKET_INFORMATION *) MemoryBuckets;
  for (Index = 0; Index < NumberOfBuckets; Index++) {
    RuntimeMemoryStats[Index] = TempStats->RuntimeBuckets[Index];
    CurrentBucketTops[Index] = TempStats->CurrentTopInBucket[Index];
  }
  RuntimeMemInitialized = TRUE;
}
