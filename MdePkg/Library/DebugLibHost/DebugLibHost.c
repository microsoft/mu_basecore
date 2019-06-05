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
#include <stdarg.h>

#include <Uefi.h>

#ifndef HOST_DEBUG_MESSAGE
#define HOST_DEBUG_MESSAGE 0
#endif

//
// Define the maximum debug and assert message length that this library supports
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100

VOID
EFIAPI
DebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
#ifndef TEST_WITH_KLEE
  printf ("ASSERT: %s(%d): %s\n", FileName, (INT32)(UINT32)LineNumber, Description);
#endif
}

BOOLEAN
EFIAPI
DebugAssertEnabled (
  VOID
  )
{
  return TRUE;
}

VOID
PatchFormat (
  IN  CONST CHAR8  *Format,
  IN        CHAR8  *MyFormat
  )
{
  UINTN  Index;
  UINTN  MyIndex;

  Index = 0;
  MyIndex = 0;
  while (Format[Index] != 0) {
    MyFormat[MyIndex] = Format[Index];
    if (Format[Index] == '%') {
      Index++;
      MyIndex++;
      switch (Format[Index]) {
      case 'a':
        MyFormat[MyIndex] = 's';
        break;
      case 's':
        MyFormat[MyIndex] = 'w';
        MyIndex++;
        MyFormat[MyIndex] = 's';
        break;
      case 'g':
      case 't':
        MyFormat[MyIndex] = 'p';
        break;
      case 'r':
        MyFormat[MyIndex] = 'x';
        break;
      case 'L':
      case 'l':
        MyFormat[MyIndex] = 'I';
        MyIndex++;
        MyFormat[MyIndex] = '6';
        MyIndex++;
        MyFormat[MyIndex] = '4';
        break;
      case '0':
        MyFormat[MyIndex] = Format[Index];
        if (Format[Index + 1] == '1') {
          Index++;
          MyIndex++;
          MyFormat[MyIndex] = Format[Index];
        }
      case '1':
        MyFormat[MyIndex] = Format[Index];
        if (Format[Index + 1] == '6') {
          Index++;
          MyIndex++;
          MyFormat[MyIndex] = Format[Index];
        }
        if (Format[Index + 1] == 'l') {
          Index++;
          MyIndex++;
          MyFormat[MyIndex] = 'I';
          MyIndex++;
          MyFormat[MyIndex] = '6';
          MyIndex++;
          MyFormat[MyIndex] = '4';
        }
        if (Format[Index + 1] == 'l') {
          Index++;
        }
        break;
      default:
        MyFormat[MyIndex] = Format[Index];
        break;
      }
    }
    Index++;
    MyIndex++;
  }
  MyFormat[MyIndex] = 0;
}

VOID
EFIAPI
DebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  )
{
#ifndef TEST_WITH_KLEE
#if HOST_DEBUG_MESSAGE
  CHAR8    Buffer[MAX_DEBUG_MESSAGE_LENGTH];
  CHAR8    MyFormat[MAX_DEBUG_MESSAGE_LENGTH];
  VA_LIST  Marker;

  VA_START (Marker, Format);

  PatchFormat (Format, MyFormat);

  vsprintf (Buffer, MyFormat, Marker);
  VA_END (Marker);

  printf ("%s", Buffer);
#endif
#endif
}

BOOLEAN
EFIAPI
DebugPrintEnabled (
  VOID
  )
{
  return TRUE;
}

BOOLEAN
EFIAPI
DebugPrintLevelEnabled (
  IN  CONST UINTN        ErrorLevel
  )
{
  return TRUE;
}

BOOLEAN
EFIAPI
DebugCodeEnabled (
  VOID
  )
{
  return TRUE;
}
