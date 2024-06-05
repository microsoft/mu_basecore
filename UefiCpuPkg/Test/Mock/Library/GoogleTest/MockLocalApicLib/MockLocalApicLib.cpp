/** @file
  Google Test mocks for LocalApicLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockLocalApicLib.h>

MOCK_INTERFACE_DEFINITION (MockLocalApicLib);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetProcessorLocationByApicId, 4, EFIAPI);
