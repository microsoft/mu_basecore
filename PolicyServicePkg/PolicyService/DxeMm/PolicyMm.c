/** @file
  Implements the MM policy protocol, providing services to publish and
  access system policy.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/MmServicesTableLib.h>
#include "PolicyCommon.h"
#include <Protocol/MmPolicy.h>

STATIC EFI_HANDLE  mProtocolHandle = NULL;

MM_POLICY_PROTOCOL  mPolicyProtocol = {
  CommonSetPolicy,
  CommonGetPolicy,
  CommonRemovePolicy,
  CommonRegisterNotify,
  CommonUnregisterNotify
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
  // Nothing to do.
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
  // Nothing to do.
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

  //
  // Attempt to uninstall to make sure to republish on update. It is okay if this
  // fails.
  //

  if (mProtocolHandle != NULL) {
    gMmst->MmUninstallProtocolInterface (
             mProtocolHandle,
             (EFI_GUID *)PolicyGuid,
             NULL
             );
  }

  Status = gMmst->MmInstallProtocolInterface (
                    &mProtocolHandle,
                    (EFI_GUID *)PolicyGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );

  return Status;
}

/**
  Common Entry of the MM policy service module.

  @retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
PolicyMmCommonEntry (
  VOID
  )
{
  EFI_STATUS  Status;

  // Process the HOBs to consume any existing policies.
  Status = IngestPoliciesFromHob ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to ingest HOB policies. (%r)\n", __FUNCTION__, Status));
    return Status;
  }

  Status =  gMmst->MmInstallProtocolInterface (
                     &mProtocolHandle,
                     &gMmPolicyProtocolGuid,
                     EFI_NATIVE_INTERFACE,
                     &mPolicyProtocol
                     );

  return Status;
}
