/** @file
  Internal ASSERT () functions for SetJump.

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <Library/BaseLib.h>

/**
  Saves the current CPU context that can be restored with a call to LongJump()
  and returns 0.

  Saves the current CPU context in the buffer specified by JumpBuffer and
  returns 0. The initial call to SetJump() must always return 0. Subsequent
  calls to LongJump() cause a non-zero value to be returned by SetJump().

  If JumpBuffer is NULL, then ASSERT().
  For Itanium processors, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().
  
  NOTE: The structure BASE_LIBRARY_JUMP_BUFFER is CPU architecture specific.
  The same structure must never be used for more than one CPU architecture context.
  For example, a BASE_LIBRARY_JUMP_BUFFER allocated by an IA-32 module must never be used from an x64 module. 
  SetJump()/LongJump() is not currently supported for the EBC processor type.   

  @param  JumpBuffer  A pointer to CPU context buffer.

  @retval 0 Indicates a return from SetJump().

**/
UINTN
EFIAPI
SetJump (
  OUT     BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer
  )
{
  jmp_buf  local_buf;
  jmp_buf  *buf;
  UINTN    Value;

  buf = malloc (sizeof(jmp_buf));
  *(VOID **)JumpBuffer = buf;
  Value = setjmp (local_buf);
  if (Value == 0) {
    memcpy (buf, &local_buf, sizeof(jmp_buf));
  }
  return Value;
}
