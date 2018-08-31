/** @file -- HeapGuardTestApp.c

This protocol provides debug access to Heap Guard memory protections to allow
validation of the memory protections.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HEAP_GUARD_DEBUG_PROTOCOL__
#define __HEAP_GUARD_DEBUG_PROTOCOL__

#define HEAP_GUARD_DEBUG_PROTOCOL_GUID \
  { \
    0xe8150630, 0x6366, 0x4294, { 0xa3, 0x47, 0x89, 0x2c, 0x3a, 0x7d, 0xe4, 0xaf } \
  }

typedef
BOOLEAN
(EFIAPI *IS_GUARD_PAGE)(
  EFI_PHYSICAL_ADDRESS    Address
  );

typedef struct {
  IS_GUARD_PAGE    IsGuardPage;
} _HEAP_GUARD_DEBUG_PROTOCOL;

typedef _HEAP_GUARD_DEBUG_PROTOCOL HEAP_GUARD_DEBUG_PROTOCOL;

extern EFI_GUID  gHeapGuardDebugProtocolGuid;

#endif
