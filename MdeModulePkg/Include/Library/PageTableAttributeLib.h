/** @file
  Library to parse page/translation table entries.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PAGE_TABLE_ATTRIBUTE_LIB_H_
#define PAGE_TABLE_ATTRIBUTE_LIB_H_

typedef struct {
  UINT64    LinearAddress;
  UINT64    Length;
  UINT64    PageEntry;
} PAGE_MAP_ENTRY;

typedef struct {
  UINT32            ArchSignature;
  PAGE_MAP_ENTRY    *Entries;
  UINTN             EntryCount;
  UINTN             EntryPagesAllocated;
} PAGE_MAP;

// The signature of the PAGE_MAP struct is used to determine the architecture of the page/translation table
// entries.
#define AARCH64_PAGE_MAP_SIGNATURE  SIGNATURE_32 ('A','A','6','4')
#define X64_PAGE_MAP_SIGNATURE      SIGNATURE_32 ('X','6','4',' ')

// When the page/translation table is parsed to create an array of PAGE_MAP_ENTRY, the following bitmasks
// are used to determine the attributes of one page/translation table entry are the same as another
// page/translation table entry. If contiguous leaf/block entries have the same attributes, then they
// will be represented in a single PAGE_MAP_ENTRY.
#define AARCH64_ATTRIBUTES_MASK  ((0xFFFULL << 52) | (0x3FFULL << 2))
#define X64_ATTRIBUTES_MASK      ((0xFFFULL << 52) | 0xFFFULL)

/**
  Populate the input page/translation table map.

  @param[in, out]      Map         Pointer to the PAGE_MAP struct to be populated.

  @retval RETURN_SUCCESS           The translation table is parsed successfully.
  @retval RETURN_INVALID_PARAMETER MapCount is NULL, or Map is NULL but *MapCount is nonzero.
  @retval RETURN_BUFFER_TOO_SMALL  *MapCount is too small.
                                   MapCount is updated to indicate the expected number of entries.
                                   Caller may still get RETURN_BUFFER_TOO_SMALL with the new MapCount.
**/
EFI_STATUS
EFIAPI
CreateFlatPageTable (
  IN OUT PAGE_MAP  *Map
  );

/**
  Checks the input flat page/translation table for the input region and converts the associated
  table entries to EFI access attributes.

  @param[in]  Map                 Pointer to the PAGE_MAP struct to be parsed
  @param[in]  Address             Start address of the region
  @param[in]  Length              Length of the region
  @param[out] Attributes          EFI Attributes of the region (EFI_MEMORY_XP, EFI_MEMORY_RO, EFI_MEMORY_RP)

  @retval EFI_SUCCESS             The output Attributes is valid
  @retval EFI_INVALID_PARAMETER   The flat translation table has not been built or
                                  Attributes was NULL or Length was 0
  @retval EFI_NOT_FOUND           The input region could not be found.
**/
EFI_STATUS
EFIAPI
GetRegionAccessAttributes (
  IN PAGE_MAP  *Map,
  IN UINT64    Address,
  IN UINT64    Length,
  OUT UINT64   *Attributes
  );

/**
  Parses the input page to determine if it is writable.

  @param[in] Page The page entry to parse.

  @retval TRUE    The page is writable.
  @retval FALSE   The page is not writable.
**/
BOOLEAN
EFIAPI
IsPageWritable (
  IN UINT64  Page
  );

/**
  Parses the input page to determine if it is executable.

  @param[in] Page The page entry to parse.

  @retval TRUE    The page is executable.
  @retval FALSE   The page is not executable.
**/
BOOLEAN
EFIAPI
IsPageExecutable (
  IN UINT64  Page
  );

/**
  Parses the input page to determine if it is readable.

  @param[in] Page The page entry to parse.

  @retval TRUE    The page is readable.
  @retval FALSE   The page is not readable.
**/
BOOLEAN
EFIAPI
IsPageReadable (
  IN UINT64  Page
  );

#endif
