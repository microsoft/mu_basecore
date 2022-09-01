/** @file
  This protocol provides WaitForEvent functionality without
  requiring TPL_APPLICATION level.

Copyright (c) Microsoft Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
// MU_CHANGE entire file

#ifndef INTERNAL_EVENT_SERVICES_
#define INTERNAL_EVENT_SERVICES_

//
// Internal Event Services Guid Value
//
// *NOTE: Should the interface of INTERNAL_EVENT_SERVICES_PROTOCOL change to
//   include additional event services functions, a new value for
//   INTERNAL_EVENT_SERVICES_PROTOCOL_GUID will be generated.
//
#define INTERNAL_EVENT_SERVICES_PROTOCOL_GUID  {0x7ecd162a, 0xc664, 0x11ec, { 0x9d, 0x64, 0x02, 0x42, 0xac, 0x12, 0x00, 0x02 } \}

typedef struct _INTERNAL_EVENT_SERVICES_PROTOCOL INTERNAL_EVENT_SERVICES_PROTOCOL;

/**
  Stops execution until an event is signaled, with no TPL restrictions.

  @param  NumberOfEvents         The number of events in the UserEvents array
  @param  UserEvents             An array of EFI_EVENT
  @param  UserIndex              Pointer to the index of the event which
                                 satisfied the wait condition

  @retval EFI_SUCCESS            The event indicated by Index was signaled.
  @retval EFI_INVALID_PARAMETER  The event indicated by Index has a notification
                                 function or Event was not a valid type

**/
typedef
EFI_STATUS
(EFIAPI *WAIT_FOR_EVENT_INTERNAL)(
  IN UINTN      NumberOfEvents,
  IN EFI_EVENT  *UserEvents,
  OUT UINTN     *UserIndex
  );

struct _INTERNAL_EVENT_SERVICES_PROTOCOL {
  WAIT_FOR_EVENT_INTERNAL    WaitForEventInternal;
};

extern EFI_GUID  gInternalEventServicesProtocolGuid;

#endif
