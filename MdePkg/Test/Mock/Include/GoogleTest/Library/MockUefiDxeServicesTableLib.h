/** @file
  Google Test mocks for DxeServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_UEFI_DXE_SERVICES_TABLE_LIB_H_
#define MOCK_UEFI_DXE_SERVICES_TABLE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Pi/PiDxeCis.h>
}

//
// Declarations to handle usage of the DxeServicesTableLib by creating mock
//
struct MockUefiDxeServicesTableLib {
  MOCK_INTERFACE_DECLARATION (MockUefiDxeServicesTableLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gDS_Dispatch,
    ()
    );
};

#endif // MOCK_UEFI_DXE_SERVICES_TABLE_LIB_H_
