/** @file -- PagingAuditProcessor.c

Platform specific memory audit functions.
Copyright (c) Microsoft Corporation. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/ArmLib.h>
#include "MemoryAttributeProtocolFuncTestApp.h"

#define TT_ADDRESS_MASK  (0xFFFFFFFFFULL << 12)

#define IS_TABLE(page, level)       ((level == 3) ? FALSE : (((page) & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY))
#define IS_BLOCK(page, level)       ((level == 3) ? (((page) & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY_LEVEL3) : ((page & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY))
#define ROOT_TABLE_LEN(T0SZ)        (TT_ENTRY_COUNT >> ((T0SZ) - 16) % 9)

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
  UINT64                *Pml0          = NULL;
  UINT64                *Pte1G         = NULL;
  UINT64                *Pte2M         = NULL;
  UINT64                Index2         = 0;
  UINT64                Index1         = 0;
  UINT64                Index0         = 0;
  UINT64                RootEntryCount = 0;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  BaseAddress;

  Pml0 = (UINT64 *)ArmGetTTBR0BaseAddress ();

  if (Pml0 == NULL) {
    return EFI_NOT_FOUND;
  }

  RootEntryCount = ROOT_TABLE_LEN (ArmGetTCR () & TCR_T0SZ_MASK);

  for (Index0 = 0x0; Index0 < RootEntryCount; Index0++) {
    if (!IS_TABLE (Pml0[Index0], 0)) {
      continue;
    }

    Pte1G = (UINT64 *)(Pml0[Index0] & TT_ADDRESS_MASK);

    for (Index1 = 0x1; Index1 < TT_ENTRY_COUNT; Index1++ ) {
      if ((Pte1G[Index1] & 0x1) == 0) {
        continue;
      }

      if (!IS_BLOCK (Pte1G[Index1], 1)) {
        Pte2M = (UINT64 *)(Pte1G[Index1] & TT_ADDRESS_MASK);

        for (Index2 = 0x0; Index2 < TT_ENTRY_COUNT; Index2++ ) {
          if ((Pte2M[Index2] & 0x1) == 0) {
            continue;
          }

          if (!IS_BLOCK (Pte2M[Index2], 2)) {
            continue;
          } else {
            BaseAddress = (EFI_PHYSICAL_ADDRESS)(Pte2M[Index2] & TT_ADDRESS_MASK);
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
  UINT64  *Pml0  = NULL;
  UINT64  *Pte1G = NULL;
  UINT64  *Pte2M = NULL;
  UINT64  Index2 = 0;
  UINT64  Index1 = 0;
  UINT64  Index0 = 0;

  Pml0 = (UINT64 *)ArmGetTTBR0BaseAddress ();

  if (Pml0 == NULL) {
    return TT_UXN_MASK;
  }

  Index0 = (Address >> 39) & (TT_ENTRY_COUNT - 1);
  Index1 = (Address >> 30) & (TT_ENTRY_COUNT - 1);
  Index2 = (Address >> 21) & (TT_ENTRY_COUNT - 1);

  Pte1G = (UINT64 *)(Pml0[Index0] & TT_ADDRESS_MASK);
  Pte2M = (UINT64 *)(Pte1G[Index1] & TT_ADDRESS_MASK);
  return Pte2M[Index2] & TT_UXN_MASK;
}
