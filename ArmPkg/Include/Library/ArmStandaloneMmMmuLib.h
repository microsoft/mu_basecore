/** @file

  Copyright (c) 2015 - 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_STANDALONE_MM_MMU_LIB_H_
#define ARM_STANDALONE_MM_MMU_LIB_H_
#include <Uefi/UefiBaseType.h>

#include <Library/ArmLib.h>

/**
  Convert a region of memory to read-protected, by clearing the access flag.
  @param  BaseAddress           The start of the region.
  @param  Length                The size of the region.
  @retval EFI_SUCCESS           The attributes were set successfully.
  @retval EFI_OUT_OF_RESOURCES  The operation failed due to insufficient memory.
**/
EFI_STATUS
EFIAPI
ArmSetMemoryRegionNoAccess (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Convert a region of memory to read-enabled, by setting the access flag.
  @param  BaseAddress           The start of the region.
  @param  Length                The size of the region.
  @retval EFI_SUCCESS           The attributes were set successfully.
  @retval EFI_OUT_OF_RESOURCES  The operation failed due to insufficient memory.
**/
EFI_STATUS
EFIAPI
ArmClearMemoryRegionNoAccess (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
EFIAPI
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
EFIAPI
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
EFIAPI
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
EFIAPI
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

#endif // ARM_STANDALONE_MM_MMU_LIB_H_
