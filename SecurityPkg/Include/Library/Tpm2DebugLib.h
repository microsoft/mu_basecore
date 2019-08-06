/** @file -- Tpm2DebugLib.h
This file contains helper functions to perform a detailed debugging of
TPM transactions as they go to and from the TPM device.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

MS_CHANGE

**/

#ifndef _TPM_2_DEBUG_LIB_H_
#define _TPM_2_DEBUG_LIB_H_

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
  );


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
  );

#endif // _TPM_2_DEBUG_LIB_H_
