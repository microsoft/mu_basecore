/** @file
  Defines the stack cookie variable for GCC, Clang and MSVC compilers.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#if defined (__GNUC__) || defined (__clang__)
VOID  *__stack_chk_guard = (VOID *)0x0;
#elif defined (_MSC_VER)
UINTN  __security_cookie = 0x0;
#endif
