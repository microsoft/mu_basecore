/** @file MemoryAttributeProtocolFuncTestAppxAarch64.c
TCBZ3519
Functionality to support MemoryAttributeProtocolFuncTestApp.c

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
***/

#include "MemoryAttributeProtocolFuncTestApp.h"

/**
  Get an unsplit page table entry and allocate entire region so the page
  doesn't need to be split on allocation

  @param[out]  Address  Address of allocated 2MB page region
**/
EFI_STATUS
EFIAPI
GetUnsplitPageTableEntry (
  OUT EFI_PHYSICAL_ADDRESS  *Address
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Check if the 2MB page entry correlating with the input address
  is set to no-execute

  @param[in]  Address  Address of the page table entry
**/
UINT64
EFIAPI
GetSpitPageTableEntryNoExecute (
  IN  PHYSICAL_ADDRESS  Address
  )
{
  return 0;
}
