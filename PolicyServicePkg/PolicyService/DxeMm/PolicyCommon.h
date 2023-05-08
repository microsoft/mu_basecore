/** @file
  Common prototypes and type definitions for the DXE/MM Policy service
  modules.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _POLICY_COMMON_H_
#define _POLICY_COMMON_H_

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

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
} POLICY_ENTRY;

#define POLICY_ENTRY_SIGNATURE  SIGNATURE_32('p', 'o', 'l', 'c')
#define POLICY_ENTRY_FROM_LINK(a)  CR (a, POLICY_ENTRY, Link, POLICY_ENTRY_SIGNATURE)

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
SetPolicy (
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
GetPolicy (
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
RemovePolicy (
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

#endif
