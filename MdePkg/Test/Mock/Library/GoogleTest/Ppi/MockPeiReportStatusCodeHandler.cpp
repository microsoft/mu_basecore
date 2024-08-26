/** @file MockPeiReportStatusCodeHandler.cpp
  Google Test mock for PeiReportStatusCodeHandler

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Ppi/MockPeiReportStatusCodeHandler.h>

MOCK_INTERFACE_DEFINITION (MockPeiReportStatusCodeHandler);
MOCK_FUNCTION_DEFINITION (MockPeiReportStatusCodeHandler, Register, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPeiReportStatusCodeHandler, Unregister, 1, EFIAPI);

EFI_PEI_RSC_HANDLER_PPI  PeiRscHandlerPpi = {
  Register,
  Unregister
};

extern "C" {
  EFI_PEI_RSC_HANDLER_PPI  *PeiRscHandlerPpiServices = &PeiRscHandlerPpi;
}
