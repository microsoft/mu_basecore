/** @file
  X64 specific page table attribute library functions.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/CpuPageTableLib.h>
#include <Library/DebugLib.h>
#include <Library/FlatPageTableLib.h>

#define X64_PRESENT_BIT  BIT0
#define X64_RW_BIT       BIT1
#define X64_NX_BIT       BIT63

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
  )
{
  IA32_CR4     Cr4;
  PAGING_MODE  PagingMode;

  ASSERT (sizeof (PAGE_MAP_ENTRY) == sizeof (IA32_MAP_ENTRY));

  if ((Map == NULL) || ((Map->Entries == NULL) && (Map->EntryCount != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  Map->ArchSignature = X64_PAGE_MAP_SIGNATURE;

  // Poll CR4 to deterimine the page table depth
  Cr4.UintN = AsmReadCr4 ();

  if (Cr4.Bits.LA57 != 0) {
    PagingMode = Paging5Level;
  } else {
    PagingMode = Paging4Level;
  }

  return PageTableParse (AsmReadCr3 (), PagingMode, (IA32_MAP_ENTRY *)Map->Entries, &Map->EntryCount);
}

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
  )
{
  return (Page & X64_RW_BIT) != 0;
}

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
  )
{
  return (Page & X64_NX_BIT) == 0;
}

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
  )
{
  return (Page & X64_PRESENT_BIT) != 0;
}
