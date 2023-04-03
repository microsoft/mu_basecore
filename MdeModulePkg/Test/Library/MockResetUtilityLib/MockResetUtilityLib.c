/** @file
  Mocked version of ResetUtilityLib

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UnitTestLib.h>

/**
  Simple helper to reset the system with a subtype GUID, and a default
  to fall back on (that isn't necessarily EfiResetPlatformSpecific).
  TODO VNK: was this used anywhere?

  @param[in]  ResetType     The default EFI_RESET_TYPE of the reset.
  @param[in]  ResetSubtype  GUID pointer for the reset subtype to be used.

  @retval     Pointer to the GUIDed reset data structure. Structure is
              SIMPLE_GUIDED_RESET_DATA_SIZE in size.

**/
VOID
EFIAPI
ResetSystemWithSubtype (
  IN EFI_RESET_TYPE  ResetType,
  IN CONST  GUID     *ResetSubtype
  )
{
  BASE_LIBRARY_JUMP_BUFFER  *JumpBuf;

  check_expected (ResetSubtype);

  JumpBuf = (BASE_LIBRARY_JUMP_BUFFER *)mock ();

  LongJump (JumpBuf, 1);
}
