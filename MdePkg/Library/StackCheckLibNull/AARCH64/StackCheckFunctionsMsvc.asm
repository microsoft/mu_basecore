;------------------------------------------------------------------------------
; AARCH64/StackCheckFunctionsMsvc.nasm
;
; Copyright (c) Microsoft Corporation.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;------------------------------------------------------------------------------

    EXPORT __report_rangecheckfailure
    EXPORT __GSHandlerCheck
    EXPORT __security_check_cookie
    EXPORT __security_push_cookie
    EXPORT __security_pop_cookie

    AREA |.text|, CODE, READONLY

;------------------------------------------------------------------------------
; Calls an interrupt using the vector specified by PcdStackCookieExceptionVector
;
; VOID
; TriggerStackCookieInterrupt (
;   VOID
;   );
;------------------------------------------------------------------------------
__report_rangecheckfailure PROC
    RET
__report_rangecheckfailure ENDP

;------------------------------------------------------------------------------
; Calls an interrupt using the vector specified by PcdStackCookieExceptionVector
;
; VOID
; __GSHandlerCheck (
;   VOID
;   );
;------------------------------------------------------------------------------
__GSHandlerCheck PROC
    RET
__GSHandlerCheck ENDP

;------------------------------------------------------------------------------
; Checks the stack cookie value against passed __security_cookie. Take
; appropiate action on a mismatch (stack smashed case)
;
; VOID
; EFIAPI
; __security_check_cookie (
;   IN UINTN CheckValue
;   );
;------------------------------------------------------------------------------
__security_check_cookie PROC
    RET
__security_check_cookie ENDP


;------------------------------------------------------------------------------
; Pop the cookie off the stack (recommended in 16 bytes)
;
; VOID
; __security_push_cookie (
;   VOID
;   );
;------------------------------------------------------------------------------
__security_pop_cookie PROC
    RET
__security_pop_cookie ENDP


;------------------------------------------------------------------------------
; Push a Security Cookie onto the stack (recommended is 16 bytes)
;
; VOID
; __security_push_cookie (
;   VOID
;   );
;------------------------------------------------------------------------------
__security_push_cookie PROC
    RET
__security_push_cookie ENDP

    END
