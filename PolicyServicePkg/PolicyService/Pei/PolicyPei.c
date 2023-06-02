/** @file
  Implements the protocol PPI, providing services to publish and access general policies in the PEI
  environment.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PolicyPei.h"

EFI_GUID  gPeiPolicyNotifyListPpiGuid = {
  0x3f22d2a0, 0xb8f5, 0x47e4, { 0xb2, 0x84, 0x76, 0x5d, 0x2d, 0xbc, 0x40, 0xaf }
};

STATIC POLICY_PPI  mPolicyPpi = {
  PeiSetPolicy,
  PeiGetPolicy,
  PeiRemovePolicy,
  PeiRegisterNotify,
  PeiUnregisterNotify
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
  IN CONST EFI_GUID  *PolicyGuid
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
  IN CONST EFI_GUID      *PolicyGuid,
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
  IN CONST EFI_GUID  *PolicyGuid
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
  IN CONST EFI_GUID  *PolicyGuid,
  OUT UINT64         *Attributes OPTIONAL,
  OUT VOID           *Policy,
  IN OUT UINT16      *PolicySize
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
  IN CONST EFI_GUID  *PolicyGuid,
  IN UINT64          Attributes,
  IN VOID            *Policy,
  IN UINT16          PolicySize
  )

{
  EFI_STATUS         Status;
  POLICY_HOB_HEADER  *PolicyHob;
  UINT32             Events;

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
      Events = POLICY_NOTIFY_SET | (Attributes & POLICY_ATTRIBUTE_FINALIZED ? POLICY_NOTIFY_FINALIZED : 0);
      PeiPolicyNotify (Events, PolicyHob);
      if (Attributes & POLICY_ATTRIBUTE_FINALIZED ) {
        Status = PeiInstallPolicyIndicatorPpi (PolicyGuid);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Failed to install notify PPI. (%r)\n", __FUNCTION__, Status));
          return Status;
        }
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

  Events = POLICY_NOTIFY_SET | (Attributes & POLICY_ATTRIBUTE_FINALIZED ? POLICY_NOTIFY_FINALIZED : 0);
  PeiPolicyNotify (Events, PolicyHob);

  if (Attributes & POLICY_ATTRIBUTE_FINALIZED ) {
    Status = PeiInstallPolicyIndicatorPpi (PolicyGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to install notify PPI. (%r)\n", __FUNCTION__, Status));
      return Status;
    }
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
  IN CONST EFI_GUID  *PolicyGuid
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

  PeiPolicyNotify (POLICY_NOTIFY_REMOVED, PolicyHob);
  return Status;
}

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
  )
{
  EFI_HOB_GUID_TYPE      *Hob;
  PEI_POLICY_NOTIFY_HOB  *NotifyList;
  POLICY_NOTIFY_ENTRY    *Entry;
  UINT16                 Index;

  if ((CallbackRoutine == NULL) ||
      ((EventTypes & POLICY_NOTIFY_ALL) != EventTypes))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // All notifications are tracked in HOBs.
  //

  Index = 0;
  Hob   = GetFirstGuidHob (&gPolicyCallbackHobGuid);
  while (Hob != NULL) {
    NotifyList = GET_GUID_HOB_DATA (Hob);
    if (NotifyList->Count < NOTIFY_ENTRIES_PER_HOB) {
      break;
    }

    Index++;
    Hob = GetNextGuidHob (&gPolicyCallbackHobGuid, GET_NEXT_HOB (Hob));
  }

  if (Hob == NULL) {
    NotifyList = (PEI_POLICY_NOTIFY_HOB *)BuildGuidHob (&gPolicyCallbackHobGuid, sizeof (PEI_POLICY_NOTIFY_HOB));
    if (NotifyList == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to allocate notification HOB.\n", __FUNCTION__));
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (NotifyList, sizeof (PEI_POLICY_NOTIFY_HOB));
    NotifyList->Index = Index;
  }

  Entry                  = &NotifyList->Entries[NotifyList->Count];
  Entry->PolicyGuid      = *PolicyGuid;
  Entry->EventTypes      = EventTypes;
  Entry->Priority        = Priority;
  Entry->CallbackRoutine = CallbackRoutine;
  *Handle                = NOTIFY_HANDLE (NotifyList->Index, NotifyList->Count);
  NotifyList->Count++;
  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS             Status;
  EFI_HOB_GUID_TYPE      *Hob;
  PEI_POLICY_NOTIFY_HOB  *NotifyList;
  UINT16                 HobIndex;
  UINT16                 EntryIndex;

  HobIndex   = NOTIFY_HANDLE_HOB_INDEX (Handle);
  EntryIndex = NOTIFY_HANDLE_ENTRY_INDEX (Handle);

  Status = EFI_NOT_FOUND;
  Hob    = GetFirstGuidHob (&gPolicyCallbackHobGuid);
  while (Hob != NULL) {
    NotifyList = GET_GUID_HOB_DATA (Hob);
    if (NotifyList->Index == HobIndex) {
      ASSERT (NotifyList->Count > EntryIndex);
      ZeroMem (&NotifyList->Entries[EntryIndex], sizeof (POLICY_NOTIFY_ENTRY));
      NotifyList->Entries[EntryIndex].Tombstone = TRUE;
      Status                                    = EFI_SUCCESS;
      break;
    }

    Hob = GetNextGuidHob (&gPolicyCallbackHobGuid, GET_NEXT_HOB (Hob));
  }

  return Status;
}

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
  )
{
  UINT16                 Depth;
  EFI_HOB_GUID_TYPE      *Hob;
  PEI_POLICY_NOTIFY_HOB  *NotifyList;
  UINT16                 Index;
  UINT32                 LowestPriority;
  UINT32                 MinPriority;
  BOOLEAN                NotifiesFound;

  PolicyHob->NotifyDepth++;
  Depth = PolicyHob->NotifyDepth;

  //
  // This iteration method is very inefficient. The reasoning at the time of implementation
  // is that PEI phase callbacks should be limited in quantity and so this inefficiency
  // is worth it to avoid the complexity of keeping the HOB lists sorted. This iteration
  // works in 2 passes. The first to find the next lowest priority and the second
  // to invoke all notifications of that priority.
  //

  MinPriority = 0;
  while (TRUE) {
    //
    // First pass, find the next lowest priority.
    //

    NotifiesFound  = FALSE;
    LowestPriority = MAX_UINT32;
    Hob            = GetFirstGuidHob (&gPolicyCallbackHobGuid);
    while (Hob != NULL) {
      NotifyList = GET_GUID_HOB_DATA (Hob);
      for (Index = 0; Index < NotifyList->Count; Index++) {
        if (!NotifyList->Entries[Index].Tombstone &&
            CompareGuid (&NotifyList->Entries[Index].PolicyGuid, &PolicyHob->PolicyGuid) &&
            ((NotifyList->Entries[Index].EventTypes & EventTypes) != 0) &&
            (NotifyList->Entries[Index].Priority >= MinPriority))
        {
          NotifiesFound = TRUE;
          if (NotifyList->Entries[Index].Priority < LowestPriority) {
            LowestPriority = NotifyList->Entries[Index].Priority;
          }
        }
      }

      Hob = GetNextGuidHob (&gPolicyCallbackHobGuid, GET_NEXT_HOB (Hob));
    }

    if (!NotifiesFound) {
      break;
    }

    //
    // Second pass, call all of the notifies that match the priority.
    //
    Hob = GetFirstGuidHob (&gPolicyCallbackHobGuid);
    while (Hob != NULL) {
      NotifyList = GET_GUID_HOB_DATA (Hob);
      for (Index = 0; Index < NotifyList->Count; Index++) {
        if (!NotifyList->Entries[Index].Tombstone &&
            CompareGuid (&NotifyList->Entries[Index].PolicyGuid, &PolicyHob->PolicyGuid) &&
            ((NotifyList->Entries[Index].EventTypes & EventTypes) != 0) &&
            (NotifyList->Entries[Index].Priority == LowestPriority))
        {
          NotifyList->Entries[Index].CallbackRoutine (
                                       &PolicyHob->PolicyGuid,
                                       EventTypes,
                                       NOTIFY_HANDLE (NotifyList->Index, Index)
                                       );

          //
          // If a more recent notify went through then quick escape.
          //
          if (PolicyHob->NotifyDepth != Depth) {
            goto LoopBreak;
          }
        }
      }

      Hob = GetNextGuidHob (&gPolicyCallbackHobGuid, GET_NEXT_HOB (Hob));
    }

    if (LowestPriority == MAX_UINT32) {
      break;
    }

    MinPriority = LowestPriority + 1;
  }

LoopBreak:

  //
  // If this is the top of the notification stack for this policy, then reset the
  // tracking field.
  //

  if (Depth == 1) {
    PolicyHob->NotifyDepth = 0;
  }
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
