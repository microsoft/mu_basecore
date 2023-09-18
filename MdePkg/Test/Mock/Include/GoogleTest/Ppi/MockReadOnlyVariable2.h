/** @file MockReadOnlyVariable2.h
  This file declares a mock of Read-only Variable Service2 PPI.
  This ppi permits read-only access to the UEFI variable store during the PEI phase.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PEI_READ_ONLY_VARIABLE2_PPI_H
#define MOCK_PEI_READ_ONLY_VARIABLE2_PPI_H

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
#include <Uefi.h>
#include <Pi/PiPeiCis.h>
#include <Ppi/ReadOnlyVariable2.h>
}

struct MockReadOnlyVariable2 {
  MOCK_INTERFACE_DECLARATION (MockReadOnlyVariable2);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PPIVariableServices_GetVariable,
    (IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *This,
     IN CONST  CHAR16                           *VariableName,
     IN CONST  EFI_GUID                         *VariableGuid,
     OUT       UINT32                           *Attributes,
     IN OUT    UINTN                            *DataSize,
     OUT       VOID                             *Data OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PPIVariableServices_NextVariableName,
    (IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI *This,
     IN OUT    UINTN                           *VariableNameSize,
     IN OUT    CHAR16                          *VariableName,
     IN OUT    EFI_GUID                        *VariableGuid)
    );
};

MOCK_INTERFACE_DEFINITION (MockReadOnlyVariable2);
MOCK_FUNCTION_DEFINITION (MockReadOnlyVariable2, PPIVariableServices_GetVariable, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReadOnlyVariable2, PPIVariableServices_NextVariableName, 4, EFIAPI);

// Normally PPIVariableServices is "found"
// This will be defined INSIDE the test, with its definition pointing to the mock function getVariable
static EFI_PEI_READ_ONLY_VARIABLE2_PPI  peiReadOnlyVariablePpi = {
  PPIVariableServices_GetVariable,      // EFI_PEI_GET_VARIABLE2
  PPIVariableServices_NextVariableName  // EFI_PEI_GET_NEXT_VARIABLE_NAME2
};

extern "C" {
EFI_PEI_READ_ONLY_VARIABLE2_PPI  *PPIVariableServices = &peiReadOnlyVariablePpi;
}

#endif