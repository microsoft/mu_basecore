/** @file
  Long Jump functions.

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <Library/BaseLib.h>

/**
  Restores the CPU context that was saved with SetJump().

  Restores the CPU context from the buffer specified by JumpBuffer. This
  function never returns to the caller. Instead is resumes execution based on
  the state of JumpBuffer.

  If JumpBuffer is NULL, then ASSERT().
  For Itanium processors, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().
  If Value is 0, then ASSERT().

  @param  JumpBuffer  A pointer to CPU context buffer.
  @param  Value       The value to return when the SetJump() context is
                      restored and must be non-zero.

**/
VOID
EFIAPI
LongJump (
  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer,
  IN      UINTN                     Value
  )
{
  jmp_buf  local_buf;
  jmp_buf  *buf;

  buf = *(VOID **)JumpBuffer;
  memcpy (&local_buf, buf, sizeof(jmp_buf));
  free (buf);
  longjmp (local_buf, (int)Value);
}
