/** @file
Helper code to consolidate the way that VariablePolicy locking is signalled
and performed between the DXE and SMM/DXE flavors.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef VARIABLE_POLICY_LOCKING_COMMON_H_
#define VARIABLE_POLICY_LOCKING_COMMON_H_

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
  );

/**
  Deinitializes the variable policy locking.

  This function sets the `mVariablePolicy` variable to `NULL` and closes the `mReadyToBootEvent` event.

  @retval EFI_SUCCESS The operation was successful.
  @retval Others      An error occurred while deinitializing the feature.
**/
EFI_STATUS
DeinitVariablePolicyLocking (
  VOID
  );

#endif // VARIABLE_POLICY_LOCKING_COMMON_H_
