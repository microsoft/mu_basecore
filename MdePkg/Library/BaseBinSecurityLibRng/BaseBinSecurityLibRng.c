/** @file -- BaseBinSecurityLibRng.c

MS_CHANGE_?

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
UINT64 __security_cookie = 0;

//
// Define the _load_config_used symbol for
// the linker to export so that the loader
// can find the location of the security cookie
// with in the binary later
//
__declspec(selectany)
EFI_IMAGE_LOAD_CONFIG_DIRECTORY   _load_config_used = {
    sizeof(EFI_IMAGE_LOAD_CONFIG_DIRECTORY),
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
InitializeSecurityCookie()
{
    UINT64 RandomValue = 0;
    InitializeSecurityCookieAddress(&RandomValue);
    __security_cookie = RandomValue;
}

/**
Initialize a security cookie

This function initializes the security cookie of which the address is passed.

@param  SecurityCookieAddress     The address of the cookie to be initialized
**/
VOID
EFIAPI
InitializeSecurityCookieAddress(UINT64* SecurityCookieAddress)
{
    GetRandomNumber64(SecurityCookieAddress);
}
