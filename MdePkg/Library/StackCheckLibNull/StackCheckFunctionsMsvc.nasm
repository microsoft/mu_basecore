;------------------------------------------------------------------------------
; StackCheckFunctionsMsvc.nasm
;
; Copyright (c) Microsoft Corporation. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;------------------------------------------------------------------------------
    DEFAULT REL
    SECTION .text

; Called when a buffer check fails. This functionality is dependent on MSVC
; C runtime libraries and so is unsupported in UEFI.
global ASM_PFX(__report_rangecheckfailure)
ASM_PFX(__report_rangecheckfailure):
    ret

; The GS handler is for checking the stack cookie during SEH or
; EH exceptions and is unsupported in UEFI.
global ASM_PFX(__GSHandlerCheck)
ASM_PFX(__GSHandlerCheck):
    ret

;------------------------------------------------------------------------------
; Checks the stack cookie value against __security_cookie and calls the
; stack cookie failure handler if there is a mismatch.
;
; VOID
; EFIAPI
; __security_check_cookie (
;   IN UINTN CheckValue);
;------------------------------------------------------------------------------
global ASM_PFX(__security_check_cookie)
ASM_PFX(__security_check_cookie):
    ret
