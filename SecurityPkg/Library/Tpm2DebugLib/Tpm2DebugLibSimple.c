/** @file -- Tpm2TransportSimple.c
This file contains helper functions to perform a simple debugging of
TPM transactions as they go to and from the TPM device.

MS_CHANGE

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <Library/DebugLib.h>


/**
  This function dumps as much information as possible about
  a command being sent to the TPM for maximum user-readability.

  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.

**/
VOID
EFIAPI
DumpTpmInputBlock (
  IN UINT32         InputBlockSize,
  IN CONST UINT8    *InputBlock
  )
{
  UINTN  Index, DebugSize;

  DEBUG ((DEBUG_INFO, "TpmCommand Send - "));

  if (InputBlockSize > 0x100) {
    DebugSize = 0x40;
  } else {
    DebugSize = InputBlockSize;
  }

  for (Index = 0; Index < DebugSize; Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", InputBlock[Index]));
  }

  if (DebugSize != InputBlockSize) {
    DEBUG ((DEBUG_INFO, "...... "));

    for (Index = InputBlockSize - 0x20; Index < InputBlockSize; Index++) {
      DEBUG ((DEBUG_INFO, "%02x ", InputBlock[Index]));
    }
  }

  DEBUG ((DEBUG_INFO, "\n"));

  return;
} // DumpTpmInputBlock()


/**
  This function dumps as much information as possible about
  a response from the TPM for maximum user-readability.

  @param[in]  OutputBlockSize  Size of the output buffer.
  @param[in]  OutputBlock      Pointer to the output buffer itself.

**/
VOID
EFIAPI
DumpTpmOutputBlock (
  IN UINT32         OutputBlockSize,
  IN CONST UINT8    *OutputBlock
  )
{
  UINTN   Index;

  DEBUG ((DEBUG_INFO, "TpmCommand Receive - "));

  for (Index = 0; Index < OutputBlockSize; Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", OutputBlock[Index]));
  }

  DEBUG ((DEBUG_INFO, "\n"));

  return;
} // DumpTpmOutputBlock()
