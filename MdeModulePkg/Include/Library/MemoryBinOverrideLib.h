/** @file
  Header definitions for the memory bin override library. This library allows
  a platform to override the location or size of the memory type bins.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEMORY_BIN_OVERRIDE_LIB_H__
#define MEMORY_BIN_OVERRIDE_LIB_H__

/**
  Reports a runtime memory bin location to the override library.

  @param[in]  Type            The memory type for the reported bin.
  @param[in]  BaseAddress     The base physical address of the reported bin.
  @param[in]  NumberOfPages   The number of pages in the reported bin.
**/
VOID
EFIAPI
ReportMemoryBinLocation (
  IN EFI_MEMORY_TYPE       Type,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                NumberOfPages
  );

/**
  Checks if the library needs to override the given memory bin allocation type,
  location, and size. If this function encounters internal errors, the
  parameters should remain unchanged.

  @param[in]    Type            The memory type of the bin.
  @param[out]   BaseAddress     The base address of the bin override on return.
  @param[out]   NumberOfPages   The number of pages of the bin override on return.
  @param[out]   AllocationType  The allocation type for the bin, AllocateAddress
                                if an override was provided.
**/
VOID
EFIAPI
GetMemoryBinOverride (
  IN EFI_MEMORY_TYPE        Type,
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT32                *NumberOfPages,
  OUT EFI_ALLOCATE_TYPE     *AllocationType
  );

#endif
