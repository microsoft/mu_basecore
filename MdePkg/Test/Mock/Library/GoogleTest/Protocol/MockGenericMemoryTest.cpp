/** @file MockGenericMemoryTest.cpp
  Google Test mock for generic memory test Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockGenericMemoryTest.h>

MOCK_INTERFACE_DEFINITION (MockMemTestProtocol);
MOCK_FUNCTION_DEFINITION (MockMemTestProtocol, MemoryTestInit, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemTestProtocol, PerformMemoryTest, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemTestProtocol, Finished, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemTestProtocol, CompatibleRangeTest, 3, EFIAPI);

EFI_GENERIC_MEMORY_TEST_PROTOCOL  MEMORY_TEST_PROTOCOL_INSTANCE = {
  MemoryTestInit,                                                                       // EFI_MEMORY_TEST_INIT
  PerformMemoryTest,                                                                    // EFI_PERFORM_MEMORY_TEST
  Finished,                                                                             // EFI_MEMORY_TEST_FINISHED
  CompatibleRangeTest                                                                   // EFI_MEMORY_TEST_COMPATIBLE_RANGE
};

extern "C" {
  extern EFI_GENERIC_MEMORY_TEST_PROTOCOL  *gMemTestProtocol = &MEMORY_TEST_PROTOCOL_INSTANCE;
}
