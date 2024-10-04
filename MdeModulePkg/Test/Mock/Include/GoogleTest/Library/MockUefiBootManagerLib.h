/** @file MockUefiBootManagerLib.h
  Google Test mocks for UefiBootManagerLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_UEFI_BOOT_MANAGER_LIB_H_
#define MOCK_UEFI_BOOT_MANAGER_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Library/UefiBootManagerLib.h>
}

struct MockUefiBootManagerLib {
  MOCK_INTERFACE_DECLARATION (MockUefiBootManagerLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerConnectDevicePath,
    (EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect,
     EFI_HANDLE                *MatchingHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EfiBootManagerConnectAll,
    ()
    );
};

#endif
