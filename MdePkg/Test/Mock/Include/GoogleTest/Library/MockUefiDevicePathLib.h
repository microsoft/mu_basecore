/** @file MockUefiDevicePathLib.h
  Google Test mocks for UefiDevicePathLib

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_UEFI_DEVICE_PATH_LIB_H_
#define MOCK_UEFI_DEVICE_PATH_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/DevicePathLib.h>
}

struct MockUefiDevicePathLib {
  MOCK_INTERFACE_DECLARATION (MockUefiDevicePathLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    DuplicateDevicePath,
    (
     IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
    )
    );
};

#endif
