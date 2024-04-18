/** @file
  Library which provides a hook that is called when a cpu exception occurs

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CPU_EXCEPTION_HOOK_LIB_H_
#define CPU_EXCEPTION_HOOK_LIB_H_

#include <Uefi.h>
#include <Protocol/DebugSupport.h>

NO_STACK_COOKIE
VOID
EFIAPI
CpuExceptionHookLib (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

#endif
