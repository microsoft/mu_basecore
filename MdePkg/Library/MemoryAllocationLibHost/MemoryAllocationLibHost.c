/** @file

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <Uefi.h>

#define PAGE_HEAD_PRIVATE_SIGNATURE  SIGNATURE_32 ('P', 'H', 'D', 'R')

typedef struct {
  UINT32 Signature;
  VOID   *AllocatedBufffer;
  UINTN  TotalPages;
  VOID   *AlignedBuffer;
  UINTN  AlignedPages;
} PAGE_HEAD;

VOID *
EFIAPI
AllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  );

VOID
EFIAPI
FreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  );

VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
{
  return AllocateAlignedPages (Pages, SIZE_4KB);
}

VOID *
EFIAPI
AllocateRuntimePages (
  IN UINTN  Pages
  )
{
  return AllocatePages (Pages);
}

VOID *
EFIAPI
AllocateReservedPages (
  IN UINTN  Pages
  )
{
  return AllocatePages (Pages);
}

VOID
EFIAPI
FreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  FreeAlignedPages (Buffer, Pages);
}

VOID *
EFIAPI
AllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  PAGE_HEAD             PageHead;
  PAGE_HEAD             *PageHeadPtr;
  UINTN                 AlignmentMask;

  assert ((Alignment & (Alignment - 1)) == 0);

  if (Alignment < SIZE_4KB) {
    Alignment = SIZE_4KB;
  }
  AlignmentMask  = Alignment - 1;

  //
  // We need reserve Alignment pages for PAGE_HEAD, as meta data.
  //

  PageHead.Signature = PAGE_HEAD_PRIVATE_SIGNATURE;
  PageHead.TotalPages = Pages + EFI_SIZE_TO_PAGES(Alignment) * 2;
  PageHead.AlignedPages = Pages;
  PageHead.AllocatedBufffer = malloc (EFI_PAGES_TO_SIZE(PageHead.TotalPages));
  if (PageHead.AllocatedBufffer == NULL) {
    return NULL;
  }
  PageHead.AlignedBuffer = (VOID *)(((UINTN) PageHead.AllocatedBufffer + AlignmentMask) & ~AlignmentMask);
  if ((UINTN)PageHead.AlignedBuffer - (UINTN)PageHead.AllocatedBufffer < sizeof(PAGE_HEAD)) {
    PageHead.AlignedBuffer = (VOID *)((UINTN)PageHead.AlignedBuffer + Alignment);
  }

  PageHeadPtr = (VOID *)((UINTN)PageHead.AlignedBuffer - sizeof(PAGE_HEAD));
  memcpy (PageHeadPtr, &PageHead, sizeof(PAGE_HEAD));

  return PageHead.AlignedBuffer;
}

VOID *
EFIAPI
AllocateAlignedRuntimePages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return AllocateAlignedPages (Pages, Alignment);
}

VOID *
EFIAPI
AllocateAlignedReservedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return AllocateAlignedPages (Pages, Alignment);
}

VOID
EFIAPI
FreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  PAGE_HEAD             *PageHeadPtr;

  //
  // NOTE: Partial free is not supported. Just keep it.
  //

  PageHeadPtr = (VOID *)((UINTN)Buffer - sizeof(PAGE_HEAD));
  if (PageHeadPtr->Signature != PAGE_HEAD_PRIVATE_SIGNATURE) {
    return ;
  }
  if (PageHeadPtr->AlignedPages != Pages) {
    return ;
  }

  PageHeadPtr->Signature = 0;
  free (PageHeadPtr->AllocatedBufffer);
}

VOID *
EFIAPI
AllocatePool (
  IN UINTN  AllocationSize
  )
{
  return malloc (AllocationSize);
}

VOID *
EFIAPI
AllocateRuntimePool (
  IN UINTN  AllocationSize
  )
{
  return AllocatePool (AllocationSize);
}

VOID *
EFIAPI
AllocateReservedPool (
  IN UINTN  AllocationSize
  )
{
  return AllocatePool (AllocationSize);
}

VOID *
EFIAPI
AllocateZeroPool (
  IN UINTN  AllocationSize
  )
{
  VOID *Buffer;
  Buffer = malloc (AllocationSize);
  if (Buffer == NULL) {
    return NULL;
  }
  memset (Buffer, 0, AllocationSize);
  return Buffer;
}

VOID *
EFIAPI
AllocateRuntimeZeroPool (
  IN UINTN  AllocationSize
  )
{
  return AllocateZeroPool (AllocationSize);
}

VOID *
EFIAPI
AllocateReservedZeroPool (
  IN UINTN  AllocationSize
  )
{
  return AllocateZeroPool (AllocationSize);
}

VOID *
EFIAPI
AllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  VOID  *Memory;  
  Memory = malloc (AllocationSize);
  if (Memory == NULL) {
    return NULL;
  }
  memcpy (Memory, Buffer, AllocationSize);
  return Memory;
}

VOID *
EFIAPI
AllocateRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return AllocateCopyPool (AllocationSize, Buffer);
}

VOID *
EFIAPI
AllocateReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return AllocateCopyPool (AllocationSize, Buffer);
}

VOID *
EFIAPI
ReallocatePool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer  OPTIONAL
  )
{
  VOID  *NewBuffer;
  NewBuffer = malloc (NewSize);
  if (NewBuffer != NULL && OldBuffer != NULL) {
    memcpy (NewBuffer, OldBuffer, MIN (OldSize, NewSize));
  }
  return NewBuffer;
}

VOID *
EFIAPI
ReallocateRuntimePool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer  OPTIONAL
  )
{
  return ReallocatePool (OldSize, NewSize, OldBuffer);
}

VOID *
EFIAPI
ReallocateReservedPool (
  IN UINTN  OldSize,
  IN UINTN  NewSize,
  IN VOID   *OldBuffer  OPTIONAL
  )
{
  return ReallocatePool (OldSize, NewSize, OldBuffer);
}

VOID
EFIAPI
FreePool (
  IN VOID   *Buffer
  )
{
  free (Buffer);
}
