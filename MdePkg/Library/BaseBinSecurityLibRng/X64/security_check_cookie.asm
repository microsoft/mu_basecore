;------------------------------------------------------------------------------
; security_check_cookie.asm
; MS_CHANGE_?
;------------------------------------------------------------------------------
    EXTRN   __security_cookie:QWORD
    .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; __security_check_cookie (
;   IN UINTN CheckValue);
;------------------------------------------------------------------------------
__security_check_cookie PROC PUBLIC
    cmp     rcx, __security_cookie
    jne     __security_check_cookie_Failure
    ret

__security_check_cookie_Failure:
    int     3
    ret
__security_check_cookie ENDP

    END
