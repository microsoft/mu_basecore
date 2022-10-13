/** @file -- MemoryProtectionDebug.h

This protocol provides debug access to memory protection functionality for validation and testing.
This protocol will be installed on every boot, so no functionality exposed within should be
a sensitive or a security concern. Any potentially sensitive debug info/logic which needs to be exposed
for testing should be attached to a new protocol whose installation is blocked behind a check
that the device is not for broad consumer use (manufacturing mode, unit test mode, etc.).

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEMORY_PROTECTION_DEBUG_PROTOCOL__
#define __MEMORY_PROTECTION_DEBUG_PROTOCOL__

#define MEMORY_PROTECTION_DEBUG_PROTOCOL_GUID \
  { \
    0xe8150630, 0x6366, 0x4294, { 0xa3, 0x47, 0x89, 0x2c, 0x3a, 0x7d, 0xe4, 0xaf } \
  }

#define IMAGE_RANGE_DESCRIPTOR_SIGNATURE  SIGNATURE_32 ('I','R','D','S')

typedef enum _IMAGE_RANGE_TYPE {
  Code,
  Data
} IMAGE_RANGE_TYPE;

typedef struct _IMAGE_RANGE_DESCRIPTOR {
  UINT32                  Signature;
  LIST_ENTRY              Link;
  EFI_PHYSICAL_ADDRESS    Base;
  EFI_PHYSICAL_ADDRESS    Length;
  IMAGE_RANGE_TYPE        Type;
} IMAGE_RANGE_DESCRIPTOR;

typedef
BOOLEAN
(EFIAPI *IS_GUARD_PAGE)(
  EFI_PHYSICAL_ADDRESS    Address
  );

typedef
EFI_STATUS
(EFIAPI *GET_PROTECTED_IMAGE_LIST)(
  IMAGE_RANGE_DESCRIPTOR **ImageList
  );

typedef struct _MEMORY_PROTECTION_DEBUG_PROTOCOL {
  IS_GUARD_PAGE               IsGuardPage;
  GET_PROTECTED_IMAGE_LIST    GetProtectedImageList;
} MEMORY_PROTECTION_DEBUG_PROTOCOL;

extern EFI_GUID  gMemoryProtectionDebugProtocolGuid;

#endif
