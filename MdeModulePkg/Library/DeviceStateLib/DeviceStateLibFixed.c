/** @file
  Read-only DeviceStateLib for platforms that pin PcdDeviceStateBitmask as
  FixedAtBuild. GetDeviceState returns the compile-time value; AddDeviceState
  is unsupported (Fixed PCDs cannot be updated).

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Library/PcdLib.h>
#include <Library/DeviceStateLib.h>
#include <Library/DebugLib.h>

DEVICE_STATE
EFIAPI
GetDeviceState (
  VOID
  )
{
  return (DEVICE_STATE)FixedPcdGet32 (PcdDeviceStateBitmask);
}

RETURN_STATUS
EFIAPI
AddDeviceState (
  DEVICE_STATE  AdditionalState
  )
{
  (VOID)AdditionalState;
  DEBUG ((DEBUG_WARN, "%a: AddDeviceState is unsupported when PcdDeviceStateBitmask is FixedAtBuild\n", __func__));
  return RETURN_UNSUPPORTED;
}
