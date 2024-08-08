/** @file    MockDebugPort.cpp

    Google Test mock for Hash2 Protocol

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockDebugPort.h>

MOCK_INTERFACE_DEFINITION (MockDebugPort);
MOCK_FUNCTION_DEFINITION (MockDebugPort, Reset, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDebugPort, Write, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDebugPort, Read, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDebugPort, Poll, 1, EFIAPI);

EFI_DEBUGPORT_PROTOCOL  DEBUG_PORT_PROTOCOL_INSTANCE = {
  Reset, // EFI_DEBUGPORT_RESET
  Write, // EFI_DEBUGPORT_WRITE
  Read,  // EFI_DEBUGPORT_READ
  Poll   // EFI_DEBUGPORT_POLLF
};

extern "C" {
  EFI_DEBUGPORT_PROTOCOL  *gDebugPortProtocol = &DEBUG_PORT_PROTOCOL_INSTANCE;
}
