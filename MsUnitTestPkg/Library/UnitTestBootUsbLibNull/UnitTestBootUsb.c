/**

NULL implementation for UnitTestBootUsbLib to allow simple compliation  


Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

EFI_STATUS
EFIAPI
SetUsbBootNext(
  VOID
)
{
  return EFI_UNSUPPORTED;
}