/** @file
  IA-32/x64 AsmReadGdtr()

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Reads the current value of Code Segment Register (CS).

  Reads and returns the current value of CS. This function is only available on
  IA-32 and x64.

  @return The current value of CS.

**/
UINT16
EFIAPI
AsmReadCs (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}


/**
  Reads the current value of Data Segment Register (DS).

  Reads and returns the current value of DS. This function is only available on
  IA-32 and x64.

  @return The current value of DS.

**/
UINT16
EFIAPI
AsmReadDs (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}


/**
  Reads the current value of Extra Segment Register (ES).

  Reads and returns the current value of ES. This function is only available on
  IA-32 and x64.

  @return The current value of ES.

**/
UINT16
EFIAPI
AsmReadEs (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}


/**
  Reads the current value of FS Data Segment Register (FS).

  Reads and returns the current value of FS. This function is only available on
  IA-32 and x64.

  @return The current value of FS.

**/
UINT16
EFIAPI
AsmReadFs (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}


/**
  Reads the current value of GS Data Segment Register (GS).

  Reads and returns the current value of GS. This function is only available on
  IA-32 and x64.

  @return The current value of GS.

**/
UINT16
EFIAPI
AsmReadGs (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}


/**
  Reads the current value of Stack Segment Register (SS).

  Reads and returns the current value of SS. This function is only available on
  IA-32 and x64.

  @return The current value of SS.

**/
UINT16
EFIAPI
AsmReadSs (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}


/**
  Reads the current value of Task Register (TR).

  Reads and returns the current value of TR. This function is only available on
  IA-32 and x64.

  @return The current value of TR.

**/
UINT16
EFIAPI
AsmReadTr (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Load given selector into TR register.

  @param[in] Selector     Task segment selector
**/
VOID
EFIAPI
AsmWriteTr (
  IN UINT16 Selector
  )
{
  ASSERT (FALSE);
}