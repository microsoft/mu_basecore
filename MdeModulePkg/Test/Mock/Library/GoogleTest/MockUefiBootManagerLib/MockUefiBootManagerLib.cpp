/** @file MockUefiBootManagerLib.cpp
  Mock instance of the PCI Host Bridge Library.

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockUefiBootManagerLib.h>

MOCK_INTERFACE_DEFINITION (MockUefiBootManagerLib);

MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerConnectDevicePath, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootManagerLib, EfiBootManagerConnectAll, 0, EFIAPI);
