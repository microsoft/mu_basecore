/** @file MockSmmCommunication.cpp
  Google Test mocks for SmmCommunication Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockSmmCommunication.h>

MOCK_INTERFACE_DEFINITION (MockSmmCommunicationProtocol);
MOCK_FUNCTION_DEFINITION (MockSmmCommunicationProtocol, Communicate, 3, EFIAPI);

EFI_SMM_COMMUNICATION_PROTOCOL  SMM_COMMUNICATION_PROTOCOL_INSTANCE = {
  Communicate
};

extern "C" {
  EFI_SMM_COMMUNICATION_PROTOCOL  *gSmmCommunicationProtocol = &SMM_COMMUNICATION_PROTOCOL_INSTANCE;
}
