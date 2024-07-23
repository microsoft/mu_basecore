/** @file MockUefiDevicePathLib.cpp
  Google Test mocks for UefiDevicePathLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockUefiDevicePathLib.h>

MOCK_INTERFACE_DEFINITION (MockUefiDevicePathLib);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, DuplicateDevicePath, 1, EFIAPI);
