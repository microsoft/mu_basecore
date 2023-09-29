/** @file
  Library to parse page/translation table entries.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/SafeIntLib.h>
#include <Library/FlatPageTableLib.h>

// TRUE if A and B have overlapping intervals
#define CHECK_OVERLAP(AStart, AEnd, BStart, BEnd) \
  ((AStart <= BStart && AEnd > BStart) || \
  (BStart <= AStart && BEnd > AStart))

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
  )
{
  UINTN    Index;
  UINT64   EntryStartAddress;
  UINT64   EntryEndAddress;
  UINT64   InputEndAddress;
  BOOLEAN  FoundRange;

  if ((Map->Entries == NULL) || (Map->EntryCount == 0) || (Attributes == NULL) || (Length == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  FoundRange      = FALSE;
  Index           = 0;
  InputEndAddress = 0;

  if (EFI_ERROR (SafeUint64Add (Address, Length - 1, &InputEndAddress))) {
    return EFI_INVALID_PARAMETER;
  }

  do {
    EntryStartAddress = Map->Entries[Index].LinearAddress;
    if (EFI_ERROR (SafeUint64Add (Map->Entries[Index].LinearAddress, Map->Entries[Index].Length - 1, &EntryEndAddress))) {
      return EFI_ABORTED;
    }

    if (CHECK_OVERLAP (Address, InputEndAddress, EntryStartAddress, EntryEndAddress)) {
      if (!FoundRange) {
        *Attributes = EFI_MEMORY_ACCESS_MASK;
        FoundRange  = TRUE;
      }

      if (IsPageExecutable (Map->Entries[Index].PageEntry)) {
        *Attributes &= ~EFI_MEMORY_XP;
      }

      if (IsPageWritable (Map->Entries[Index].PageEntry)) {
        *Attributes &= ~EFI_MEMORY_RO;
      }

      if (IsPageReadable (Map->Entries[Index].PageEntry)) {
        *Attributes &= ~EFI_MEMORY_RP;
      }

      Address = EntryEndAddress + 1;
    }

    if (EntryEndAddress >= InputEndAddress) {
      break;
    }
  } while (++Index < Map->EntryCount);

  return FoundRange ? EFI_SUCCESS : EFI_NOT_FOUND;
}
