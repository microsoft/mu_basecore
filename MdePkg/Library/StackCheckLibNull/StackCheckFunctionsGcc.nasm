;------------------------------------------------------------------------------
; StackCheckFunctionsGcc.nasm
;
; Copyright (c) Microsoft Corporation. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;------------------------------------------------------------------------------
    DEFAULT REL
    SECTION .text

; Called when a stack cookie check fails.
global ASM_PFX(__stack_chk_fail)
ASM_PFX(__stack_chk_fail):
    ret
