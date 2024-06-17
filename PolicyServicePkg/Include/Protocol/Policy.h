/** @file
  This protocol provides services to publish, update, and retrieve general policies in the DXE
  environment.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <PolicyInterface.h>

#define POLICY_PROTOCOL_GUID  {0xd7c9b744, 0x13a5, 0x4377, { 0x8d, 0x2a, 0x6b, 0x37, 0xad, 0x1f, 0xd8, 0x2a } }

typedef struct _POLICY_INTERFACE POLICY_PROTOCOL;

extern EFI_GUID  gPolicyProtocolGuid;
