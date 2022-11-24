/** @file
  Common code for the policy service modules.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <PolicyCommon.h>

/**
  Compares two strings considering NULL pointers equivalent.

  @param[in]  Name1     The first string pointer to be compared
  @param[in]  Name2     The second string pointer to be compared

  @retval   TRUE    The pointers are both NULL or identical.
  @retval   FALSE   One of the pointers is NULL or string are not identical.
**/
BOOLEAN
EFIAPI
PolicyCompareNames (
  CONST CHAR16  *Name1 OPTIONAL,
  CONST CHAR16  *Name2 OPTIONAL
  )
{
  if ((Name1 == NULL) && (Name2 == NULL)) {
    return TRUE;
  } else if ((Name1 == NULL) || (Name1 == NULL)) {
    return FALSE;
  } else {
    return (StrCmp (Name1, Name2) == 0);
  }
}
