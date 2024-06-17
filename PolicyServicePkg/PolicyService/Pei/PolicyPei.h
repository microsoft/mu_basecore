/** @file
  Prototypes and type definitions for the PEI Policy service
  module.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _POLICY_PEI_H_
#define _POLICY_PEI_H_

#include <Uefi.h>
#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

#include <Ppi/Policy.h>
#include "../PolicyHob.h"

//
// Structures for tracking policy callbacks.
//

#define NOTIFY_ENTRIES_PER_HOB  (64)

typedef struct _POLICY_NOTIFY_ENTRY {
  EFI_GUID                   PolicyGuid;
  UINT32                     EventTypes;
  UINT32                     Priority;
  POLICY_HANDLER_CALLBACK    CallbackRoutine;
  BOOLEAN                    Tombstone;
} POLICY_NOTIFY_ENTRY;

typedef struct _PEI_POLICY_NOTIFY_HOB {
  UINT16                 Index;
  UINT16                 Count;
  POLICY_NOTIFY_ENTRY    Entries[NOTIFY_ENTRIES_PER_HOB];
} PEI_POLICY_NOTIFY_HOB;

//
// Pointers to HOBs should be avoided. For this reason, the PEI handle is
// actually just the index of the HOB and index in that HOB.
//

#define NOTIFY_HANDLE(_hobindex, _entryindex)  (VOID *)(UINTN)((_hobindex << 16) | _entryindex)
#define NOTIFY_HANDLE_HOB_INDEX(_handle)       (UINT16)((((UINTN)_handle) >> 16) & MAX_UINT16)
#define NOTIFY_HANDLE_ENTRY_INDEX(_handle)     (UINT16)((UINTN)_handle & MAX_UINT16)

/**
  Creates or updates a policy in the policy store. Will notify any applicable
  callbacks.

  @param[in]  PolicyGuid          The uniquely identifying GUID for the policy.
  @param[in]  Attributes          Attributes of the policy to be set.
  @param[in]  Policy              The policy data buffer. This buffer will be
                                  copied into the data store.
  @param[in]  PolicySize          The size of the provided policy data.

  @retval   EFI_SUCCESS           Policy was created or updated.
  @retval   EFI_ACCESS_DENIED     Policy was already finalized prior to this call.
  @retval   EFI_OUT_OF_RESOURCES  Failed to allocate space for policy structures.
**/
EFI_STATUS
EFIAPI
PeiSetPolicy (
  IN CONST EFI_GUID  *PolicyGuid,
  IN UINT64          Attributes,
  IN VOID            *Policy,
  IN UINT16          PolicySize
  );

/**
  Retrieves the policy descriptor, buffer, and size for a given policy GUID.

  @param[in]      PolicyGuid        The GUID of the policy being retrieved.
  @param[out]     Attributes        The attributes of the stored policy.
  @param[out]     Policy            The buffer where the policy data is copied.
  @param[in,out]  PolicySize        The size of the stored policy data buffer.
                                    On output, contains the size of the stored policy.

  @retval   EFI_SUCCESS           The policy was retrieved.
  @retval   EFI_BUFFER_TOO_SMALL  The provided buffer size was too small.
  @retval   EFI_NOT_FOUND         The policy does not exist.
**/
EFI_STATUS
EFIAPI
PeiGetPolicy (
  IN CONST EFI_GUID  *PolicyGuid,
  OUT UINT64         *Attributes OPTIONAL,
  OUT VOID           *Policy,
  IN OUT UINT16      *PolicySize
  );

/**
  Removes a policy from the policy store. The policy will be removed from the store
  and freed if possible.

  @param[in]  PolicyGuid        The GUID of the policy being retrieved.

  @retval   EFI_SUCCESS         The policy was removed.
  @retval   EFI_NOT_FOUND       The policy does not exist.
**/
EFI_STATUS
EFIAPI
PeiRemovePolicy (
  IN CONST EFI_GUID  *PolicyGuid
  );

/**
  Registers a callback for a policy event notification. The provided routine
  will be invoked when one of multiple of the provided event types for the specified
  guid occurs.

  @param[in]   PolicyGuid        The GUID of the policy the being watched.
  @param[in]   EventTypes        The events to notify the callback for.
  @param[in]   Priority          The priority of the callback where the lower values
                                 will be called first.
  @param[in]   CallbackRoutine   The function pointer of the callback to be invoked.
  @param[out]  Handle            Returns the handle to this callback entry.

  @retval   EFI_SUCCESS            The callback notification as successfully registered.
  @retval   EFI_INVALID_PARAMETER  EventTypes was 0 or Callback routine is invalid.
  @retval   Other                  The callback registration failed.
**/
EFI_STATUS
EFIAPI
PeiRegisterNotify (
  IN CONST EFI_GUID           *PolicyGuid,
  IN CONST UINT32             EventTypes,
  IN CONST UINT32             Priority,
  IN POLICY_HANDLER_CALLBACK  CallbackRoutine,
  OUT VOID                    **Handle
  );

/**
  Removes a registered notification callback.

  @param[in]   Handle     The handle for the registered callback.

  @retval   EFI_SUCCESS            The callback notification as successfully removed.
  @retval   EFI_INVALID_PARAMETER  The provided handle is invalid.
  @retval   EFI_NOT_FOUND          The provided handle could not be found.
**/
EFI_STATUS
EFIAPI
PeiUnregisterNotify (
  IN VOID  *Handle
  );

/**
  Notifies all registered callbacks of a policy event.

  @param[in]   EventTypes    The event that occurred.
  @param[in]   PolicyHob     The policy entry the event occurred for.
**/
VOID
EFIAPI
PeiPolicyNotify (
  IN CONST UINT32       EventTypes,
  IN POLICY_HOB_HEADER  *PolicyHob
  );

#endif
