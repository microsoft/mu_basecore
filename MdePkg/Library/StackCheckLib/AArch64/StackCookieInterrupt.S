//------------------------------------------------------------------------------
// AArch64/StackCookieInterrupt.S
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//------------------------------------------------------------------------------

#include <Library/StackCheckLib.h>  // MU_CHANGE: CLANGPDB Stack Cookies

    .text

//------------------------------------------------------------------------------
// MU_CHANGE BEGIN: CLANGPDB Stack Cookies
// Calls an interrupt using the stack cookie exception vector
// and passes the ExceptionAddress through in x0.
//
// VOID
// EFIAPI
// TriggerStackCookieInterrupt (
//   EFI_PHYSICAL_ADDRESS ExceptionAddress
//   );
// MU_CHANGE END: CLANGPDB Stack Cookies
//------------------------------------------------------------------------------
.global ASM_PFX(TriggerStackCookieInterrupt)
ASM_PFX(TriggerStackCookieInterrupt):
// MU_CHANGE BEGIN: CLANGPDB Stack Cookies
    AARCH64_BTI(c)
    svc STACK_CHECK_ERROR_HANDLER_VECTOR
// MU_CHANGE END: CLANGPDB Stack Cookies
    ret
