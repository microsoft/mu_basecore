; MU_CHANGE - START
;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
; Copyright (c) Microsoft Corporation.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   EnableInterruptsAndSleep.nasm
;
; Abstract:
;
;   EnableInterruptsAndSleep function
;
; Notes:
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

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
