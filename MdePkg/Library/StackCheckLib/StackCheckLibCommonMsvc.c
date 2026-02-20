/** @file
  Provides the required functionality for handling stack
  cookie check failures for MSVC.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>

// MU_CHANGE BEGIN: CLANGPDB Stack Cookies - removed DebugLib dependency
// #include <Library/DebugLib.h>
// MU_CHANGE END: CLANGPDB Stack Cookies
// MU_CHANGE BEGIN: CLANGPDB Stack Cookies - removed BaseLib dependency
// #include <Library/BaseLib.h>
// MU_CHANGE END: CLANGPDB Stack Cookies
#include <Uefi/UefiBaseType.h>  // MU_CHANGE: CLANGPDB Stack Cookies
#include <Library/StackCheckLib.h>
// MU_CHANGE BEGIN: CLANGPDB Stack Cookies - removed StackCheckFailureHookLib dependency
// #include <Library/StackCheckFailureHookLib.h>
// MU_CHANGE END: CLANGPDB Stack Cookies

// MU_CHANGE BEGIN: CLANGPDB Stack Cookies
/**
  Triggers an interrupt using the stack cookie exception vector.
**/
VOID
EFIAPI
TriggerStackCookieInterrupt (
  EFI_PHYSICAL_ADDRESS  ExceptionAddress
  );
// MU_CHANGE END: CLANGPDB Stack Cookies

VOID  *__security_cookie = (VOID *)(UINTN)STACK_COOKIE_VALUE;

/**
  This function gets called when an MSVC generated stack cookie fails. This implementation calls into a platform
  failure hook lib and then triggers the stack cookie interrupt.

  @param[in] ActualCookieValue  The value that was written onto the stack, corrupting the stack cookie.

**/
VOID
StackCheckFailure (
  VOID  *ActualCookieValue
  )
{
  // MU_CHANGE BEGIN: CLANGPDB Stack Cookies - removed DebugLib dependency
  // DEBUG ((DEBUG_ERROR, "Stack cookie check failed at address 0x%llx!\n", RETURN_ADDRESS (0)));
  // MU_CHANGE END: CLANGPDB Stack Cookies
  // MU_CHANGE BEGIN: CLANGPDB Stack Cookies - removed StackCheckFailureHookLib dependency
  // StackCheckFailureHook (RETURN_ADDRESS (0));
  // MU_CHANGE END: CLANGPDB Stack Cookies
  TriggerStackCookieInterrupt ((EFI_PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0));  // MU_CHANGE: CLANGPDB Stack Cookies
}
