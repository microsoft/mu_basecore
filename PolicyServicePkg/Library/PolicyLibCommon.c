/** @file
  The common implementation of the verified policy library routines.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/VerifiedPolicy.h>
#include <PolicyInterface.h>

#include "../PolicyLibCommon.h"

/**
  Retrieves a verified policy of the given type from the policy store.

  @param[in]    PolicyGuid      The GUID of policy in the policy store.

  @param[in]    Descriptor      The descriptor for the verified policy data
                                structure.

  @param[out]   Attributes      Returns the attributes of the policy in the
                                policy store.

  @param[out]   DataHandle      Returns the handle to the verified policy.

  @retval   EFI_SUCCESS               The policy was successfully retrieved.
  @retval   EFI_BAD_BUFFER_SIZE       The policy was an unexpected size.
  @retval   EFI_INCOMPATIBLE_VERSION  The verified policy major version did not
                                      match.
  @retval   EFI_OUT_OF_RESOURCES      Failed to allocate memory.
**/
RETURN_STATUS
EFIAPI
GetVerifiedPolicy (
  IN CONST EFI_GUID                    *PolicyGuid,
  IN CONST VERIFIED_POLICY_DESCRIPTOR  *Descriptor,
  OUT UINT64                           *Attributes OPTIONAL,
  OUT EFI_HANDLE                       *DataHandle
  )
{
  VOID                    *Buffer;
  UINT16                  BufferSize;
  POLICY_INTERFACE        *PolicyService;
  EFI_STATUS              Status;
  VERIFIED_POLICY_HEADER  *Header;

  Buffer      = NULL;
  BufferSize  = 0;
  Header      = NULL;
  *DataHandle = NULL;

  Status = GetPolicyInterface (&PolicyService);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = PolicyService->GetPolicy (PolicyGuid, Attributes, &Buffer, &BufferSize);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    ASSERT (EFI_ERROR (Status));
    goto Exit;
  }

  if (BufferSize < sizeof (VERIFIED_POLICY_HEADER)) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Exit;
  }

  Buffer = AllocatePool (BufferSize);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Status = PolicyService->GetPolicy (PolicyGuid, Attributes, Buffer, &BufferSize);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Header = (VERIFIED_POLICY_HEADER *)Buffer;
  if ((Descriptor->Signature != Header->Signature) ||
      (Descriptor->MajorVersion != Header->MajorVersion))
  {
    Status = EFI_INCOMPATIBLE_VERSION;
    goto Exit;
  }

  if (Descriptor->Size != Header->Size) {
    ASSERT (FALSE);
    Status = EFI_BAD_BUFFER_SIZE;
    goto Exit;
  }

  *DataHandle = Buffer;

Exit:
  if (EFI_ERROR (Status) && (Buffer != NULL)) {
    FreePool (Buffer);
  }

  return Status;
}

/**
  Creates a new verified policy data structure.

  @param[in]  Descriptor    The descriptor of the verified policy data structure
                            to be created.

  @param[out] DataHandle    The handle to the newly created verified policy data
                            structure.

  @retval     EFI_SUCCESS           The data structure was successfully created.
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate memory.
**/
RETURN_STATUS
EFIAPI
CreateVerifiedPolicy (
  IN CONST VERIFIED_POLICY_DESCRIPTOR  *Descriptor,
  OUT EFI_HANDLE                       *DataHandle
  )
{
  VOID                    *Buffer;
  VERIFIED_POLICY_HEADER  *Header;
  UINT32                  PolicySize;

  *DataHandle = NULL;

  PolicySize = sizeof (VERIFIED_POLICY_HEADER) + Descriptor->Size;
  Buffer     = AllocatePool (PolicySize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Buffer, PolicySize);
  Header               = (VERIFIED_POLICY_HEADER *)Buffer;
  Header->Signature    = Descriptor->Signature;
  Header->MajorVersion = Descriptor->MajorVersion;
  Header->MinorVersion = Descriptor->MinorVersion;
  Header->Size         = Descriptor->Size;
  *DataHandle          = Buffer;
  return EFI_SUCCESS;
}

/**
  Write a verified policy to the policy store.

  @param[in]    PolicyGuid      The GUID of policy in the policy store.

  @param[in]    Attributes      The attributes to set in the policy store.

  @param[in]    DataHandle      The handle to the policy data.

  @retval   EFI_SUCCESS               The policy was successfully retrieved.
  @retval   EFI_INVALID_PARAMETER     DataHandle is NULL.
  @retval   EFI_BAD_BUFFER_SIZE       The policy is too large.
**/
RETURN_STATUS
EFIAPI
SetVerifiedPolicy (
  IN CONST EFI_GUID  *PolicyGuid,
  IN UINT64          Attributes,
  IN EFI_HANDLE      DataHandle
  )
{
  UINT16                  BufferSize;
  POLICY_INTERFACE        *PolicyService;
  EFI_STATUS              Status;
  VERIFIED_POLICY_HEADER  *Header;
  UINT32                  PolicySize;

  if (DataHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Header     = (VERIFIED_POLICY_HEADER *)DataHandle;
  PolicySize = sizeof (VERIFIED_POLICY_HEADER) + Header->Size;
  if (PolicySize > MAX_UINT16) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = GetPolicyInterface (&PolicyService);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferSize = (UINT16)PolicySize;
  return PolicyService->SetPolicy (PolicyGuid, Attributes, DataHandle, BufferSize);
}

/**
  Closes a policy data handle.

  @param[in]    DataHandle      The policy handle to be closed.

  @retval       EFI_SUCCESS             The policy handle was successfully
                                        closed.
  @retval       EFI_INVALID_PARAMETER   The data handle is NULL.
**/
RETURN_STATUS
EFIAPI
CloseVerifiedPolicy (
  IN EFI_HANDLE  DataHandle
  )
{
  if (DataHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FreePool (DataHandle);
  return EFI_SUCCESS;
}

/**
  Records access to a policy data structure used by autogenerated code. This
  function should not be manually called.

  @param[in]    DataHandle      The policy handle where the access was made.
  @param[in]    CallerGuid      The file guid of the caller for tracking access.
  @param[in]    Offset          The offset into the policy data for the access.
  @param[in]    Size            The size of the access.
  @param[in]    Write           Indicates if the policy was written to.

**/
VOID
EFIAPI
ReportVerifiedPolicyAccess (
  IN EFI_HANDLE      DataHandle,
  IN CONST EFI_GUID  *CallerGuid,
  IN UINT32          Offset,
  IN UINT32          Size,
  IN BOOLEAN         Write
  )

{
  //
  // Currently a no-op, but leaving room for future checks and analytics.
  //

  return;
}
