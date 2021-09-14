/** @file
This library instance implements a very limited MMU Lib instance
for the ARM/AARCH64 architectures.  This library shims a common library
interface to the ArmPkg defined ArmMmuLib.ib.

MU_CHANGE -
If this architectural neutral abstraction is accepted upstream then
a full implementation can be done to support more general purpose usage
and/or it could be combined with the ArmMmuLib

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/ArmMmuLib.h>
#include <Library/DebugLib.h>

/**
  Bitwise sets the memory attributes on a range of memory based on an attributes mask.

  @param  BaseAddress           The start of the range for which to set attributes.
  @param  Length                The length of the range.
  @param  Attributes            A bitmask of the attributes to set. See "Physical memory
                                protection attributes" in UefiSpec.h

  @return EFI_SUCCESS
  @return Others

**/
EFI_STATUS
EFIAPI
MmuSetAttributes (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes
  )
{
  EFI_STATUS  Status;

  Status = EFI_UNSUPPORTED;

  // MU_CHANGE - START
  // Ensure that the attributes are valid
  ASSERT ((Attributes & ~(EFI_MEMORY_XP | EFI_MEMORY_RO)) == 0);

  // Use ArmSetMemoryAttributes because the individually called attribute updates have been removed
  Status = ArmSetMemoryAttributes (
             BaseAddress,
             Length,
             Attributes,
             (EFI_MEMORY_XP | EFI_MEMORY_RO)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to clear attributes - 0x%llx.  Status = %r\n", __func__, Attributes, Status));
  }

  // MU_CHANGE - END
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Bitwise clears the memory attributes on a range of memory based on an attributes mask.

  @param  BaseAddress           The start of the range for which to clear attributes.
  @param  Length                The length of the range.
  @param  Attributes            A bitmask of the attributes to clear. See "Physical memory
                                protection attributes" in UefiSpec.h

  @return EFI_SUCCESS
  @return Others

**/
EFI_STATUS
EFIAPI
MmuClearAttributes (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes
  )
{
  EFI_STATUS  Status;

  Status = EFI_UNSUPPORTED;

  // MU_CHANGE - START
  // Ensure that the attributes are valid
  ASSERT ((Attributes & ~(EFI_MEMORY_XP | EFI_MEMORY_RO)) == 0);

  // As we clear the attributes, we need to "set" the inverse of the attributes
  Attributes = (~Attributes) & (EFI_MEMORY_XP | EFI_MEMORY_RO);

  // Use ArmSetMemoryAttributes because the individually called attribute updates have been removed
  Status = ArmSetMemoryAttributes (
             BaseAddress,
             Length,
             Attributes,
             (EFI_MEMORY_XP | EFI_MEMORY_RO)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to clear attributes - 0x%llx.  Status = %r\n", __func__, Attributes, Status));
  }

  // MU_CHANGE - END
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Returns the memory attributes on a range of memory.

  @param  BaseAddress           The start of the range for which to set attributes.
  @param  Attributes            A return pointer for the attributes.

  @return EFI_SUCCESS
  @return EFI_INVALID_PARAMETER   A return pointer is NULL.
  @return Others

**/
EFI_STATUS
EFIAPI
MmuGetAttributes (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  OUT UINT64                *Attributes
  )
{
  EFI_STATUS  Status;

  Status = EFI_UNSUPPORTED;

  DEBUG ((DEBUG_ERROR, "%a() API not implemented\n", __func__));

  ASSERT_EFI_ERROR (Status);
  return Status;
}
