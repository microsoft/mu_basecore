/** @file
  Common prototypes and type definitions for the DXE/MM Policy service
  modules.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _POLICY_COMMON_H_
#define _POLICY_COMMON_H_

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <PolicyInterface.h>
#include "../PolicyHob.h"

typedef struct _POLICY_ENTRY {
  UINT32        Signature;
  LIST_ENTRY    Link;
  EFI_GUID      PolicyGuid;
  UINT64        Attributes;
  BOOLEAN       FromHob;
  VOID          *Policy;
  UINT16        PolicySize;
  UINTN         AllocationSize;
  UINT32        NotifyDepth;
  BOOLEAN       FreeAfterNotify;
} POLICY_ENTRY;

#define POLICY_ENTRY_SIGNATURE  SIGNATURE_32('p', 'o', 'l', 'c')
#define POLICY_ENTRY_FROM_LINK(a)  CR (a, POLICY_ENTRY, Link, POLICY_ENTRY_SIGNATURE)

typedef struct _POLICY_NOTIFY_ENTRY {
  UINT32                     Signature;
  LIST_ENTRY                 Link;
  EFI_GUID                   PolicyGuid;
  UINT32                     EventTypes;
  UINT32                     Priority;
  POLICY_HANDLER_CALLBACK    CallbackRoutine;
  BOOLEAN                    Tombstone;
} POLICY_NOTIFY_ENTRY;

#define POLICY_NOTIFY_ENTRY_SIGNATURE  SIGNATURE_32('p', 'o', 'l', 'n')
#define POLICY_NOTIFY_ENTRY_FROM_LINK(a)  CR (a, POLICY_NOTIFY_ENTRY, Link, POLICY_NOTIFY_ENTRY_SIGNATURE)

//
// Macros for managing the critical section.
//

#define POLICY_CS_INIT   EFI_TPL OldTpl
#define POLICY_CS_ENTER  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL)
#define POLICY_CS_EXIT   gBS->RestoreTPL (OldTpl)

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
CommonSetPolicy (
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
CommonGetPolicy (
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
CommonRemovePolicy (
  IN CONST EFI_GUID  *PolicyGuid
  );

/**
  Acquires the environment specific lock for the policy list.

**/
VOID
EFIAPI
PolicyLockAcquire (
  VOID
  );

/**
  Release the environment specific lock for the policy list.

**/
VOID
EFIAPI
PolicyLockRelease (
  VOID
  );

/**
  Creates and empty protocol for a given GUID to notify or dispatch consumers of
  this policy GUID. If the protocol already exists it will be reinstalled.

  @param[in]  PolicyGuid        The policy GUID used for the protocol.

  @retval     EFI_SUCCESS       The protocol was installed or reinstalled.
**/
EFI_STATUS
EFIAPI
InstallPolicyIndicatorProtocol (
  IN CONST EFI_GUID  *PolicyGuid
  );

/**
  Parses the HOB list to find active policies to add to the policy store.

  @retval   EFI_SUCCESS           Policies added to the policy store.
  @retval   EFI_OUT_OF_RESOURCES  Failed to allocate memory for policy.
**/
EFI_STATUS
EFIAPI
IngestPoliciesFromHob (
  VOID
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
CommonRegisterNotify (
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
CommonUnregisterNotify (
  IN VOID  *Handle
  );

/**
  Notifies all registered callbacks of a policy event.

  @param[in]   EventTypes    The event that occurred.
  @param[in]   PolicyEntry   The policy entry the event occurred for.
**/
VOID
EFIAPI
CommonPolicyNotify (
  IN CONST UINT32  EventTypes,
  IN POLICY_ENTRY  *PolicyEntry
  );

#endif
