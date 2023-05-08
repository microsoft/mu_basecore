/** @file
  This protocol provides services to publish, update, and retrieve general policies in the MM
  environment.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MM_POLICY_PROTOCOL_H_
#define _MM_POLICY_PROTOCOL_H_

#include <PolicyInterface.h>

#define MM_POLICY_PROTOCOL_GUID  { 0xe55ad3a1, 0xbd34, 0x46f4, { 0xbb, 0x6e, 0x72, 0x28, 0x0b, 0xdc, 0xbf, 0xd9 } }

typedef struct _POLICY_INTERFACE MM_POLICY_PROTOCOL;

extern EFI_GUID  gMmPolicyProtocolGuid;

#endif
