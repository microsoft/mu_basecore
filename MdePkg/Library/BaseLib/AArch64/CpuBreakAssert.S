# @file CpuBreakAssert.S
# 
# CpuBreakAssert function for AArch64.
# 
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

.text
.p2align 2
GCC_ASM_EXPORT(CpuBreakAssert)

#/**
#  Generates a debugger assertion break on the CPU.
#
#  This does a special break into the debugger such that the debugger knows
#  that the code running has hit an assertion, not a generic breakpoint.
#
#**/
#VOID
#EFIAPI
#CpuBreakAssert (
#  VOID
#  );
#
ASM_PFX(CpuBreakAssert):
    brk   0xf001
    ret
