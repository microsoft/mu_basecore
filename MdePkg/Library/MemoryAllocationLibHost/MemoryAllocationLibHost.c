/** @file

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <Uefi.h>

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

VOID
EFIAPI
FreePool (
  IN VOID   *Buffer
  )
{
  free (Buffer);
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
AllocateRuntimePool (
  IN UINTN  AllocationSize
  )
{
  return AllocatePool (AllocationSize);
}

VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
{
  return malloc (EFI_PAGES_TO_SIZE(Pages));
}

VOID
EFIAPI
FreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  free (Buffer);
}

VOID *
EFIAPI
AllocateAlignedReservedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  )
{
  return malloc (EFI_PAGES_TO_SIZE(Pages));
}
