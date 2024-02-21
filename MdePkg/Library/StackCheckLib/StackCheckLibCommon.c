/** @file
  Provides the required functionality for handling stack
  cookie check failures.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/StackCheckFailureHookLib.h>

/**
  Calls an interrupt using the vector specified by PcdStackCookieExceptionVector
**/
VOID
TriggerStackCookieInterrupt (
  VOID
  );

#if defined (__GNUC__) || defined (__clang__)

VOID  *__stack_chk_guard = (VOID *)(UINTN)STACK_COOKIE_VALUE;

VOID
__stack_chk_fail (
  VOID
  )
{
  StackCheckFailureHook (RETURN_ADDRESS (0));
  TriggerStackCookieInterrupt ();
}

#elif defined (_MSC_VER)
VOID  *__security_cookie = (VOID *)(UINTN)STACK_COOKIE_VALUE;

NORETURN VOID __cdecl
__report_rangecheckfailure (
  VOID
  )
{
  CpuDeadLoop ();
}

NORETURN VOID __cdecl
__GSHandlerCheck (
  VOID
  )
{
  CpuDeadLoop ();
}

VOID
StackCheckFailure (
  VOID  *ActualCookieValue
  )
{
  StackCheckFailureHook (RETURN_ADDRESS (0));
  TriggerStackCookieInterrupt ();
}

#endif
