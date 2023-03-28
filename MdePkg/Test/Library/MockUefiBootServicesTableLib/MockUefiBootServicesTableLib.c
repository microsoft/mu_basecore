/** @file
  Mock implementation of the UEFI Boot Services Table Library.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

extern EFI_BOOT_SERVICES  MockBoot;
extern EFI_SYSTEM_TABLE   MockSys;


EFI_BOOT_SERVICES  *gBS = &MockBoot;
EFI_SYSTEM_TABLE   *gST = &MockSys;
