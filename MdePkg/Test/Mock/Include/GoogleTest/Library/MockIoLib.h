/** @file MockIoLib.h
  Google Test mocks for IoLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_IO_LIB_H_
#define MOCK_IO_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/IoLib.h>
}

struct MockIoLib {
  MOCK_INTERFACE_DECLARATION (MockIoLib);

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioRead32,
    (
     IN      UINTN  Address
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioAndThenOr32,
    (
     IN      UINTN   Address,
     IN      UINT32  AndData,
     IN      UINT32  OrData
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoWrite8,
    (
     IN      UINTN  Port,
     IN      UINT8  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoRead8,
    (
     IN      UINTN  Port
    )
    );
};

#endif
