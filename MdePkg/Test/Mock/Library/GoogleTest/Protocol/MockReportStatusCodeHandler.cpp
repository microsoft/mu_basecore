/** @file MockReportStatusCodeHandler.cpp
  This file declares a mock of Report Status Code Handler Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockReportStatusCodeHandler.h>

MOCK_INTERFACE_DEFINITION (MockReportStatusCodeHandler);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeHandler, Register, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeHandler, Unregister, 1, EFIAPI);

EFI_RSC_HANDLER_PROTOCOL  RscHandlerProtocol = {
  Register,
  Unregister
};

extern "C" {
  EFI_RSC_HANDLER_PROTOCOL  *RscHandlerProtocolServices = &RscHandlerProtocol;
}
