/** @file
  Implements the DXE policy protocol, providing services to publish and access
  system policy.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PolicyDxe.h"

STATIC LIST_ENTRY  mPolicyListHead = INITIALIZE_LIST_HEAD_VARIABLE (mPolicyListHead);
STATIC EFI_LOCK    mPolicyListLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);
STATIC EFI_HANDLE  mImageHandle    = NULL;

POLICY_PROTOCOL  mPolicyProtocol = {
  DxeSetPolicy,
  DxeGetPolicy,
  DxeRemovePolicy
};

/**
  Creates and emptry protocol for a given GUID to notify or dispatch consumers of
  this policy GUID. If the protocol already exists it will be reinstalled.

  @param[in]  PolicyGuid        The policy GUID used for the protocol.

  @retval     EFI_SUCCESS       The protocol was installed or reinstalled.
**/
EFI_STATUS
EFIAPI
DxeInstallPolicyIndicatorProtocol (
  IN EFI_GUID  *PolicyGuid
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;

  Status = gBS->LocateProtocol (PolicyGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mImageHandle,
                    PolicyGuid,
                    NULL,
                    NULL
                    );
  } else {
    Status = gBS->ReinstallProtocolInterface (
                    mImageHandle,
                    PolicyGuid,
                    NULL,
                    NULL
                    );
  }

  return Status;
}

