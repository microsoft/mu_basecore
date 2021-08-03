/** @file
  This file defines the GUID and data structure used to pass information about
  a variable store mapped on flash (i.e. a MMIO firmware volume) to the DXE and MM environment.

  Copyright (c) Microsoft Corporation.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// MU_CHANGE - WHOLE FILE - TCBZ3479  - Add Variable Flash Information HOB

#ifndef __VARIABLE_FLASH_INFO_H__
#define __VARIABLE_FLASH_INFO_H__

#define VARIABLE_FLASH_INFO_HOB_GUID \
  { 0x5d11c653, 0x8154, 0x4ac3, { 0xa8, 0xc2, 0xfb, 0xa2, 0x89, 0x20, 0xfc, 0x90 }}

extern EFI_GUID gVariableFlashInfoHobGuid;

#pragma pack (push, 1)

///
/// This structure can be used to describe UEFI variable
/// flash information.
///
typedef struct {
  EFI_PHYSICAL_ADDRESS        BaseAddress;
  UINT64                      Length;
} VARIABLE_FLASH_INFO;

#pragma pack (pop)

#endif // __VARIABLE_FLASH_INFO_H__
