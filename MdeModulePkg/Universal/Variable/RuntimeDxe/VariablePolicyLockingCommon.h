/** @file
Helper code to consolidate the way that VariablePolicy locking is signalled
and performed between the DXE and SMM/DXE flavors.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_POLICY_LOCKING_COMMON_H_
#define _VARIABLE_POLICY_LOCKING_COMMON_H_

EFI_STATUS
InitializeVariablePolicyLocking (
  IN EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy
  );

EFI_STATUS
DeinitVariablePolicyLocking (
  VOID
  );

#endif // _VARIABLE_POLICY_LOCKING_COMMON_H_
