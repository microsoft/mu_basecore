/**

NULL implementation for UnitTestBootUsbLib to allow simple compliation  


Copyright (c) Microsoft
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