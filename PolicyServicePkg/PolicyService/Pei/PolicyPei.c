/** @file
  Implements the protocol PPI, providing services to publish and access general policies in the PEI
  environment.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PolicyPei.h"

STATIC POLICY_PPI  mPolicyPpi = {
  PeiSetPolicy,
  PeiGetPolicy,
  PeiRemovePolicy
};

STATIC EFI_PEI_PPI_DESCRIPTOR  PolicyPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiPolicyPpiGuid,
  &mPolicyPpi
};

/**
  Creates and emptry PPI for a given GUID to notify or dispatch consumers of
  this policy GUID. If the PPI already exists it will be reinstalled.

  @param[in]  PolicyGuid        The policy GUID used for the PPI.

  @retval     EFI_SUCCESS       The PPI was installed or reinstalled.
**/
EFI_STATUS
EFIAPI
PeiInstallPolicyIndicatorPpi (
  IN EFI_GUID  *PolicyGuid
  )
{
  EFI_STATUS              Status;
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor;
  EFI_PEI_PPI_DESCRIPTOR  *OldPpiDescriptor;
  EFI_GUID                *AllocatedGuid;
  VOID                    *ExistingPpi;

  Status = PeiServicesLocatePpi (PolicyGuid, 0, &OldPpiDescriptor, &ExistingPpi);
  if (EFI_ERROR (Status)) {
    OldPpiDescriptor = NULL;
  }

  Status = PeiServicesAllocatePool (sizeof (EFI_PEI_PPI_DESCRIPTOR) + sizeof (EFI_GUID), (VOID **)&PpiDescriptor);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AllocatedGuid        = (EFI_GUID *)(PpiDescriptor + 1);
  *AllocatedGuid       = *PolicyGuid;
  PpiDescriptor->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  PpiDescriptor->Guid  = AllocatedGuid;
  PpiDescriptor->Ppi   = NULL;
  if (OldPpiDescriptor == NULL) {
    Status = PeiServicesInstallPpi (PpiDescriptor);
  } else {
    Status = PeiServicesReInstallPpi (OldPpiDescriptor, PpiDescriptor);
  }

  return Status;
}

/**
  Retrieves the hob for a given policy GUID.

  @param[in]  PolicyGuid      The policy GUID to match.
  @param[out] PolicyHob       Pointer to the policy hob header found.

  @retval   EFI_SUCCESS       The policy entry was found.
  @retval   EFI_NOT_FOUND     The policy entry was not found.
**/
EFI_STATUS
EFIAPI
PeiGetPolicyHob (
  IN EFI_GUID            *PolicyGuid,
  OUT POLICY_HOB_HEADER  **PolicyHob
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  POLICY_HOB_HEADER  *CheckHob;

  GuidHob = GetFirstGuidHob (&gPolicyHobGuid);
  while (GuidHob != NULL) {
    CheckHob = (POLICY_HOB_HEADER *)GET_GUID_HOB_DATA (GuidHob);
    if (!CheckHob->Removed &&
        CompareGuid (PolicyGuid, &CheckHob->PolicyGuid))
    {
      *PolicyHob = CheckHob;
      return EFI_SUCCESS;
    }

    GuidHob = GetNextGuidHob (&gPolicyHobGuid, GET_NEXT_HOB (GuidHob));
  }

  return EFI_NOT_FOUND;
}

/**
  Checks if a given policy exists in the policy store.

  @param[in]  PolicyGuid      The policy GUID to match.

  @retval   TRUE              The policy exists in the store.
  @retval   FALSE             The policy does not exists in the store.
**/
BOOLEAN
EFIAPI
PeiCheckPolicyExists (
  IN EFI_GUID  *PolicyGuid
  )

{
  POLICY_HOB_HEADER  *PolicyHob;

  return !EFI_ERROR (PeiGetPolicyHob (PolicyGuid, &PolicyHob));
}

