/** @file
  Google Test mocks for SecureBootVariableLib

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SECURE_BOOT_VARIABLE_LIB_H_
#define MOCK_SECURE_BOOT_VARIABLE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <UefiSecureBoot.h>
  #include <Guid/ImageAuthentication.h>
  #include <Library/SecureBootVariableLib.h>
}

struct MockSecureBootVariableLib {
  MOCK_INTERFACE_DECLARATION (MockSecureBootVariableLib);

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    IsSecureBootEnabled,
    ()
    );
};

#endif
