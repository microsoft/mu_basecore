/** @file -- BaseBinSecurityLib.h

MS_CHANGE_?

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BASE_BIN_SECURITY_LIB_H_
#define _BASE_BIN_SECURITY_LIB_H_

/**
Initialize the security cookie present in this module

This function uses the function below to initialize the __security_cookie
value that's inserted by the compiler when the security cookie compiler
flag is not disabled.

**/
VOID
EFIAPI
InitializeSecurityCookie (
  );

/**
Initialize a security cookie

This function initializes the security cookie of which the address is passed.

@param  SecurityCookieAddress     The address of the cookie to be initialized
**/
VOID
EFIAPI
InitializeSecurityCookieAddress (
  UINT64  *SecurityCookieAddress
  );

#endif // _BASE_BIN_SECURITY_LIB_H_
