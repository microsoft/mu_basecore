/** @file
  Google Test mocks for PolicyLibCommon

  Copyright (c) Microsoft Corporation.
  Your use of this software is governed by the terms of the Microsoft agreement under which you obtained the software.
**/

#include <GoogleTest/Library/MockPolicyLibCommon.h>

MOCK_INTERFACE_DEFINITION (MockPolicyLibCommon);
MOCK_FUNCTION_DEFINITION (MockPolicyLibCommon, GetPolicy, 4, EFIAPI);
