/** @file PageSplitTest.c
TCBZ3519
Functionality to support MemoryAttributeProtocolFuncTestApp.c

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
***/

#include "MemoryAttributeProtocolFuncTestApp.h"

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
  )
{
  EFI_STATUS                      Status;
  EFI_PHYSICAL_ADDRESS            BaseAddress;
  UINTN                           Index2;
  UINTN                           Index3;
  UINTN                           Index4;
  PAGE_MAP_AND_DIRECTORY_POINTER  *Intermediate;
  PAGE_MAP_AND_DIRECTORY_POINTER  *L4PageTable;
  PAGE_TABLE_1G_ENTRY             *L3PageTable;
  PAGE_TABLE_ENTRY                *L2PageTable;

  L4PageTable = (PAGE_MAP_AND_DIRECTORY_POINTER *)(UINTN)(AsmReadCr3 ());

  for (Index4 = 0x0; Index4 < 0x200; Index4++) {
    if (!L4PageTable[Index4].Bits.Present) {
      continue;
    }

    L3PageTable = (PAGE_TABLE_1G_ENTRY *)(UINTN)(L4PageTable[Index4].Bits.PageTableBaseAddress << 12);

    for (Index3 = 0x0; Index3 < 0x200; Index3++ ) {
      if (!L3PageTable[Index3].Bits.Present) {
        continue;
      }

      //
      // MustBe1 indicates if the pointer is a directory pointer or a page table entry.
      //
      if (!(L3PageTable[Index3].Bits.MustBe1)) {
        //
        // We have to cast 1G and 2M directories to get all address bits.
        //
        Intermediate = (PAGE_MAP_AND_DIRECTORY_POINTER *)L3PageTable;
        L2PageTable  = (PAGE_TABLE_ENTRY *)(UINTN)(Intermediate[Index3].Bits.PageTableBaseAddress << 12);

        for (Index2 = 0x0; Index2 < 0x200; Index2++ ) {
          if (L2PageTable[Index2].Bits.MustBe1) {
            BaseAddress = (Index4 * PTE512GB) + (Index3 * PTE1GB) + (Index2 * PTE2MB);
            Status      = gBS->AllocatePages (AllocateAddress, EfiLoaderCode, EFI_SIZE_TO_PAGES (PTE2MB), &BaseAddress);
            if (!EFI_ERROR (Status)) {
              *Address = BaseAddress;
              return EFI_SUCCESS;
            }
          }
        }
      }
    }
  }

  return EFI_OUT_OF_RESOURCES;
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
  PAGE_MAP_AND_DIRECTORY_POINTER  *Intermediate;
  PAGE_MAP_AND_DIRECTORY_POINTER  *L4PageTable;
  PAGE_TABLE_1G_ENTRY             *L3PageTable;
  PAGE_TABLE_ENTRY                *L2PageTable;
  UINTN                           Index4;
  UINTN                           Index3;
  UINTN                           Index2;

  Index4 = ((UINTN)RShiftU64 (Address, 39)) & PAGING_PAE_INDEX_MASK;
  Index3 = ((UINTN)Address >> 30) & PAGING_PAE_INDEX_MASK;
  Index2 = ((UINTN)Address >> 21) & PAGING_PAE_INDEX_MASK;

  L4PageTable  = (PAGE_MAP_AND_DIRECTORY_POINTER *)(UINTN)(AsmReadCr3 ());
  L3PageTable  = (PAGE_TABLE_1G_ENTRY *)(UINTN)(L4PageTable[Index4].Bits.PageTableBaseAddress << 12);
  Intermediate = (PAGE_MAP_AND_DIRECTORY_POINTER *)L3PageTable;
  L2PageTable  = (PAGE_TABLE_ENTRY *)(UINTN)(Intermediate[Index3].Bits.PageTableBaseAddress << 12);

  return L2PageTable[Index2].Bits.Nx;
}
