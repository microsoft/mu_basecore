/** @file -- CpuMpDebug.h

This protocol provides debug access to AP buffer information for validation and testing.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CPU_MP_DEBUG_PROTOCOL__
#define __CPU_MP_DEBUG_PROTOCOL__

#define CPU_MP_DEBUG_PROTOCOL_GUID \
  { \
    0xce05eb65, 0x9416, 0x4197, {0x98, 0x4b, 0xa2, 0x8b, 0x62, 0x9c, 0x64, 0x4d } \
  }

#define CPU_MP_DEBUG_SIGNATURE  SIGNATURE_32 ('C','M','P','S')

typedef struct _CPU_MP_DEBUG_PROTOCOL {
  UINT32        Signature;
  UINTN         ApStackBuffer;
  UINTN         ApStackSize;
  UINTN         CpuNumber;
  BOOLEAN       IsSwitchStack;
  LIST_ENTRY    Link;
} CPU_MP_DEBUG_PROTOCOL;

extern EFI_GUID  gCpuMpDebugProtocolGuid;

#endif
