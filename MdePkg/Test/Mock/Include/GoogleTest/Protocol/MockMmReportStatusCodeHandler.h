/** @file MockMmReportStatusCodeHandler.h
  This file declares a mock of Standalone MM Report Status Code Handler Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_MM_REPORT_STATUS_CODE_HANDLER_PROTOCOL_H
#define MOCK_MM_REPORT_STATUS_CODE_HANDLER_PROTOCOL_H

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/ReportStatusCodeLib.h>
  #include <Protocol/MmReportStatusCodeHandler.h>
}

struct MockEfiMmRscHandlerProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiMmRscHandlerProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Register,
    (
     IN EFI_MM_RSC_HANDLER_CALLBACK   Callback)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Unregister,
    (IN EFI_MM_RSC_HANDLER_CALLBACK Callback)
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiMmRscHandlerProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiMmRscHandlerProtocol, Register, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiMmRscHandlerProtocol, Unregister, 1, EFIAPI);

#define MOCK_EFI_MM_RSC_HANDLER_PROTOCOL_INSTANCE(NAME) \
  EFI_MM_RSC_HANDLER_PROTOCOL NAME##_INSTANCE = {       \
    Register,                                           \
    Unregister };                                       \
  EFI_MM_RSC_HANDLER_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif
