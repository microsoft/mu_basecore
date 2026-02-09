/** @file
  Public include file for PageTableLib library.

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PAGE_TABLE_LIB_H_
#define PAGE_TABLE_LIB_H_

typedef union {
  struct {
    UINT32    Present                  : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT32    ReadWrite                : 1;  // 0 = Read-Only, 1= Read/Write
    UINT32    UserSupervisor           : 1;  // 0 = Supervisor, 1=User
    UINT32    WriteThrough             : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT32    CacheDisabled            : 1;  // 0 = Cached, 1=Non-Cached
    UINT32    Accessed                 : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT32    Dirty                    : 1;  // 0 = Not dirty, 1 = Dirty (set by CPU)
    UINT32    Pat                      : 1;  // PAT
    UINT32    Global                   : 1;  // 0 = Not global, 1 = Global (if CR4.PGE = 1)
    UINT32    Reserved1                : 3;  // Ignored
    UINT32    PageTableBaseAddressLow  : 20; // Page Table Base Address Low

    UINT32    PageTableBaseAddressHigh : 20; // Page Table Base Address High
    UINT32    Reserved2                : 7;  // Ignored
    UINT32    ProtectionKey            : 4;  // Protection key
    UINT32    Nx                       : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} IA32_MAP_ATTRIBUTE;

#define IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK  0xFFFFFFFFFF000ull
#define IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS(pa)  ((pa)->Uint64 & IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK)
#define IA32_MAP_ATTRIBUTE_ATTRIBUTES(pa)               ((pa)->Uint64 & ~IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK)

//
// Below enum follows "4.1.1 Four Paging Modes" in Chapter 4 Paging of SDM Volume 3.
// Page1GB is only supported in 4-level and 5-level.
//
typedef enum {
  Paging32bit,

  //
  // High byte in paging mode indicates the max levels of the page table.
  // Low byte in paging mode indicates the max level that can be a leaf entry.
  //
  PagingPae4KB = 0x0301,
  PagingPae2MB = 0x0302,
  PagingPae    = 0x0302,

  Paging4Level4KB = 0x0401,
  Paging4Level2MB = 0x0402,
  Paging4Level    = 0x0402,
  Paging4Level1GB = 0x0403,

  Paging5Level4KB = 0x0501,
  Paging5Level2MB = 0x0502,
  Paging5Level    = 0x0502,
  Paging5Level1GB = 0x0503,

  PagingModeMax
} PAGING_MODE;

#define IA32_PE_BASE_ADDRESS_MASK_40  0xFFFFFFFFFF000ull
#define IA32_PE_BASE_ADDRESS_MASK_39  0xFFFFFFFFFE000ull

#define MAX_PAE_PDPTE_NUM  4

typedef enum {
  Pte   = 1,
  Pde   = 2,
  Pdpte = 3,
  Pml4  = 4,
  Pml5  = 5
} IA32_PAGE_LEVEL;

typedef struct {
  UINT32    Present        : 1;       // 0 = Not present in memory, 1 = Present in memory
  UINT32    ReadWrite      : 1;       // 0 = Read-Only, 1= Read/Write
  UINT32    UserSupervisor : 1;       // 0 = Supervisor, 1=User
  UINT32    Reserved0      : 29;
  UINT32    Reserved1      : 31;
  UINT32    Nx             : 1;        // No Execute bit
} IA32_PAGE_COMMON_ENTRY;

///
/// Format of a non-leaf entry that references a page table entry
///
typedef union {
  struct {
    UINT32    Present                  : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT32    ReadWrite                : 1;  // 0 = Read-Only, 1= Read/Write
    UINT32    UserSupervisor           : 1;  // 0 = Supervisor, 1=User
    UINT32    WriteThrough             : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT32    CacheDisabled            : 1;  // 0 = Cached, 1=Non-Cached
    UINT32    Accessed                 : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT32    Available0               : 1;  // Ignored
    UINT32    MustBeZero               : 1;  // Must Be Zero
    UINT32    Available2               : 4;  // Ignored
    UINT32    PageTableBaseAddressLow  : 20; // Page Table Base Address Low

    UINT32    PageTableBaseAddressHigh : 20; // Page Table Base Address High
    UINT32    Available3               : 11; // Ignored
    UINT32    Nx                       : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} IA32_PAGE_NON_LEAF_ENTRY;

#define IA32_PNLE_PAGE_TABLE_BASE_ADDRESS(pa)  ((pa)->Uint64 & IA32_PE_BASE_ADDRESS_MASK_40)

///
/// Format of a PML5 Entry (PML5E) that References a PML4 Table
///
typedef IA32_PAGE_NON_LEAF_ENTRY IA32_PML5E;

///
/// Format of a PML4 Entry (PML4E) that References a Page-Directory-Pointer Table
///
typedef IA32_PAGE_NON_LEAF_ENTRY IA32_PML4E;

///
/// Format of a Page-Directory-Pointer-Table Entry (PDPTE) that References a Page Directory
///
typedef IA32_PAGE_NON_LEAF_ENTRY IA32_PDPTE;

///
/// Format of a Page-Directory Entry that References a Page Table
///
typedef IA32_PAGE_NON_LEAF_ENTRY IA32_PDE;

///
/// Format of a leaf entry that Maps a 1-Gbyte or 2-MByte Page
///
typedef union {
  struct {
    UINT32    Present                  : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT32    ReadWrite                : 1;  // 0 = Read-Only, 1= Read/Write
    UINT32    UserSupervisor           : 1;  // 0 = Supervisor, 1=User
    UINT32    WriteThrough             : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT32    CacheDisabled            : 1;  // 0 = Cached, 1=Non-Cached
    UINT32    Accessed                 : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT32    Dirty                    : 1;  // 0 = Not dirty, 1 = Dirty (set by CPU)
    UINT32    MustBeOne                : 1;  // Page Size. Must Be One
    UINT32    Global                   : 1;  // 0 = Not global, 1 = Global (if CR4.PGE = 1)
    UINT32    Available1               : 3;  // Ignored
    UINT32    Pat                      : 1;  // PAT
    UINT32    PageTableBaseAddressLow  : 19; // Page Table Base Address Low

    UINT32    PageTableBaseAddressHigh : 20; // Page Table Base Address High
    UINT32    Available3               : 7;  // Ignored
    UINT32    ProtectionKey            : 4;  // Protection key
    UINT32    Nx                       : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE;
#define IA32_PLEB_PAGE_TABLE_BASE_ADDRESS(pa)  ((pa)->Uint64 & IA32_PE_BASE_ADDRESS_MASK_39)

///
/// Format of a Page-Directory Entry that Maps a 2-MByte Page
///
typedef IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE IA32_PDE_2M;

///
/// Format of a Page-Directory-Pointer-Table Entry (PDPTE) that Maps a 1-GByte Page
///
typedef IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE IA32_PDPTE_1G;

///
/// Format of a Page-Table Entry that Maps a 4-KByte Page
///
typedef union {
  struct {
    UINT32    Present                  : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT32    ReadWrite                : 1;  // 0 = Read-Only, 1= Read/Write
    UINT32    UserSupervisor           : 1;  // 0 = Supervisor, 1=User
    UINT32    WriteThrough             : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT32    CacheDisabled            : 1;  // 0 = Cached, 1=Non-Cached
    UINT32    Accessed                 : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT32    Dirty                    : 1;  // 0 = Not dirty, 1 = Dirty (set by CPU)
    UINT32    Pat                      : 1;  // PAT
    UINT32    Global                   : 1;  // 0 = Not global, 1 = Global (if CR4.PGE = 1)
    UINT32    Available1               : 3;  // Ignored
    UINT32    PageTableBaseAddressLow  : 20; // Page Table Base Address Low

    UINT32    PageTableBaseAddressHigh : 20; // Page Table Base Address High
    UINT32    Available3               : 7;  // Ignored
    UINT32    ProtectionKey            : 4;  // Protection key
    UINT32    Nx                       : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} IA32_PTE_4K;
#define IA32_PTE4K_PAGE_TABLE_BASE_ADDRESS(pa)  ((pa)->Uint64 & IA32_PE_BASE_ADDRESS_MASK_40)

///
/// Format of a Page-Directory-Pointer-Table Entry (PDPTE) that References a Page Directory (32bit PAE specific)
///
typedef union {
  struct {
    UINT32    Present                  : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT32    MustBeZero               : 2;  // Must Be Zero
    UINT32    WriteThrough             : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT32    CacheDisabled            : 1;  // 0 = Cached, 1=Non-Cached
    UINT32    MustBeZero2              : 4;  // Must Be Zero
    UINT32    Available                : 3;  // Ignored
    UINT32    PageTableBaseAddressLow  : 20; // Page Table Base Address Low

    UINT32    PageTableBaseAddressHigh : 20; // Page Table Base Address High
    UINT32    MustBeZero3              : 12; // Must Be Zero
  } Bits;
  UINT64    Uint64;
} IA32_PDPTE_PAE;

typedef union {
  IA32_PAGE_NON_LEAF_ENTRY             Pnle; // To access Pml5, Pml4, Pdpte and Pde.
  IA32_PML5E                           Pml5;
  IA32_PML4E                           Pml4;
  IA32_PDPTE                           Pdpte;
  IA32_PDE                             Pde;

  IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE    PleB; // to access Pdpte1G and Pde2M.
  IA32_PDPTE_1G                        Pdpte1G;
  IA32_PDE_2M                          Pde2M;

  IA32_PTE_4K                          Pte4K;

  IA32_PDPTE_PAE                       PdptePae;
  IA32_PAGE_COMMON_ENTRY               Pce; // To access all common bits in above entries.

  UINT64                               Uint64;
  UINTN                                Uintn;
} IA32_PAGING_ENTRY;

/**
  Create or update page table to map [LinearAddress, LinearAddress + Length) with specified attribute.

  @param[in, out] PageTable      The pointer to the page table to update, or pointer to NULL if a new page table is to be created.
                                 If not pointer to NULL, the value it points to won't be changed in this function.
  @param[in]      PagingMode     The paging mode.
  @param[in]      Buffer         The free buffer to be used for page table creation/updating.
  @param[in, out] BufferSize     The buffer size.
                                 On return, the remaining buffer size.
                                 The free buffer is used from the end so caller can supply the same Buffer pointer with an updated
                                 BufferSize in the second call to this API.
  @param[in]      LinearAddress  The start of the linear address range.
  @param[in]      Length         The length of the linear address range.
  @param[in]      Attribute      The attribute of the linear address range.
                                 All non-reserved fields in IA32_MAP_ATTRIBUTE are supported to set in the page table.
                                 Page table entries that map the linear address range are reset to 0 before set to the new attribute
                                 when a new physical base address is set.
  @param[in]      Mask           The mask used for attribute. The corresponding field in Attribute is ignored if that in Mask is 0.
  @param[out]     IsModified     TRUE means page table is modified by software or hardware. FALSE means page table is not modified by software.
                                 If the output IsModified is FALSE, there is possibility that the page table is changed by hardware. It is ok
                                 because page table can be changed by hardware anytime, and caller don't need to Flush TLB.

  @retval RETURN_UNSUPPORTED        PagingMode is not supported.
  @retval RETURN_INVALID_PARAMETER  PageTable, BufferSize, Attribute or Mask is NULL.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 1 but some other attributes are not provided.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  For present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  *BufferSize is not multiple of 4KB.
  @retval RETURN_BUFFER_TOO_SMALL   The buffer is too small for page table creation/updating.
                                    BufferSize is updated to indicate the expected buffer size.
                                    Caller may still get RETURN_BUFFER_TOO_SMALL with the new BufferSize.
  @retval RETURN_SUCCESS            PageTable is created/updated successfully or the input Length is 0.
**/
RETURN_STATUS
EFIAPI
PageTableMap (
  IN OUT UINTN               *PageTable  OPTIONAL,
  IN     PAGING_MODE         PagingMode,
  IN     VOID                *Buffer,
  IN OUT UINTN               *BufferSize,
  IN     UINT64              LinearAddress,
  IN     UINT64              Length,
  IN     IA32_MAP_ATTRIBUTE  *Attribute,
  IN     IA32_MAP_ATTRIBUTE  *Mask,
  OUT    BOOLEAN             *IsModified   OPTIONAL
  );

typedef struct {
  UINT64                LinearAddress;
  UINT64                Length;
  IA32_MAP_ATTRIBUTE    Attribute;
} IA32_MAP_ENTRY;

/**
  Parse page table.

  @param[in]      PageTable  Pointer to the page table.
  @param[in]      PagingMode The paging mode.
  @param[out]     Map        Return an array that describes multiple linear address ranges.
  @param[in, out] MapCount   On input, the maximum number of entries that Map can hold.
                             On output, the number of entries in Map.

  @retval RETURN_UNSUPPORTED       PageLevel is not 5 or 4.
  @retval RETURN_INVALID_PARAMETER MapCount is NULL.
  @retval RETURN_INVALID_PARAMETER *MapCount is not 0 but Map is NULL.
  @retval RETURN_BUFFER_TOO_SMALL  *MapCount is too small.
  @retval RETURN_SUCCESS           Page table is parsed successfully.
**/
RETURN_STATUS
EFIAPI
PageTableParse (
  IN     UINTN           PageTable,
  IN     PAGING_MODE     PagingMode,
  IN     IA32_MAP_ENTRY  *Map,
  IN OUT UINTN           *MapCount
  );

#endif
