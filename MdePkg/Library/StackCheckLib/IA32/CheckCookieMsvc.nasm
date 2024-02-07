;------------------------------------------------------------------------------
; IA32/CheckCookieMsvc.nasm
;
; Copyright (c) Microsoft Corporation. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

extern ASM_PFX(StackCheckFailure)
extern ASM_PFX(__security_cookie)

;------------------------------------------------------------------------------
; Checks the stack cookie value against __security_cookie and calls the
; stack cookie failure handler if there is a mismatch.
;
; VOID
; EFIAPI
; __security_check_cookie (
;   IN UINTN CheckValue
;   );
;------------------------------------------------------------------------------
global @__security_check_cookie@4
@__security_check_cookie@4:
    cmp     ecx, [ASM_PFX(__security_cookie)]
    jne    ASM_PFX(StackCheckFailure)
    ret
