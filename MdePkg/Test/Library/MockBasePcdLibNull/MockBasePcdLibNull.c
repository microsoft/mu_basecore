/** @file
  Mocked version of BasePcdLib
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
  Mock retrieving a PCD Ptr.
  Returns the pointer to the buffer of the token specified by TokenNumber.
  @param[in]  TokenNumber The PCD token number to retrieve a current value for.
  @return Returns the pointer to the token specified by TokenNumber.
**/
VOID *
EFIAPI
LibPcdGetPtr (
  IN UINTN  TokenNumber
  )
{
  return (VOID *)mock ();
}

/**
  This function provides a means by which to set a value for a given PCD token.
  Sets a buffer for the token specified by TokenNumber to the value specified
  by Buffer and SizeOfBuffer. If SizeOfBuffer is greater than the maximum size
  support by TokenNumber, then set SizeOfBuffer to the maximum size supported by
  TokenNumber and return EFI_INVALID_PARAMETER to indicate that the set operation
  was not actually performed.
  If SizeOfBuffer is set to MAX_ADDRESS, then SizeOfBuffer must be set to the
  maximum size supported by TokenName and EFI_INVALID_PARAMETER must be returned.
  If SizeOfBuffer is NULL, then ASSERT().
  If SizeOfBuffer > 0 and Buffer is NULL, then ASSERT().
  @param[in]      TokenNumber   The PCD token number to set a current value for.
  @param[in, out] SizeOfBuffer  The size, in bytes, of Buffer.
  @param[in]      Buffer        A pointer to the buffer to set.
  @return The status of the set operation.
**/
RETURN_STATUS
EFIAPI
LibPcdSetPtrS (
  IN       UINTN  TokenNumber,
  IN OUT   UINTN  *SizeOfBuffer,
  IN CONST VOID   *Buffer
  )
{
  return (RETURN_STATUS)mock ();
}
