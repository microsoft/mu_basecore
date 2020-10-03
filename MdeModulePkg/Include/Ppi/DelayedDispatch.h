/**

  PPI that allows a Pei module to request a callback after a minumum
  delay.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_DELAYED_DISPATCH_PPI_H__
#define __EFI_DELAYED_DISPATCH_PPI_H__

#define EFI_DELAYED_DISPATCH_PPI_GUID \
  { \
    0x869c711d, 0x649c, 0x44fe, { 0x8b, 0x9e, 0x2c, 0xbb, 0x29, 0x11, 0xc3, 0xe6} } \
  }

#define GET_TIME_IN_US()  ((UINT32)DivU64x32(GetTimeInNanoSecond(GetPerformanceCounter ()), 1000))
#define UNREGISTER_DISPATCH  (0)
#define DELAY_DISPATCH_IN_MS(t)  ((t) * 1000)

/**
  Delayed Dispatch function.  This routine is called sometime after the required
  delay.  At entry, NewDelay will be 0. Upon return, if NewDelay is still 0, the function
  is unregistered.  If NewDelay is not zero, this routine will be called again after the
  new delay period.

  @param[in,out] Context         Pointer to Context. Can be updated by routine.
  @param[out]    NewDelay        The new delay in us.  Leave at 0 to unregister callback.

**/

typedef
VOID
(EFIAPI *EFI_DELAYED_DISPATCH_FUNCTION)(
  IN OUT UINT64 *Context,
  OUT UINT32 *NewDelay
  );

///
/// Forward declaration for the EFI_DELAYED_DISPATCH PPI.
///
typedef struct _EFI_DELAYED_DISPATCH_PPI EFI_DELAYED_DISPATCH_PPI;

/**
  Register a callback to be called after a minimum delay has occurred.

  @param[in]     This            The Delayed Dispatch PPI pointer.
  @param[in]     Function        The function to be called when the delay has expired.
  @param[in]     Context         64 bit Context to pass to delay function.
  @param[in]     UniqueId        GUID for this Delayed Dispatch request.
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
  IN  UINT64                         Context,
  IN  EFI_GUID                      *UniqueId   OPTIONAL,
  IN  UINT32                         Delay
  );

/**
  Wait on a registered Delayed Dispatch unit that has a UniqueId.  Continue
  to dispatch all registered delayed dispatch entries until *ALL* entries with
  UniqueId have completed.

  @param[in]     This            The Delayed Dispatch PPI pointer.
  @param[in]     UniqueId        UniqueId of delayed dispatch entry.

  @retval EFI_SUCCESS            The operation succeeds.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DELAYED_DISPATCH_WAIT_ON_EVENT)(
  IN  EFI_DELAYED_DISPATCH_PPI      *This,
  IN  EFI_GUID                      *UniqueId
  );

///
/// This PPI contains a set of services to delay dispatch of functionality.
///
struct _EFI_DELAYED_DISPATCH_PPI {
  EFI_DELAYED_DISPATCH_REGISTER         Register;
  EFI_DELAYED_DISPATCH_WAIT_ON_EVENT    WaitOnEvent;
};

extern EFI_GUID  gEfiDelayedDispatchPpiGuid;

#endif
