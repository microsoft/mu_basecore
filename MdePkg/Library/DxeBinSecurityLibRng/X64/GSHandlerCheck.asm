;------------------------------------------------------------------------------
; GSHandlerCheck.asm
; MS_CHANGE_?
;------------------------------------------------------------------------------
    .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; __GSHandlerCheck (VOID);
;------------------------------------------------------------------------------
__GSHandlerCheck PROC PUBLIC
    ret ; this GS handler is for checking the stack cookie during SEH or
        ; EH exceptions since UEFI doesn't support these we can just do
        ; nothing here
__GSHandlerCheck ENDP

    END