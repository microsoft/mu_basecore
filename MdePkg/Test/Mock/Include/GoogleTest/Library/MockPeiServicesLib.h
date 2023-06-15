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
#include <Ppi/ReadOnlyVariable2.h>
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

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    pei_GetVariable,
    (IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *This,
     IN CONST  CHAR16                           *VariableName,
     IN CONST  EFI_GUID                         *VariableGuid,
     OUT       UINT32                           *Attributes,
     IN OUT    UINTN                            *DataSize,
     OUT       VOID                             *Data OPTIONAL)
    );
};

#endif
