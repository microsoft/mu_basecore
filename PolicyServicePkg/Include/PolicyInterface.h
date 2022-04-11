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

typedef struct _POLICY_INTERFACE {
  POLICY_SET_POLICY       SetPolicy;
  POLICY_GET_POLICY       GetPolicy;
  POLICY_REMOVE_POLICY    RemovePolicy;
} POLICY_INTERFACE;

#endif
