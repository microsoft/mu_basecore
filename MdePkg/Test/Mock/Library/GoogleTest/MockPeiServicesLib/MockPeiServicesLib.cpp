/** @file MockPeiServicesLib.cpp
  Google Test mocks for PeiServicesLib
  
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockPeiServicesLib.h>

MOCK_INTERFACE_DEFINITION(MockPeiServicesLib);
MOCK_FUNCTION_DEFINITION(MockPeiServicesLib, PeiServicesLocatePpi, 4, EFIAPI);
