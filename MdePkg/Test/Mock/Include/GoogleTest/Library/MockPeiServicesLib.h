/** @file MockPeiServicesLib.h
  Google Test mocks for PeiServicesLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PEI_SERVICES_LIB_H_
#define MOCK_PEI_SERVICES_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
#include <Uefi.h>
#include <Pi/PiPeiCis.h>
#include <Library/PeiServicesLib.h>
}

struct MockPeiServicesLib {
  MOCK_INTERFACE_DECLARATION (MockPeiServicesLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PeiServicesLocatePpi,
    (IN CONST EFI_GUID                *Guid,
     IN       UINTN                   Instance,
     IN OUT   EFI_PEI_PPI_DESCRIPTOR  **PpiDescriptor  OPTIONAL,
     IN OUT   VOID                    **Ppi)
    );
};

#endif
