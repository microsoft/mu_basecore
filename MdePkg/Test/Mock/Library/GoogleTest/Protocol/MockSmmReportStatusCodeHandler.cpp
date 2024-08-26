/** @file MockSmmReportStatusCodeHandler.cpp
  This file declares a mock of Report Status Code Handler Protocol in SMM.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockSmmReportStatusCodeHandler.h>

MOCK_INTERFACE_DEFINITION (MockReportStatusCodeHandler);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeHandler, Register, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeHandler, Unregister, 1, EFIAPI);

EFI_SMM_RSC_HANDLER_PROTOCOL  SmmRscHandlerProtocol = {
  Register,
  Unregister
};

extern "C" {
  EFI_SMM_RSC_HANDLER_PROTOCOL  *SmmRscHandlerProtocolServices = &SmmRscHandlerProtocol;
}
