/** @file
  Google Test mocks for PolicyLibCommon

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_POLICY_LIB_COMMON_LIB_H_
#define MOCK_POLICY_LIB_COMMON_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PolicyLib.h>
}

struct MockPolicyLibCommon {
  MOCK_INTERFACE_DECLARATION (MockPolicyLibCommon);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetPolicy,
    (
     IN CONST EFI_GUID  *PolicyGuid,
     OUT UINT64         *Attributes OPTIONAL,
     OUT VOID           *Policy,
     IN OUT UINT16      *PolicySize
    )
    );
};

#endif
