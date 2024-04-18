/** @file
  Library provides a hook called when a stack cookie check fails.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>
#include <Library/CpuExceptionHookLib.h>

/**
*/
NO_STACK_COOKIE
VOID
EFIAPI
CpuExceptionHookLib (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  return;
}