/**
  Allocates a policy HOB and initialized its header structure.

  @param[in]  PolicyGuid      The policy GUID the HOB is created for.
  @param[out] HobHeader       The pointer to the created HOBs header structure.

  @retval     EFI_SUCCESS           The policy HOB was successfully allocated.
  @retval     EFI_BAD_BUFFER_SIZE   The policy exceeds the maximum policy size.
  @retval     EFI_OUT_OF_RESOURCES  Failed to create the GUID HOB.
**/
EFI_STATUS
EFIAPI
PeiCreatePolicyHob (
  IN UINT16              PolicySize,
  OUT POLICY_HOB_HEADER  **HobHeader
  )
{
  POLICY_HOB_HEADER  *HobPolicy;
  UINT32             HobLength;

  HobLength = sizeof (POLICY_HOB_HEADER) + PolicySize;
  if (HobLength > MAX_UINT16) {
    DEBUG ((DEBUG_ERROR, "%a: Policy provided exceeds maximum HOB size! Required size: %lu\n", __FUNCTION__, HobLength));
    return EFI_BAD_BUFFER_SIZE;
  }

  HobPolicy = BuildGuidHob (&gPolicyHobGuid, HobLength);
  if (HobPolicy == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create policy hob!\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (HobPolicy, sizeof (POLICY_HOB_HEADER));
  HobPolicy->AllocationSize = PolicySize;
  *HobHeader                = HobPolicy;
  return EFI_SUCCESS;
}

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
  IN EFI_GUID    *PolicyGuid,
  OUT UINT64     *Attributes OPTIONAL,
  OUT VOID       *Policy,
  IN OUT UINT16  *PolicySize
  )
{
  POLICY_HOB_HEADER  *PolicyHob;
  EFI_STATUS         Status;

  if ((PolicyGuid == NULL) ||
      (PolicySize == NULL) ||
      ((Policy == NULL) && (*PolicySize != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = PeiGetPolicyHob (PolicyGuid, &PolicyHob);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Attributes != NULL) {
    *Attributes = PolicyHob->Attributes;
  }

  if (*PolicySize < PolicyHob->PolicySize) {
    *PolicySize = PolicyHob->PolicySize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *PolicySize = PolicyHob->PolicySize;
  CopyMem (Policy, GET_HOB_POLICY_DATA (PolicyHob), PolicyHob->PolicySize);
  return EFI_SUCCESS;
}

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
  IN EFI_GUID  *PolicyGuid,
  IN UINT64    Attributes,
  IN VOID      *Policy,
  IN UINT16    PolicySize
  )

{
  EFI_STATUS         Status;
  POLICY_HOB_HEADER  *PolicyHob;

  if ((PolicyGuid == NULL) || (Policy == NULL) || (PolicySize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PeiGetPolicyHob (PolicyGuid, &PolicyHob);
  if (!EFI_ERROR (Status)) {
    if (PolicyHob->Attributes & POLICY_ATTRIBUTE_FINALIZED) {
      return EFI_ACCESS_DENIED;
    }

    if (PolicySize <= PolicyHob->AllocationSize) {
      PolicyHob->PolicySize = PolicySize;
      PolicyHob->Attributes = Attributes;
      CopyMem (GET_HOB_POLICY_DATA (PolicyHob), Policy, PolicySize);
      Status = PeiInstallPolicyIndicatorPpi (PolicyGuid);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to install notify PPI. (%r)\n", __FUNCTION__, Status));
        return Status;
      }

      return EFI_SUCCESS;
    }

    // If the policy cannot fit in the existing buffer, mark it removed and
    // allocate a new one.
    PolicyHob->Removed = 1;
  }

  Status = PeiCreatePolicyHob (PolicySize, &PolicyHob);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create policy HOB. (%r)\n", __FUNCTION__, Status));
    return Status;
  }

  PolicyHob->PolicyGuid = *PolicyGuid;
  PolicyHob->PolicySize = PolicySize;
  PolicyHob->Attributes = Attributes;
  CopyMem (GET_HOB_POLICY_DATA (PolicyHob), Policy, PolicySize);

  Status = PeiInstallPolicyIndicatorPpi (PolicyGuid);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install notify PPI. (%r)\n", __FUNCTION__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

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
  IN EFI_GUID  *PolicyGuid
  )
{
  EFI_STATUS         Status;
  POLICY_HOB_HEADER  *PolicyHob;

  if (PolicyGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PeiGetPolicyHob (PolicyGuid, &PolicyHob);
  if (!EFI_ERROR (Status)) {
    PolicyHob->Removed = 1;
  }

  return Status;
}

/**
  Entry point for the policy PEIM. Installs the policy service PPI.

  @param[in]    FileHandle      UNUSED.
  @param[in]    PeiServices     UNUSED.

  @retval EFI_SUCCESS           The interface was successfully installed.
  @retval EFI_OUT_OF_RESOURCES  There is no additional space in the PPI database.
**/
EFI_STATUS
EFIAPI
PeiPolicyEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  // Install the Policy PPI
  return PeiServicesInstallPpi (&PolicyPpiList);
}
