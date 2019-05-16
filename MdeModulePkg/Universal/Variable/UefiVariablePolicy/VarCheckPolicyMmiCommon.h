/** @file -- VarCheckPolicyMmiCommon.h
This header contains communication definitions that are shared between DXE
and the MM component of VarCheckPolicy.

Copyright (C) Microsoft Corporation

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

#ifndef _VAR_CHECK_POLICY_MMI_COMMON_H_
#define _VAR_CHECK_POLICY_MMI_COMMON_H_

#define   VAR_CHECK_POLICY_COMM_SIG       SIGNATURE_32('V', 'C', 'P', 'C')
#define   VAR_CHECK_POLICY_COMM_REVISION  1

#pragma pack(push, 1)

typedef struct _VAR_CHECK_POLICY_COMM_HEADER {
  UINT32      Signature;
  UINT32      Revision;
  UINT32      Command;
  EFI_STATUS  Result;
} VAR_CHECK_POLICY_COMM_HEADER;

typedef struct _VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS {
  BOOLEAN     State;
} VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS;

typedef struct _VAR_CHECK_POLICY_COMM_DUMP_PARAMS {
  UINT32      Size;
} VAR_CHECK_POLICY_COMM_DUMP_PARAMS;

#pragma pack(pop)

// Make sure that we will hold at least the headers.
#define   VAR_CHECK_POLICY_MIN_MM_BUFFER_SIZE	MAX((OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data) + sizeof(VAR_CHECK_POLICY_COMM_HEADER)), EFI_PAGES_TO_SIZE(4))

#define   VAR_CHECK_POLICY_COMMAND_DISABLE      0x0001
#define   VAR_CHECK_POLICY_COMMAND_IS_ENABLED   0x0002
#define   VAR_CHECK_POLICY_COMMAND_REGISTER     0x0003
#define   VAR_CHECK_POLICY_COMMAND_DUMP         0x0004
#define   VAR_CHECK_POLICY_COMMAND_LOCK         0x0005

#endif // _VAR_CHECK_POLICY_MMI_COMMON_H_
