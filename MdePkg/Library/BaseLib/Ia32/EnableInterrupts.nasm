;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   EnableInterrupts.Asm
;
; Abstract:
;
;   EnableInterrupts function
;
; Notes:
;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; EnableInterrupts (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(EnableInterrupts)
ASM_PFX(EnableInterrupts):
    sti
    ret

; MU_CHANGE - START
;------------------------------------------------------------------------------
; VOID
; EFIAPI
; EnableInterruptsAndSleep (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(EnableInterruptsAndSleep)
ASM_PFX(EnableInterruptsAndSleep):
    sti
    hlt
    ret
; MU_CHANGE - END

