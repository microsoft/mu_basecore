/** @file

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <Uefi.h>

VOID *
EFIAPI
SetMem (
  OUT VOID  *Buffer,
  IN UINTN  Length,
  IN UINT8  Value
  )
{
  memset (Buffer, Value, Length);
  return Buffer;
}

VOID *
EFIAPI
SetMem16 (
  OUT VOID   *Buffer,
  IN UINTN   Length,
  IN UINT16  Value
  )
{
  for (; Length != 0; Length--) {
    ((UINT16*)Buffer)[Length - 1] = Value;
  }
  return Buffer;
}

VOID *
EFIAPI
SetMem32 (
  OUT VOID   *Buffer,
  IN UINTN   Length,
  IN UINT32  Value
  )
{
  for (; Length != 0; Length--) {
    ((UINT32*)Buffer)[Length - 1] = Value;
  }
  return Buffer;
}

VOID *
EFIAPI
SetMem64 (
  OUT VOID   *Buffer,
  IN UINTN   Length,
  IN UINT64  Value
  )
{
  for (; Length != 0; Length--) {
    ((UINT64*)Buffer)[Length - 1] = Value;
  }
  return Buffer;
}

VOID *
EFIAPI
SetMemN (
  OUT VOID  *Buffer,
  IN UINTN  Length,
  IN UINTN  Value
  )
{
  if (sizeof (UINTN) == sizeof (UINT64)) {
    return SetMem64 (Buffer, Length, (UINT64)Value);
  } else {
    return SetMem32 (Buffer, Length, (UINT32)Value);
  }
}

VOID *
EFIAPI
ZeroMem (
  OUT VOID  *Buffer,
  IN UINTN  Length
  )
{
  memset (Buffer, 0, Length);
  return Buffer;
}

VOID *
EFIAPI
CopyMem (
  OUT VOID       *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  )
{
  memmove (DestinationBuffer, SourceBuffer, Length);
  return DestinationBuffer;
}

INTN
EFIAPI
CompareMem (
  IN CONST VOID  *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  )
{
  return memcmp (DestinationBuffer, SourceBuffer, Length);
}

BOOLEAN
EFIAPI
CompareGuid (
  IN CONST GUID  *Guid1,
  IN CONST GUID  *Guid2
  )
{
  return ((BOOLEAN)(memcmp (Guid1, Guid2, sizeof (GUID)) == 0));
}

GUID *
EFIAPI
CopyGuid (
  OUT GUID       *DestinationGuid,
  IN CONST GUID  *SourceGuid
  )
{
  memmove (DestinationGuid, SourceGuid, sizeof(GUID));
  return DestinationGuid;
}

UINT8 mZeroGuid[sizeof(GUID)] = {0};

BOOLEAN
EFIAPI
IsZeroGuid (
  IN CONST GUID  *Guid
  )
{
  return ((BOOLEAN)(memcmp (Guid, mZeroGuid, sizeof (GUID)) == 0));
}

VOID *
EFIAPI
ScanMem8 (
  IN CONST VOID  *Buffer,
  IN UINTN       Length,
  IN UINT8       Value
  )
{
  return memchr (Buffer, Value, Length);
}

