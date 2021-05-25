/** @file
Helper code to consolidate the way that VariablePolicy locking is signalled
and performed between the DXE and SMM/DXE flavors.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/VariablePolicy.h>

#include <Library/VariablePolicyLockingExLib.h>
#include "VariablePolicyLockingCommon.h"

CONST VARPOL_LOCK_CALLBACK_INTERFACE  *mCallbackInterface = NULL;
EDKII_VARIABLE_POLICY_PROTOCOL        *mVariablePolicy    = NULL;
EFI_EVENT                             mReadyToBootEvent;

/**
  This helper is responsible for telemetry and any other actions that
  need to be taken if the VariablePolicy fails to lock.

  NOTE: It's possible that parts of this handling will need to become
        part of a platform policy.

  @param[in]  FailureStatus   The failure that was reported by LockVariablePolicy

**/
STATIC
VOID
VariablePolicyHandleFailureToLock (
  IN  EFI_STATUS  FailureStatus
  )
{
  // TODO VARPOL: Telemetry and reporting if necessary.
  // TODO VARPOL: Should we force a reboot or something if the interface fails to lock?
  //              Would need to account for: already locked and disabled.
  return;
}

/**
  ReadyToBoot Callback
  Lock the VariablePolicy interface if it hasn't already been locked.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
STATIC
VOID
EFIAPI
LockPolicyInterfaceAtReadyToBoot (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS  Status;

  if (mCallbackInterface != NULL) {
    DEBUG ((DEBUG_INFO, "[%a] Invoking pre-lock callback.\n", __FUNCTION__));
    Status = mCallbackInterface->PreLock (mVariablePolicy);
    ASSERT_EFI_ERROR (Status);
  }

  Status = mVariablePolicy->LockVariablePolicy ();

  if (EFI_ERROR (Status)) {
    VariablePolicyHandleFailureToLock (Status);
  } else {
    gBS->CloseEvent (Event);
  }

  if (mCallbackInterface != NULL) {
    DEBUG ((DEBUG_INFO, "[%a] Invoking post-lock callback.\n", __FUNCTION__));
    Status = mCallbackInterface->PostLock (mVariablePolicy);
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Register an instance of the VariablePolicy lock callback interface.

  @param[in] CallbackInterface  The interface to register.

  @retval EFI_SUCCESS          This interface is registered successfully.
  @retval EFI_ALREADY_STARTED  System already register this interface.
**/
EFI_STATUS
EFIAPI
RegisterVarPolLockCallbackInterface (
  IN CONST VARPOL_LOCK_CALLBACK_INTERFACE  *CallbackInterface
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (mCallbackInterface == NULL) {
    mCallbackInterface = CallbackInterface;
  } else {
    Status = EFI_ALREADY_STARTED;
  }

  return Status;
}

/**
  Initializes the Variable Policy Locking feature.

  This function is responsible for initializing the Variable Policy Locking feature by
  setting the Variable Policy and creating an event to lock the policy interface at
  the Ready To Boot phase.

  @param[in] VariablePolicy  A pointer to the EDKII_VARIABLE_POLICY_PROTOCOL instance.

  @retval EFI_SUCCESS            The Variable Policy Locking feature was successfully initialized.
  @retval EFI_INVALID_PARAMETER  The VariablePolicy parameter is NULL.
  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to initialize the feature.
  @retval Others                 An error occurred while initializing the feature.
**/
EFI_STATUS
InitializeVariablePolicyLocking (
  IN EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy
  )
{
  EFI_STATUS  Status;

  mVariablePolicy = VariablePolicy;

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             LockPolicyInterfaceAtReadyToBoot,
             NULL,
             &mReadyToBootEvent
             );

  return Status;
}

/**
  Deinitializes the variable policy locking.

  This function sets the `mVariablePolicy` variable to `NULL` and closes the `mReadyToBootEvent` event.

  @retval EFI_SUCCESS The operation was successful.
  @retval Others      An error occurred while deinitializing the feature.
**/
EFI_STATUS
DeinitVariablePolicyLocking (
  VOID
  )
{
  mVariablePolicy = NULL;
  return gBS->CloseEvent (mReadyToBootEvent);
}
