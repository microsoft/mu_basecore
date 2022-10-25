/** @file

Library for defining memory buckets.  For internal use only.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEMORY_BUCKET_HELPER_LIB_H__
#define __MEMORY_BUCKET_HELPER_LIB_H__

#include <Uefi.h>
#include <Base.h>
#include <Guid/MemoryBucketInformation.h>

VOID
EFIAPI
UpdateRuntimeMemoryStats (
  UINTN Pages,
  EFI_MEMORY_TYPE MemoryType
  );

VOID
EFIAPI
InitializeMemoryBuckets (
  EFI_PHYSICAL_ADDRESS StartingAddress
  );

VOID
EFIAPI
UpdateCurrentBucketTop (
  EFI_PHYSICAL_ADDRESS NewTop,
  EFI_MEMORY_TYPE      MemoryType
  );

VOID
EFIAPI
InitializeRuntimeMemoryBuckets (
  VOID
  );

EFI_PHYSICAL_ADDRESS
EFIAPI
GetCurrentBucketTop (
  EFI_MEMORY_TYPE MemoryType
  );

EFI_PHYSICAL_ADDRESS
EFIAPI
GetCurrentBucketEnd (
  EFI_MEMORY_TYPE MemoryType
  );

VOID
MemoryBucketLibInitialize (
  VOID
  );

BOOLEAN
EFIAPI
CheckIfInRuntimeBoundaryInternal (
  EFI_PHYSICAL_ADDRESS Start
  );

BOOLEAN
EFIAPI
IsRuntimeMemoryInitialized (
  VOID
  );

UINTN
EFIAPI
GetBucketLength (
  VOID
  );

BOOLEAN
EFIAPI
IsRuntimeTypeInternal (
  IN       EFI_MEMORY_TYPE       MemoryType
  );

PEI_MEMORY_BUCKET_INFORMATION
EFIAPI
GetRuntimeBucketHob (
  VOID
);


VOID
EFIAPI
SetMemoryBucketsFromHob (
 VOID* MemoryBuckets
);

#endif
