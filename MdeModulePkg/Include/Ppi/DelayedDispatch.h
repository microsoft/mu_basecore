/**

  PPI that allows a Pei module to request a callback after a minumum
  delay.

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

#ifndef __EFI_DELAYED_DISPATCH_PPI_H__
#define __EFI_DELAYED_DISPATCH_PPI_H__

#define EFI_DELAYED_DISPATCH_PPI_GUID \
  { \
    0x869c711d, 0x649c, 0x44fe, { 0x8b, 0x9e, 0x2c, 0xbb, 0x29, 0x11, 0xc3, 0xe6} } \
  }

/**
  Delayed Dispatch function.  This routine is called sometime after the required
  delay.  Upon return, if NewDelay is 0, the function is unregistered.  If NewDelay
  is not zero, this routine will be called again after the new delay period.

  @param[in,out] Context         Pointer to Context. Can be updated by routine.
  @param[out]    NewDelay        The new delay in us.  Leave at 0 to unregister callback.

**/

typedef
VOID
(EFIAPI *EFI_DELAYED_DISPATCH_FUNCTION) (
  IN OUT UINT64 *Context,
     OUT UINT32 *NewDelay
  );

///
/// Forward declaration for the EFI_DELAYED_DISPATCH PPI.
///
typedef struct _EFI_DELAYED_DISPATCH_PPI  EFI_DELAYED_DISPATCH_PPI;

/**
  Register a callback to be called after a minimum delay has occurred.

  @param[in]     This            The protocol instance pointer.
  @param[in]     Function        The function to be called when the delay has expired.
  @param[in]     Context         64 bit Context to pass to delay function.
  @param[in]     Delay           Delay in microseconds (us)

  @retval EFI_SUCCESS            The operation succeeds.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Too many requests for delayed dispatch.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DELAYED_DISPATCH_REGISTER)(
  IN  EFI_DELAYED_DISPATCH_PPI      *This,
  IN  EFI_DELAYED_DISPATCH_FUNCTION  Function,
  IN  UINT64                     Context,
  OUT UINT32                     Delay
  );

///
/// This PPI contains a set of services to interact with the SD_MMC host controller.
///
struct _EFI_DELAYED_DISPATCH_PPI {
  EFI_DELAYED_DISPATCH_REGISTER      Register;
};

extern EFI_GUID gEfiDelayedDispatchPpiGuid;

#endif
