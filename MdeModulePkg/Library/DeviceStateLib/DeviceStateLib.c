/** @file
  Library to get and set the device state

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <Library/DeviceStateLib.h>
#include <Library/DebugLib.h>

/**
  Function to get current device state

  @retval the current DEVICE_STATE
**/
DEVICE_STATE
EFIAPI
GetDeviceState (
  VOID
  )
{
  return (DEVICE_STATE)PcdGet32 (PcdDeviceStateBitmask);
}

/**
  Function to add additional bits to the device state

  @param[in] AdditionalState    State to set active

  @retval EFI_SUCCESS           Update was successful
  @retval EFI_DEVICE_ERROR      Error updating device state
  @retval EFI_OUT_OF_RESOURCES  Unable to set device state
**/
EFI_STATUS
EFIAPI
AddDeviceState (
  IN DEVICE_STATE  AdditionalState
  )
{
  DEVICE_STATE  DevState;
  DEVICE_STATE  SetValue;
  EFI_STATUS    Status;

  DevState = PcdGet32 (PcdDeviceStateBitmask);

  DEBUG ((DEBUG_INFO, "Adding Device State.  0x%X\n", AdditionalState));

  DevState |= AdditionalState;
  Status    = PcdSet32S (PcdDeviceStateBitmask, DevState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error setting device state - %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  SetValue = PcdGet32 (PcdDeviceStateBitmask);

  return ((SetValue == DevState) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES);
}
