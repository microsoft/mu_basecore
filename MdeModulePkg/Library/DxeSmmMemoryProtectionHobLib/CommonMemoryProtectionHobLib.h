/**@file
Internal interfaces used by MemoryProtectionHobLib instances.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MEMORY_PROTECTION_HOB_LIB_H_
#define MEMORY_PROTECTION_HOB_LIB_H_

/**
  Populates gMPS global with the data present in the HOB. If the HOB entry does not exist,
  this constructor will zero the memory protection settings.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
CommonMemoryProtectionHobLibConstructor (
  VOID
  );

#endif // MEMORY_PROTECTION_HOB_LIB_H_
