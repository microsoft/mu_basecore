/** @file
  Google Test mocks for PolicyLibCommon

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockPolicyLibCommon.h>

MOCK_INTERFACE_DEFINITION (MockPolicyLibCommon);
MOCK_FUNCTION_DEFINITION (MockPolicyLibCommon, GetPolicy, 4, EFIAPI);
