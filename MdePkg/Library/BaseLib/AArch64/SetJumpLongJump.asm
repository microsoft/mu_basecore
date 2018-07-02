;------------------------------------------------------------------------------
;
; Copyright (c) 2009-2013, ARM Ltd.  All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

  EXPORT SetJump
  EXPORT InternalLongJump
; MS_CHANGE: change area name to |.text| and add an ALIGN directive
  AREA |.text|, ALIGN=3, CODE, READONLY


;/**
;  Saves the current CPU context that can be restored with a call to LongJump() and returns 0.#
;
;  Saves the current CPU context in the buffer specified by JumpBuffer and returns 0.  The initial
;  call to SetJump() must always return 0.  Subsequent calls to LongJump() cause a non-zero
;  value to be returned by SetJump().
;
;  If JumpBuffer is NULL, then ASSERT().
;
;  @param  JumpBuffer    A pointer to CPU context buffer.
;
;**/
;
;UINTN
;EFIAPI
;SetJump (
;  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer  // X0
;  );
;
; MS_CHANGE: do not use the pre-processor macro as it doesn't work with VS ARM Assembler
SetJump
#ifndef MDEPKG_NDEBUG
        stp     x29, x30, [sp, #-32]!
        mov     x29, sp
        str     x0, [sp, #16]
        bl      InternalAssertJumpBuffer
        ldr     x0, [sp, #16]
        ldp     x29, x30, [sp], #32
#endif
        mov     x16, sp // use IP0 so save SP
        stp x19, x20, [x0, #0]
        stp x21, x22, [x0, #16]
        stp x23, x24, [x0, #32]
        stp x25, x26, [x0, #48]
        stp x27, x28, [x0, #64]
        stp x29, x30, [x0, #80]
        str x16, [x0, #96]
        stp  d8,  d9, [x0, #112]
        stp d10, d11, [x0, #128]
        stp d12, d13, [x0, #144]
        stp d14, d15, [x0, #160]
        mov     x0, #0
        ret

;/**
;  Restores the CPU context that was saved with SetJump().#
;
;  Restores the CPU context from the buffer specified by JumpBuffer.
;  This function never returns to the caller.
;  Instead it resumes execution based on the state of JumpBuffer.
;
;  @param  JumpBuffer    A pointer to CPU context buffer.
;  @param  Value         The value to return when the SetJump() context is restored.
;
;**/
;VOID
;EFIAPI
;InternalLongJump (
;  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer,  // X0
;  IN      UINTN                     Value         // X1
;  );
;
; MS_CHANGE: do not use the pre-processor macro as it doesn't work with VS ARM Assembler
InternalLongJump
        ldp x19, x20, [x0,  #0]
        ldp x21, x22, [x0, #16]
        ldp x23, x24, [x0, #32]
        ldp x25, x26, [x0, #48]
        ldp x27, x28, [x0, #64]
        ldp x29, x30, [x0, #80]
        ldr x16, [x0, #96]
        ldp  d8,  d9, [x0, #112]
        ldp d10, d11, [x0, #128]
        ldp d12, d13, [x0, #144]
        ldp d14, d15, [x0, #160]
        mov     sp, x16
        cmp     x1, #0
        mov     x0, #1
        beq     exit
        mov     x0, x1
exit
        // use br not ret, as ret is guaranteed to mispredict
        br      x30

ASM_FUNCTION_REMOVE_IF_UNREFERENCED

  END

