;------------------------------------------------------------------------------
; StackCheckFunctionsGcc.nasm
;
; Copyright (c) Microsoft Corporation. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;------------------------------------------------------------------------------
    DEFAULT REL
    SECTION .text

extern ASM_PFX(StackCheckFailure)

; Called when a stack cookie check fails.
global ASM_PFX(__stack_chk_fail)
ASM_PFX(__stack_chk_fail):
    call StackCheckFailure
    int FixedPcdGet8 (PcdStackCookieExceptionVector)
    ret
