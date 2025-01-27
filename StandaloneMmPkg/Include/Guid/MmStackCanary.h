/** @file
  GUIDs for Standalone Mm Stack Canary.

Copyright (c) 2024, MediaTek Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MM_STACK_CANARY_H__
#define __MM_STACK_CANARY_H__

#define MM_STACK_CANARY_GUID \
  { 0x43723e6c, 0xcdbb, 0x4ae9, { 0x83, 0xa7, 0x7e, 0x35, 0x1b, 0x4d, 0x7b, 0x18 }}

extern EFI_GUID  gMmStackCanaryGuid;

///
/// Structure that is used for StandaloneMmCore (MM driver loader) to set driver's
/// stack canary. Driver should install this struct to MM ConfigurationTable during
/// its entrypoint.
///
typedef struct {
  ///
  /// Address of driver's __stack_chk_guard
  ///
  VOID**  AddressOfGuard;

  ///
  /// Caller's Guid. driver can get this info through gEfiCallerIdGuid
  ///
  GUID    ImageGuid;
} MM_STACK_CANARY;

#endif

