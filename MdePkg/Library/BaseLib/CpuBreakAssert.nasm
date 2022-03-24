;------------------------------------------------------------------------------
;
; Copyright (c) Microsoft Corporation. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   CpuBreakAssert.nasm
;
; Abstract:
;
;   CpuBreakAssert function
;
; Notes:
;  Generates a debugger assertion break on the CPU.
;
;  This does a special break into the debugger such that the debugger knows
;  that the code running has hit an assertion, not a generic breakpoint.
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; CpuBreakAssert (
;  VOID
;  );
;------------------------------------------------------------------------------
global ASM_PFX(CpuBreakAssert)
ASM_PFX(CpuBreakAssert):
    int 2ch
    ret

