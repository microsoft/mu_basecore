/** @file -- BaseBinSecurityLibRng.c

MS_CHANGE_?

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseBinSecurityLib.h>
#include <Library/PeCoffLib.h>
#include <Library/RngLib.h>                        // GetRandomNumber64()

//
// Initialize the initial security cookie to zero
// in case we don't initialize it by the loader
// somewhere
//
UINT64  __security_cookie = 0;

//
// Define the _load_config_used symbol for
// the linker to export so that the loader
// can find the location of the security cookie
// with in the binary later
//
__declspec(selectany)
EFI_IMAGE_LOAD_CONFIG_DIRECTORY   _load_config_used = {
  sizeof (EFI_IMAGE_LOAD_CONFIG_DIRECTORY),
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  (UINT64)&__security_cookie,
  0,
  0
};

/**
Initialize the security cookie present in this module

This function uses the function below to initialize the __security_cookie
value that's inserted by the compiler when the security cookie compiler
flag is not disabled.

**/
VOID
EFIAPI
InitializeSecurityCookie (
  )
{
  UINT64  RandomValue = 0;

  InitializeSecurityCookieAddress (&RandomValue);
  __security_cookie = RandomValue;
}

/**
Initialize a security cookie

This function initializes the security cookie of which the address is passed.

@param  SecurityCookieAddress     The address of the cookie to be initialized
**/
VOID
EFIAPI
InitializeSecurityCookieAddress (
  UINT64  *SecurityCookieAddress
  )
{
  GetRandomNumber64 (SecurityCookieAddress);
}
