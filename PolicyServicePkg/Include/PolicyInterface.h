/** @file
  Common public header definitions for the policy interface.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _POLICY_INTERFACE_H_
#define _POLICY_INTERFACE_H_

// Flag indicating the policy is not mutable.
#define POLICY_ATTRIBUTE_FINALIZED  BIT0

// Indicating the provided policy should not be available in DXE.
#define POLICY_ATTRIBUTE_PEI_ONLY  BIT1

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
typedef
EFI_STATUS
(EFIAPI *POLICY_SET_POLICY)(
  IN CONST EFI_GUID *PolicyGuid,
  IN UINT64 Attributes,
  IN VOID *Policy,
  IN UINT16 PolicySize
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
typedef
EFI_STATUS
(EFIAPI *POLICY_GET_POLICY)(
  IN CONST EFI_GUID *PolicyGuid,
  OUT UINT64 *Attributes OPTIONAL,
  OUT VOID *Policy,
  IN OUT UINT16 *PolicySize
  );

/**
  Removes a policy from the policy store. The policy will be removed from the store
  and freed if possible.

  @param[in]  PolicyGuid        The GUID of the policy being retrieved.

  @retval   EFI_SUCCESS         The policy was removed.
  @retval   EFI_NOT_FOUND       The policy does not exist.
**/
typedef
EFI_STATUS
(EFIAPI *POLICY_REMOVE_POLICY)(
  IN CONST EFI_GUID *PolicyGuid
  );

/**
  Callback for a policy notification event.

  @param[in]  PolicyGuid        The GUID of the policy being notified.
  @param[in]  EventTypes        The events that occurred for the notification.
  @param[in]  CallbackHandle    The handle for the callback being invoked.
**/
typedef
VOID
(EFIAPI *POLICY_HANDLER_CALLBACK)(
  IN CONST EFI_GUID *PolicyGuid,
  IN UINT32 EventTypes,
  IN VOID *CallbackHandle
  );

/**
  Flags used for policy callbacks.

  POLICY_NOTIFY_SET - The policy content and/or attributes were set.
  POLICY_NOTIFY_FINALIZED - The policy was set with the POLICY_ATTRIBUTE_FINALIZED set.
  POLICY_NOTIFY_REMOVED - The policy was removed.

**/
#define POLICY_NOTIFY_SET        (BIT0)
#define POLICY_NOTIFY_FINALIZED  (BIT1)
#define POLICY_NOTIFY_REMOVED    (BIT2)
#define POLICY_NOTIFY_ALL        (POLICY_NOTIFY_SET | \
                                  POLICY_NOTIFY_FINALIZED | \
                                  POLICY_NOTIFY_REMOVED)

#define POLICY_NOTIFY_DEFAULT_PRIORITY  (512)

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
typedef
EFI_STATUS
(EFIAPI *POLICY_REGISTER_CALLBACK)(
  IN CONST EFI_GUID *PolicyGuid,
  IN CONST UINT32 EventTypes,
  IN CONST UINT32 Priority,
  IN POLICY_HANDLER_CALLBACK CallbackRoutine,
  OUT VOID **Handle
  );

/**
  Removes a registered notification callback.

  @param[in]   Handle     The handle for the registered callback.

  @retval   EFI_SUCCESS            The callback notification as successfully removed.
  @retval   EFI_INVALID_PARAMETER  The provided handle is invalid.
  @retval   EFI_NOT_FOUND          The provided handle could not be found.
**/
typedef
EFI_STATUS
(EFIAPI *POLICY_UNREGISTER_CALLBACK)(
  IN VOID *Handle
  );

typedef struct _POLICY_INTERFACE {
  POLICY_SET_POLICY             SetPolicy;
  POLICY_GET_POLICY             GetPolicy;
  POLICY_REMOVE_POLICY          RemovePolicy;
  POLICY_REGISTER_CALLBACK      RegisterNotify;
  POLICY_UNREGISTER_CALLBACK    UnregisterNotify;
} POLICY_INTERFACE;

#endif
