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
#define PAGING_PAE_INDEX_MASK  0x1FF

#pragma pack(1)

//
// Page-Map Level-4 Offset (L4PageTable) and
// Page-Directory-Pointer Offset (PDPE) entries 4K & 2MB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Reserved             : 1;  // Reserved
    UINT64    MustBeZero           : 2;  // Must Be Zero
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PageTableBaseAddress : 40; // Page Table Base Address
    UINT64    AvailableHigh        : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} PAGE_MAP_AND_DIRECTORY_POINTER;

//
// Page Table Entry 1GB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    MustBe1              : 1;  // Must be 1
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PAT                  : 1;  //
    UINT64    MustBeZero           : 17; // Must be zero;
    UINT64    PageTableBaseAddress : 22; // Page Table Base Address
    UINT64    AvailableHigh        : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_1G_ENTRY;

//
// Page Table Entry 2MB
//
typedef union {
  struct {
    UINT64    Present              : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;  // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;  // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;  // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    MustBe1              : 1;  // Must be 1
    UINT64    Global               : 1;  // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;  // Available for use by system software
    UINT64    PAT                  : 1;  //
    UINT64    MustBeZero           : 8;  // Must be zero;
    UINT64    PageTableBaseAddress : 31; // Page Table Base Address
    UINT64    AvailableHigh        : 11; // Available for use by system software
    UINT64    Nx                   : 1;  // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_ENTRY;

#pragma pack()

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
