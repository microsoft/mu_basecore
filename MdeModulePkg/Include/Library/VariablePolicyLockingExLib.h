/** @file
This library contains definitions necessary for a platform to register
a pre- and post-lock callback on the VariablePolicy interface.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_POLICY_LOCKING_EX_LIB_H_
#define _VARIABLE_POLICY_LOCKING_EX_LIB_H_

/**
  A callback that is triggered immediately prior to locking VariablePolicy.

  @param[in]  VariablePolicy    A pointer to the VariablePolicy protocol interface that
                                is about to be locked.

  @retval EFI_SUCCESS         Successfully registered.
  @retval EFI_UNSUPPORTED     This callback is not supported.
  @retval Others              Some unspecified error has occured.

**/
typedef
EFI_STATUS
(EFIAPI *PRE_VARPOL_LOCK)(
  IN EDKII_VARIABLE_POLICY_PROTOCOL   *VariablePolicy
  );

/**
  A callback that is triggered immediately after locking VariablePolicy.

  @param[in]  VariablePolicy    A pointer to the VariablePolicy protocol interface that
                                has just been locked.

  @retval EFI_SUCCESS         Successfully registered.
  @retval EFI_UNSUPPORTED     This callback is not supported.
  @retval Others              Some unspecified error has occured.

**/
typedef
EFI_STATUS
(EFIAPI *POST_VARPOL_LOCK)(
  IN EDKII_VARIABLE_POLICY_PROTOCOL   *VariablePolicy
  );

typedef struct {
  PRE_VARPOL_LOCK     PreLock;
  POST_VARPOL_LOCK    PostLock;
} VARPOL_LOCK_CALLBACK_INTERFACE;

/**
  Register an instance of the VariablePolicy lock callback interface.

  @retval EFI_SUCCESS          This interface is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this interface.
  @retval EFI_ALREADY_STARTED  System already register this interface.
**/
EFI_STATUS
EFIAPI
RegisterVarPolLockCallbackInterface (
  IN CONST VARPOL_LOCK_CALLBACK_INTERFACE  *CallbackInterface
  );

#endif // _VARIABLE_POLICY_LOCKING_EX_LIB_H_
