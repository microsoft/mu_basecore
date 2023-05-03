/** @file
  Implements the DXE policy protocol, providing services to publish and access
  system policy.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PolicyCommon.h"
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Policy.h>

STATIC EFI_LOCK    mPolicyListLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);
STATIC EFI_HANDLE  mImageHandle    = NULL;

POLICY_PROTOCOL  mPolicyProtocol = {
  SetPolicy,
  GetPolicy,
  RemovePolicy
};

/**
  Acquires the environment specific lock for the policy list.

**/
VOID
EFIAPI
PolicyLockAcquire (
  VOID
  )
{
  EfiAcquireLock (&mPolicyListLock);
}

/**
Release the environment specific lock for the policy list.

**/
VOID
EFIAPI
PolicyLockRelease (
  VOID
  )
{
  EfiReleaseLock (&mPolicyListLock);
}

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
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;

  Status = gBS->LocateProtocol ((EFI_GUID *)PolicyGuid, NULL, &Interface);
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
                    (EFI_GUID *)PolicyGuid,
                    NULL,
                    NULL
                    );
  }

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
  mImageHandle = ImageHandle;
  Status       = IngestPoliciesFromHob ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to ingest HOB policies. (%r)\n", __FUNCTION__, Status));
    return Status;
  }

  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gPolicyProtocolGuid,
                &mPolicyProtocol,
                NULL
                );
}
