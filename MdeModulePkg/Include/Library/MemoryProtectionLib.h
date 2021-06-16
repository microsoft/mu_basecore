/**@file

Library for controlling memory protection variables/settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEM_PROT_LIB_H__
#define __MEM_PROT_LIB_H__

#include <Uefi.h>

/**
 Updates the memory protection global toggle

  @param[in] Setting      What the memory protection global toggle should
                          be set to

  @retval EFI_SUCCESS     Memory protection global toggle was updated
  @retval EFI_UNSUPPORTED This function is either not available in this
                          execution phase or a null instance is used
 **/
EFI_STATUS
EFIAPI
SetMemoryProtectionGlobalToggle(
  IN BOOLEAN     Setting
  );

/**
 Updates the memory protection global toggle

  @retval TRUE            Memory protection global toggle is on
  @retval FALSE           Memory protection global toggle is off, meaning
                          no memory protections can be initialized
 **/
BOOLEAN
EFIAPI
IsMemoryProtectionGlobalToggleEnabled(
  VOID
  );

#endif