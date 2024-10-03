/** @file DeviceStateLib.c
Functions used to support Getting and Setting Device States.
This library uses the PcdDeviceStateBitmask dynamic PCD to store the device state

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

MU_CHANGE: new file
**/

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <Library/DeviceStateLib.h>
#include <Library/DebugLib.h>

CHAR8  *DEVICE_STATE_STRINGS[6] = {
  DEVICE_STATE_SECUREBOOT_OFF_STRING,
  DEVICE_STATE_MANUFACTURING_MODE_STRING,
  DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED_STRING,
  DEVICE_STATE_SOURCE_DEBUG_ENABLED_STRING,
  DEVICE_STATE_UNDEFINED_STRING,
  DEVICE_STATE_UNIT_TEST_MODE_STRING,
};

/**
 * Get String that represents the device's current insecure states, using the platform defined set of
 * insecure device states. This can be used to measure insecure device states into the TPM or perform
 * other required platform actions when the device enters an insecure state.
 *
 * @param[out] Buffer - Buffer to store the string
 * @param[in] MaxSize - Maximum size of the buffer
 * @retval The The number of characters in the string, including the null character.
 */
UINTN
EFIAPI
GetInsecureDeviceStateString (
  OUT CHAR8  *Buffer,
  IN UINTN   MaxSize
  )
{
  CHAR8         *String;
  DEVICE_STATE  BitmaskFromIndex;
  DEVICE_STATE  CurrentDeviceState         = GetDeviceState ();
  DEVICE_STATE  InsecureDeviceStateSetting = GetInsecureDeviceStateSetting ();

  UINT32  NumberOfDeviceStates = sizeof (DEVICE_STATE_STRINGS) / sizeof (DEVICE_STATE_STRINGS[0]);

  //
  // Check standard device states
  //
  for (UINT32 i = 0; i < NumberOfDeviceStates; i++) {
    BitmaskFromIndex = 1 << i;
    if ((InsecureDeviceStateSetting & BitmaskFromIndex != 0) &&
        (CurrentDeviceState & BitmaskFromIndex != 0))
    {
      //
      // An insecure device state is detected. Append to the insecure device state string.
      //
      String = DEVICE_STATE_STRINGS[i];
      AsciiStrnCatS (Buffer, MaxSize, String, AsciiStrLen (String));
    }
  }

  //
  // Check OEM-defined device states
  //
  for (UINT32 i = DEVICE_STATE_PLATFORM_MODE_INDEX_START; i <= DEVICE_STATE_PLATFORM_MODE_INDEX_END; i++) {
    BitmaskFromIndex = 1 << i;
    if ((InsecureDeviceStateSetting & BitmaskFromIndex != 0) &&
        (CurrentDeviceState & BitmaskFromIndex != 0))
    {
      //
      // An OEM insecure device state is detected. Append to the insecure device state string.
      //
      String = DEVICE_STATE_PLATFORM_MODE_STRING;
      AsciiStrnCatS (Buffer, MaxSize, String, AsciiStrLen (String));
      break;
    }
  }

  return AsciiStrnLenS (Buffer, MaxSize) + 1;
}

/**
 * Get the platform defined set of insecure device states. This can be used to measure insecure device
 * states into the TPM or perform other required platform actions when the device enters an insecure state.
 *
 * @retval The bitmask of insecure device states as defined by the platform.
 */
DEVICE_STATE
EFIAPI
GetInsecureDeviceStateSetting (
  )
{
  return FixedPcdGet32 (PcdInsecureDeviceState);
}

/**
 * Check if a specific bitmask is set in the device state
 *
 * @param[in] CurrentDeviceState - the current device state
 * @param[in] DeviceStateBitmask - the bitmask to check
 * @retval TRUE if all the bits in the bitmask are set, FALSE otherwise
 */

/**
 * Function to Get current device state
 *
 * @retval the current DEVICE_STATE
 */
DEVICE_STATE
EFIAPI
GetDeviceState (
  )
{
  return PcdGet32 (PcdDeviceStateBitmask);
}

/**
 * Function to Add additional bits to the device state
 *
 * @param[in] AdditionalState - additional state to set active
 * @retval Status of operation.  EFI_SUCCESS on successful update.
 */
RETURN_STATUS
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
    return RETURN_DEVICE_ERROR;
  }

  SetValue = PcdGet32 (PcdDeviceStateBitmask);

  return ((SetValue == DevState) ? RETURN_SUCCESS : RETURN_OUT_OF_RESOURCES);
}
