/** @file
  Macros to work around lack of Clang support for LDR register, =expr

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
// MU_CHANGE - Start - Fix Incude Guard Name

#ifndef ASM_MACRO_LIB_H_
#define ASM_MACRO_LIB_H_
// MU_CHANGE - End - Fix Incude Guard Name

// MU_CHANGE - Start - MSVC ARM64 change
// GCC doesn't follow the C++ spec and is using a '#' in macro definitions.
//
#define CATSTR2(x, y)  x##y
#define CATSTR(x, y)   CATSTR2(x,y)
#define NUM(x)         CATSTR(HASH,x)
#define HASH  #
// MU_CHANGE - End - MSVC ARM64 change

// CurrentEL : 0xC = EL3; 8 = EL2; 4 = EL1
// This only selects between EL1 and EL2, else we die.
// Provide the Macro with a safe temp xreg to use.
// MU_CHANGE - Start - MSVC ARM64 change
#if !defined (_MSC_VER)
// MU_CHANGE - End - MSVC ARM64 change
#define EL1_OR_EL2(SAFE_XREG)        \
        mrs    SAFE_XREG, CurrentEL ;\
        cmp    SAFE_XREG, #0x8      ;\
        b.gt   .                    ;\
        b.eq   2f                   ;\
        cbnz   SAFE_XREG, 1f        ;\
        b      .                    ;// We should never get here
// MU_CHANGE - Start - MSVC ARM64 change
#else
#define EL1_OR_EL2(SAFE_XREG)        \
        mrs    SAFE_XREG, CurrentEL __CR__\
        cmp    SAFE_XREG, NUM(0x8)  __CR__\
6
bgt    %b6                  __CR__ \
  beq    %f2                  __CR__ \
  cbnz   SAFE_XREG, NUM (0x4)  __CR__ \
  5                                   __CR__ \
  bne    %b5                       // We should never get here
#endif
// MU_CHANGE - End - MSVC ARM64 change

// MU_CHANGE - Start - MSVC ARM64 change
// CurrentEL : 0xC = EL3; 8 = EL2; 4 = EL1
// This only selects between EL1 and EL2 and EL3, else we die.
// Provide the Macro with a safe temp xreg to use.
#if !defined (_MSC_VER)    // MU_CHANGE - ARM64 VS change
#define EL1_OR_EL2_OR_EL3(SAFE_XREG) \
        mrs    SAFE_XREG, CurrentEL ;\
        cmp    SAFE_XREG, #0x8      ;\
        b.gt   3f                   ;\
        b.eq   2f                   ;\
        cbnz   SAFE_XREG, 1f        ;\
        b      .                    ;// We should never get here
#else
#define EL1_OR_EL2_OR_EL3(SAFE_XREG) \
        mrs    SAFE_XREG, CurrentEL  __CR__\
        cmp    SAFE_XREG, NUM(0x8)   __CR__\
                bgt    %f3                   __CR__\
        beq    %f2                   __CR__\
        cbnz   SAFE_XREG, %f1        __CR__\
5                                    __CR__\
        bne    %b5                // We should never get here
#endif
// MU_CHANGE - End - MSVC ARM64 change

// MU_CHANGE - Start - MSVC ARM64 change - Add Back Defines

#if defined (_MSC_VER)

#define LoadConstantToReg(Data, Reg) \
  ldr Reg, =Data

#elif defined (__clang__)

// load x0 with _Data
#define LoadConstant(_Data)              \
  ldr  x0, 1f                          ; \
  b    2f                              ; \
.align(8)                              ; \
1:                                       \
  .8byte (_Data)                       ; \
2:

// load _Reg with _Data
#define LoadConstantToReg(_Data, _Reg)    \
  ldr  _Reg, 1f                         ; \
  b    2f                               ; \
.align(8)                               ; \
1:                                        \
  .8byte (_Data)                        ; \
2:

#elif defined (__GNUC__)

#define LoadConstant(Data) \
  ldr  x0, =Data

#define LoadConstantToReg(Data, Reg) \
  ldr  Reg, =Data

#endif // _MSC_VER

// MU_CHANGE - Start - MSVC ARM64 change
#if !defined (_MSC_VER)
// MU_CHANGE - End - MSVC ARM64 change
// MU_CHANGE - Start - Clang Support
  #ifndef __clang__
// MU_CHANGE - End - Clang Support
#define _ASM_FUNC(Name, Section)    \
  .global   Name                  ; \
  .section  #Section, "ax"        ; \
  .type     Name, %function       ; \
  Name:                           ; \
  AARCH64_BTI(c)
// MU_CHANGE - Start - Clang Support
  #else
#define _ASM_FUNC(Name, Section)    \
  .global   Name                  ; \
  .section  #Section, "ax"        ; \
  Name:                           ; \
  AARCH64_BTI(c)
  #endif

  #ifndef __clang__ // MU_CHANGE
// MU_CHANGE - End - Clang Support
#define _ASM_FUNC_ALIGN(Name, Section, Align)       \
  .global   Name                                  ; \
  .section  #Section, "ax"                        ; \
  .type     Name, %function                       ; \
  .balign   Align                                 ; \
  Name:                                           ; \
  AARCH64_BTI(c)
// MU_CHANGE - Start - Clang Support
  #else
#define _ASM_FUNC_ALIGN(Name, Section, Align)       \
  .global   Name                                  ; \
  .section  #Section, "ax"                        ; \
  .balign   Align                                 ; \
  Name:                                           ; \
  AARCH64_BTI(c)
  #endif
// MU_CHANGE - End - Clang Support

#define ASM_FUNC(Name)  _ASM_FUNC(ASM_PFX(Name), .text. ## Name)

#define ASM_FUNC_ALIGN(Name, Align)  \
  _ASM_FUNC_ALIGN(ASM_PFX(Name), .text. ## Name, Align)

#define MOV32(Reg, Val)                   \
  movz      Reg, (Val) >> 16, lsl #16   ; \
  movk      Reg, (Val) & 0xffff

#define MOV64(Reg, Val)                             \
  movz      Reg, (Val) >> 48, lsl #48             ; \
  movk      Reg, ((Val) >> 32) & 0xffff, lsl #32  ; \
  movk      Reg, ((Val) >> 16) & 0xffff, lsl #16  ; \
  movk      Reg, (Val) & 0xffff

// MU_CHANGE - Start - MSVC ARM64 change
#endif
// MU_CHANGE - End - MSVC ARM64 change
#endif // ASM_MACRO_LIB_H_