/**
  Retrieves the policy descriptor, buffer, and size for a given policy GUID.
  Assumes the caller is already in CS.

  @param[in]      PolicyGuid        The GUID of the policy being retrieved.
  @param[out]     PolicyDescriptor  Descriptor for the stored policy.
  @param[out]     Policy            The buffer where the policy data is copied.
  @param[in,out]  PolicySize        The size of the stored policy data buffer.
                                    On output, contains the size of the stored policy.

  @retval   EFI_SUCCESS           The policy was retrieved.
  @retval   EFI_BUFFER_TOO_SMALL  The provided buffer size was too small.
  @retval   EFI_NOT_FOUND         The policy does not exist.
**/
EFI_STATUS
EFIAPI
DxeGetPolicyEntry (
  IN EFI_GUID       *PolicyGuid,
  OUT POLICY_ENTRY  **PolicyEntry
  )
{
  LIST_ENTRY    *Link;
  POLICY_ENTRY  *CheckEntry;

  BASE_LIST_FOR_EACH (Link, &mPolicyListHead) {
    CheckEntry = POLICY_ENTRY_FROM_LINK (Link);
    if (CompareGuid (PolicyGuid, &CheckEntry->PolicyGuid)) {
      *PolicyEntry = CheckEntry;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Checks if a given policy exists in the policy store. Assumes the caller
  is already in CS.

  @param[in]  PolicyGuid      The policy GUID to match.

  @retval   TRUE              The policy exists in the store.
  @retval   FALSE             The policy does not exists in the store.
**/
BOOLEAN
EFIAPI
DxeCheckPolicyExists (
  IN EFI_GUID  *PolicyGuid
  )
{
  POLICY_ENTRY  *Entry;

  return !EFI_ERROR (DxeGetPolicyEntry (PolicyGuid, &Entry));
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
DxeGetPolicy (
  IN EFI_GUID    *PolicyGuid,
  OUT UINT64     *Attributes OPTIONAL,
  OUT VOID       *Policy,
  IN OUT UINT16  *PolicySize
  )
{
  EFI_STATUS    Status;
  POLICY_ENTRY  *Entry;

  if ((PolicyGuid == NULL) ||
      (PolicySize == NULL) ||
      ((Policy == NULL) && (*PolicySize != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&mPolicyListLock);
  Status = DxeGetPolicyEntry (PolicyGuid, &Entry);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (Attributes != NULL) {
    *Attributes = Entry->Attributes;
  }

  if (*PolicySize < Entry->PolicySize) {
    *PolicySize = Entry->PolicySize;
    Status      = EFI_BUFFER_TOO_SMALL;
    goto Exit;
  }

  CopyMem (Policy, Entry->Policy, Entry->PolicySize);
  *PolicySize = Entry->PolicySize;

Exit:
  EfiReleaseLock (&mPolicyListLock);
  return Status;
}

/**
  Parses the HOB list to find active policies to add to the policy store.

  @retval   EFI_SUCCESS           Policies added to the policy store.
  @retval   EFI_OUT_OF_RESOURCES  Failed to allocate memory for policy.
**/
EFI_STATUS
EFIAPI
IngestPoliciesFromHob (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  POLICY_HOB_HEADER  *PolicyHob;
  POLICY_ENTRY       *PolicyEntry;
  UINT32             PolicyCount;
  EFI_STATUS         Status;

  PolicyCount = 0;
  Status      = EFI_SUCCESS;
  EfiAcquireLock (&mPolicyListLock);
  for (GuidHob = GetFirstGuidHob (&gPolicyHobGuid);
       GuidHob != NULL;
       GuidHob = GetNextGuidHob (&gPolicyHobGuid, GET_NEXT_HOB (GuidHob)))
  {
    PolicyHob = (POLICY_HOB_HEADER *)GET_GUID_HOB_DATA (GuidHob);
    if (PolicyHob->Removed) {
      continue;
    }

    if (PolicyHob->Attributes & POLICY_ATTRIBUTE_PEI_ONLY) {
      continue;
    }

    ASSERT (!DxeCheckPolicyExists (&PolicyHob->PolicyGuid));

    PolicyEntry = AllocatePool (sizeof (POLICY_ENTRY));
    if (PolicyEntry == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    ZeroMem (PolicyEntry, sizeof (POLICY_ENTRY));
    PolicyEntry->Signature      = POLICY_ENTRY_SIGNATURE;
    PolicyEntry->PolicyGuid     = PolicyHob->PolicyGuid;
    PolicyEntry->FromHob        = TRUE;
    PolicyEntry->Attributes     = PolicyHob->Attributes;
    PolicyEntry->Policy         = GET_HOB_POLICY_DATA (PolicyHob);
    PolicyEntry->PolicySize     = PolicyHob->PolicySize;
    PolicyEntry->AllocationSize = 0;
    InsertTailList (&mPolicyListHead, &PolicyEntry->Link);
    PolicyCount++;

    Status = DxeInstallPolicyIndicatorProtocol (&PolicyEntry->PolicyGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to install notification protocol. (%r)\n", __FUNCTION__, Status));
    }
  }

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Found %d active policies in HOBs.\n", PolicyCount));
  }

  EfiReleaseLock (&mPolicyListLock);
  return Status;
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
DxeRemovePolicy (
  IN EFI_GUID  *PolicyGuid
  )
{
  EFI_STATUS    Status;
  POLICY_ENTRY  *Entry;

  if (PolicyGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&mPolicyListLock);
  Status = DxeGetPolicyEntry (PolicyGuid, &Entry);
  if (!EFI_ERROR (Status)) {
    RemoveEntryList (&Entry->Link);
    if (!Entry->FromHob) {
      FreePool (Entry->Policy);
    }

    FreePool (Entry);
  }

  EfiReleaseLock (&mPolicyListLock);
  return Status;
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
DxeSetPolicy (
  IN EFI_GUID  *PolicyGuid,
  IN UINT64    Attributes,
  IN VOID      *Policy,
  IN UINT16    PolicySize
  )
{
  EFI_STATUS    Status;
  POLICY_ENTRY  *Entry;
  VOID          *AllocatedPolicy;

  if ((PolicyGuid == NULL) || (Policy == NULL) || (PolicySize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&mPolicyListLock);
  Status = DxeGetPolicyEntry (PolicyGuid, &Entry);
  if (!EFI_ERROR (Status)) {
    if (Entry->Attributes & POLICY_ATTRIBUTE_FINALIZED) {
      Status = EFI_ACCESS_DENIED;
      goto Exit;
    }

    // Re-use the buffer if possible
    if (!Entry->FromHob && (PolicySize <= Entry->AllocationSize)) {
      CopyMem (Entry->Policy, Policy, PolicySize);
      Entry->PolicySize = PolicySize;
      Entry->Attributes = Attributes;
    } else {
      AllocatedPolicy = AllocatePool (PolicySize);
      if (AllocatedPolicy == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      CopyMem (AllocatedPolicy, Policy, PolicySize);
      if (!Entry->FromHob) {
        FreePool (Entry->Policy);
      }

      Entry->Policy         = AllocatedPolicy;
      Entry->Attributes     = Attributes;
      Entry->PolicySize     = PolicySize;
      Entry->AllocationSize = PolicySize;
      Entry->FromHob        = FALSE;
    }
  } else {
    Entry = AllocatePool (sizeof (POLICY_ENTRY));
    if (Entry == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    AllocatedPolicy = AllocatePool (PolicySize);
    if (AllocatedPolicy == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    ZeroMem (Entry, sizeof (POLICY_ENTRY));
    Entry->Signature      = POLICY_ENTRY_SIGNATURE;
    Entry->PolicyGuid     = *PolicyGuid;
    Entry->Attributes     = Attributes;
    Entry->Policy         = AllocatedPolicy;
    Entry->PolicySize     = PolicySize;
    Entry->AllocationSize = PolicySize;
    CopyMem (Entry->Policy, Policy, PolicySize);
    InsertTailList (&mPolicyListHead, &Entry->Link);
  }

  Status = DxeInstallPolicyIndicatorProtocol (PolicyGuid);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install notification protocol. (%r)\n", __FUNCTION__, Status));
    goto Exit;
  }

Exit:
  EfiReleaseLock (&mPolicyListLock);
  return Status;
}

/**
  DXE policy driver entry point. Initialized the policy store from the HOB list
  and install the DXE policy protocol.

  @param[in]  ImageHandle     The firmware allocated handle for the EFI image.
  @param[in]  SystemTable     UNUSED.

  @retval   EFI_SUCCESS           Policy store initialized and protocol installed.
  @retval   EFI_OUT_OF_RESOURCES  Failed to allocate memory for policy and global structures.
**/
EFI_STATUS
EFIAPI
DxePolicyEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  // Process the HOBs to consume any existing policies.
  Status = IngestPoliciesFromHob ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to ingest HOB policies. (%r)\n", __FUNCTION__, Status));
    return Status;
  }

  mImageHandle = ImageHandle;
  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gPolicyProtocolGuid,
                &mPolicyProtocol,
                NULL
                );
}
