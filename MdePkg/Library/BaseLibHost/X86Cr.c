/** @file
  Base Library CPU Functions for all architectures.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>

UINTN  gCr0;
UINTN  gCr2;
UINTN  gCr3;
UINTN  gCr4;

/**
  Reads the current value of the Control Register 0 (CR0).

  Reads and returns the current value of CR0. This function is only available
  on IA-32 and x64. This returns a 32-bit value on IA-32 and a 64-bit value on
  x64.

  @return The value of the Control Register 0 (CR0).

**/
UINTN
EFIAPI
AsmReadCr0 (
  VOID
  )
{
  return gCr0;
}


/**
  Reads the current value of the Control Register 2 (CR2).

  Reads and returns the current value of CR2. This function is only available
  on IA-32 and x64. This returns a 32-bit value on IA-32 and a 64-bit value on
  x64.

  @return The value of the Control Register 2 (CR2).

**/
UINTN
EFIAPI
AsmReadCr2 (
  VOID
  )
{
  return gCr2;
}


/**
  Reads the current value of the Control Register 3 (CR3).

  Reads and returns the current value of CR3. This function is only available
  on IA-32 and x64. This returns a 32-bit value on IA-32 and a 64-bit value on
  x64.

  @return The value of the Control Register 3 (CR3).

**/
UINTN
EFIAPI
AsmReadCr3 (
  VOID
  )
{
  return gCr3;
}


/**
  Reads the current value of the Control Register 4 (CR4).

  Reads and returns the current value of CR4. This function is only available
  on IA-32 and x64. This returns a 32-bit value on IA-32 and a 64-bit value on
  x64.

  @return The value of the Control Register 4 (CR4).

**/
UINTN
EFIAPI
AsmReadCr4 (
  VOID
  )
{
  return gCr4;
}


/**
  Writes a value to Control Register 0 (CR0).

  Writes and returns a new value to CR0. This function is only available on
  IA-32 and x64. This writes a 32-bit value on IA-32 and a 64-bit value on x64.

  @param  Cr0 The value to write to CR0.

  @return The value written to CR0.

**/
UINTN
EFIAPI
AsmWriteCr0 (
  UINTN  Cr0
  )
{
  gCr0 = Cr0;
  return Cr0;
}

/**
  Writes a value to Control Register 2 (CR2).

  Writes and returns a new value to CR2. This function is only available on
  IA-32 and x64. This writes a 32-bit value on IA-32 and a 64-bit value on x64.

  @param  Cr2 The value to write to CR2.

  @return The value written to CR2.

**/
UINTN
EFIAPI
AsmWriteCr2 (
  UINTN  Cr2
  )
{
  gCr2 = Cr2;
  return Cr2;
}


/**
  Writes a value to Control Register 3 (CR3).

  Writes and returns a new value to CR3. This function is only available on
  IA-32 and x64. This writes a 32-bit value on IA-32 and a 64-bit value on x64.

  @param  Cr3 The value to write to CR3.

  @return The value written to CR3.

**/
UINTN
EFIAPI
AsmWriteCr3 (
  UINTN  Cr3
  )
{
  gCr3 = Cr3;
  return Cr3;
}


/**
  Writes a value to Control Register 4 (CR4).

  Writes and returns a new value to CR4. This function is only available on
  IA-32 and x64. This writes a 32-bit value on IA-32 and a 64-bit value on x64.

  @param  Cr4 The value to write to CR4.

  @return The value written to CR4.

**/
UINTN
EFIAPI
AsmWriteCr4 (
  UINTN  Cr4
  )
{
  gCr4 = Cr4;
  return Cr4;
}
