/** @file MockGenericMemoryTest.h
  This file declares a mock of generic memory test Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_GENERIC_MEMORY_TEST_H
#define MOCK_GENERIC_MEMORY_TEST_H

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/GenericMemoryTest.h>
}
struct MockMemTestProtocol {
  MOCK_INTERFACE_DECLARATION (MockMemTestProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MemoryTestInit,
    (
     IN  EFI_GENERIC_MEMORY_TEST_PROTOCOL         *This,
     IN  EXTENDMEM_COVERAGE_LEVEL                 Level,
     OUT BOOLEAN                                  *RequireSoftECCInit
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PerformMemoryTest,
    (
     IN  EFI_GENERIC_MEMORY_TEST_PROTOCOL         *This,
     OUT UINT64                                   *TestedMemorySize,
     OUT UINT64                                   *TotalMemorySize,
     OUT BOOLEAN                                  *ErrorOut,
     IN  BOOLEAN                                  IfTestAbort
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Finished,
    (
     IN  EFI_GENERIC_MEMORY_TEST_PROTOCOL         *This
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CompatibleRangeTest,
    (
     IN  EFI_GENERIC_MEMORY_TEST_PROTOCOL         *This,
     IN  EFI_PHYSICAL_ADDRESS                     StartAddress,
     IN  UINT64                                   Length
    )
    );
};

extern "C" {
  extern EFI_GENERIC_MEMORY_TEST_PROTOCOL  *gMemTestProtocol;
}

#endif // MOCK_GENERIC_MEMORY_TEST_H
