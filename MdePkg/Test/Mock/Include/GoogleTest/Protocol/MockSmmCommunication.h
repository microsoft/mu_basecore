/** @file MockSmmCommunication.h
  Declare mock SMM Communication Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SMM_COMMUNICATION_H_
#define MOCK_SMM_COMMUNICATION_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Pi/PiMultiPhase.h>
  #include <Protocol/SmmCommunication.h>
}

struct MockSmmCommunicationProtocol {
  MOCK_INTERFACE_DECLARATION (MockSmmCommunicationProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Communicate,
    (
     IN CONST EFI_MM_COMMUNICATION_PROTOCOL   *This,
     IN OUT VOID                              *CommBuffer,
     IN OUT UINTN                             *CommSize OPTIONAL
    )
    );
};

extern "C" {
  extern EFI_SMM_COMMUNICATION_PROTOCOL  *gSmmCommunicationProtocol;
}

#endif
