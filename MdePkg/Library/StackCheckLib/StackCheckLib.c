/** @file
  Provides the required functionality for initializing and
  checking the stack cookie.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffLib.h>
#include <Library/RngLib.h>
#include <Library/StackCheckFailureLib.h>

#if defined (__GNUC__) || defined (__clang__)
VOID  *__stack_chk_guard = (VOID *)0xBEEBE;
#elif defined (_MSC_VER)
UINTN  __security_cookie = 0xBEEBE;
#endif

/**
  Initialize the security cookie.
**/
NO_STACK_COOKIE
EFI_STATUS
EFIAPI
InitializeSecurityCookie (
  VOID
  )
{
  // Only generate a random value for MSVC builds. GCC builds which use
  // -fstack-protector-strong or -fstack-protector-all may encounter a
  // stack overflow with a random cookie value. The exact cause of this
  // behavior needs to be investigated further.
 #ifdef _MSC_VER
  if (sizeof (UINTN) == sizeof (UINT64)) {
    GetRandomNumber64 ((UINT64 *)&__security_cookie);
  } else {
    GetRandomNumber32 ((UINT32 *)&__security_cookie);
  }

 #endif

  return EFI_SUCCESS;
}
