/** @file -- MemoryAttributeProtocolFuncTestApp.h
TCBZ3519
Functionality to support MemoryAttributeProtocolFuncTestApp.c

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEM_ATTR_PROTOCOL_FUNC_TEST_APP_
#define _MEM_ATTR_PROTOCOL_FUNC_TEST_APP_

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MemoryAttribute.h>

#define PTE2MB                 0x200000
#define PTE1GB                 0x40000000
#define PTE512GB               0x8000000000

/**
  Get an unsplit page table entry and allocate entire region so the page
  doesn't need to be split on allocation

  @param[out]  Address  Address of allocated 2MB page region
**/
EFI_STATUS
EFIAPI
GetUnsplitPageTableEntry (
  OUT EFI_PHYSICAL_ADDRESS  *Address
  );

/**
  Check if the 2MB page entry correlating with the input address
  is set to no-execute

  @param[in]  Address  Address of the page table entry
**/
UINT64
EFIAPI
GetSpitPageTableEntryNoExecute (
  IN  PHYSICAL_ADDRESS  Address
  );

#endif // _MEM_ATTR_PROTOCOL_FUNC_TEST_APP_
