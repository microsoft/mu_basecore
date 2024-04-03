/** @file MockIoLib.cpp
  Google Test mocks for IoLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockIoLib.h>

MOCK_INTERFACE_DEFINITION (MockIoLib);

MOCK_FUNCTION_DEFINITION (MockIoLib, MmioRead32, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, MmioAndThenOr32, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, IoWrite8, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, IoRead8, 1, EFIAPI);
