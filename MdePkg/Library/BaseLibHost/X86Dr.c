/** @file
  Base Library CPU Functions for all architectures.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

UINTN  gDr0;
UINTN  gDr1;
UINTN  gDr2;
UINTN  gDr3;
UINTN  gDr4;
UINTN  gDr5;
UINTN  gDr6;
UINTN  gDr7;

UINTN
EFIAPI
AsmReadDr0 (
  VOID
  )
{
  return gDr0;
}

UINTN
EFIAPI
AsmReadDr1 (
  VOID
  )
{
  return gDr1;
}

UINTN
EFIAPI
AsmReadDr2 (
  VOID
  )
{
  return gDr2;
}

UINTN
EFIAPI
AsmReadDr3 (
  VOID
  )
{
  return gDr3;
}

UINTN
EFIAPI
AsmReadDr4 (
  VOID
  )
{
  return gDr4;
}

UINTN
EFIAPI
AsmReadDr5 (
  VOID
  )
{
  return gDr5;
}

UINTN
EFIAPI
AsmReadDr6 (
  VOID
  )
{
  return gDr6;
}

UINTN
EFIAPI
AsmReadDr7 (
  VOID
  )
{
  return gDr7;
}

UINTN
EFIAPI
AsmWriteDr0 (
  UINTN  Dr0
  )
{
  gDr0 = Dr0;
  return Dr0;
}

UINTN
EFIAPI
AsmWriteDr1 (
  UINTN  Dr1
  )
{
  gDr1 = Dr1;
  return Dr1;
}

UINTN
EFIAPI
AsmWriteDr2 (
  UINTN  Dr2
  )
{
  gDr2 = Dr2;
  return Dr2;
}

UINTN
EFIAPI
AsmWriteDr3 (
  UINTN  Dr3
  )
{
  gDr3 = Dr3;
  return Dr3;
}

UINTN
EFIAPI
AsmWriteDr4 (
  UINTN  Dr4
  )
{
  gDr4 = Dr4;
  return Dr4;
}

UINTN
EFIAPI
AsmWriteDr5 (
  UINTN  Dr5
  )
{
  gDr5 = Dr5;
  return Dr5;
}

UINTN
EFIAPI
AsmWriteDr6 (
  UINTN  Dr6
  )
{
  gDr6 = Dr6;
  return Dr6;
}

UINTN
EFIAPI
AsmWriteDr7 (
  UINTN  Dr7
  )
{
  gDr7 = Dr7;
  return Dr7;
}
