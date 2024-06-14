//------------------------------------------------------------------------------
// Arm/StackCookieInterrupt.S
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//------------------------------------------------------------------------------

    .text

//------------------------------------------------------------------------------
// Calls an interrupt using the vector specified by PcdStackCookieExceptionVector
//
// VOID
// TriggerStackCookieInterrupt (
//   VOID
//   );
//------------------------------------------------------------------------------
.global ASM_PFX(TriggerStackCookieInterrupt)
ASM_PFX(TriggerStackCookieInterrupt):
    swi FixedPcdGet8 (PcdStackCookieExceptionVector)
    bx lr
