/** @file -- SmmExceptionTestProtocol.h
A simple protocol to enable SMM exception handling being placed in test mode.

Copyright (c) 2017, Microsoft Corporation

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

#ifndef _SMM_EXCEPTION_TEST_PROTOCOL_H_
#define _SMM_EXCEPTION_TEST_PROTOCOL_H_

// B76383A1-0E70-4A3F-86B4-C6134C8E5723
#define SMM_EXCEPTION_TEST_PROTOCOL_GUID \
  { \
    0xb76383a1, 0x0e70, 0x4a3f, { 0x86, 0xb4, 0xc6, 0x13, 0x4c, 0x8e, 0x57, 0x23 } \
  }

extern EFI_GUID gSmmExceptionTestProtocolGuid;

/**
  Enable exception handling test mode.

  NOTE: This should only work on debug builds, otherwise return EFI_UNSUPPORTED.

  @retval EFI_SUCCESS            Test mode enabled.
  @retval EFI_UNSUPPORTED        Test mode could not be enabled.
**/
typedef
EFI_STATUS
(EFIAPI *SMM_ENABLE_EXCEPTION_TEST_MODE)(
  VOID
  );

typedef struct _SMM_EXCEPTION_TEST_PROTOCOL
{
  SMM_ENABLE_EXCEPTION_TEST_MODE    EnableTestMode;
} SMM_EXCEPTION_TEST_PROTOCOL;

#endif // _SMM_EXCEPTION_TEST_PROTOCOL_H_
