/** @file MockUefiDevicePathLib.cpp
  Google Test mocks for UefiDevicePathLib

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockUefiDevicePathLib.h>

MOCK_INTERFACE_DEFINITION (MockUefiDevicePathLib);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, DuplicateDevicePath, 1, EFIAPI);
