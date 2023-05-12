/** @file
  Implementation of the Verified Policy library for gMock.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <GoogleTest/Library/MockPolicyLib.h>

MOCK_INTERFACE_DEFINITION(MockPolicyLib);

//
// Standard policy access mock routines.
//

MOCK_FUNCTION_DEFINITION(MockPolicyLib, SetPolicy, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockPolicyLib, GetPolicy, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockPolicyLib, RemovePolicy, 1, EFIAPI);

//
// Verified policy access mock routines.
//

MOCK_FUNCTION_DEFINITION(MockPolicyLib, GetVerifiedPolicy, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockPolicyLib, CreateVerifiedPolicy, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockPolicyLib, SetVerifiedPolicy, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockPolicyLib, CloseVerifiedPolicy, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockPolicyLib, ReportVerifiedPolicyAccess, 5, EFIAPI);
