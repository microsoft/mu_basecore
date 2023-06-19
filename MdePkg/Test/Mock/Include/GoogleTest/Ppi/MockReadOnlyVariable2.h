/** @file MockReadOnlyVariable2.h
  This file declares a mock of Read-only Variable Service2 PPI.
  This ppi permits read-only access to the UEFI variable store during the PEI phase.

  Copyright (c) Miocrosoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MOCK_PEI_READ_ONLY_VARIABLE2_PPI_H__
#define __MOCK_PEI_READ_ONLY_VARIABLE2_PPI_H__

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
