/** @file MockSmmSwDispatch2.h
  This file declares a mock of SMM Software Dispatch Protocol

  Copyright (c) Microsoft Corporation.
  Your use of this software is governed by the terms of the Microsoft agreement under which you obtained the software.
**/

#ifndef MOCK_SMM_SW_DISPATCH2_H_
#define MOCK_SMM_SW_DISPATCH2_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SmmSwDispatch2.h>
}

// Declarations to handle usage of the EFI_SMM_SW_DISPATCH2_PROTOCOL
struct MockSmmSwDispatch2Protocol {
  MOCK_INTERFACE_DECLARATION (MockSmmSwDispatch2Protocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockRegister,
    (
      IN  CONST EFI_SMM_SW_DISPATCH2_PROTOCOL  *This,
      IN        EFI_SMM_HANDLER_ENTRY_POINT2   DispatchFunction,
      IN  OUT   EFI_SMM_SW_REGISTER_CONTEXT    *RegisterContext,
      OUT       EFI_HANDLE                     *DispatchHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockUnRegister,
    (
      IN CONST EFI_SMM_SW_DISPATCH2_PROTOCOL  *This,
      IN       EFI_HANDLE                     DispatchHandle)
    );
};

MOCK_INTERFACE_DEFINITION (MockSmmSwDispatch2Protocol);
MOCK_FUNCTION_DEFINITION (MockSmmSwDispatch2Protocol, MockRegister, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmSwDispatch2Protocol, MockUnRegister, 2, EFIAPI);

EFI_SMM_SW_DISPATCH2_PROTOCOL SMM_SW_DISPATCH2_PROTOCOL_MOCK = {
  MockRegister,           // EFI_SMM_SW_REGISTER2      Register;
  MockUnRegister          // EFI_SMM_SW_UNREGISTER2    UnRegister;
};


extern "C" {
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *gSmmSwDispatch2 = &SMM_SW_DISPATCH2_PROTOCOL_MOCK;
}
#endif // MOCK_SMM_SW_DISPATCH2_H_

