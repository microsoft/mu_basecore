/** @file DeviceStateLib.c
Functions used to support Getting and Setting Device States.
This library uses the PcdDeviceStateBitmask dynamic PCD to store the device state.
It also uses the PcdInsecureDeviceState fixed PCD to retrieve the platform defined set of insecure device states.

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

MU_CHANGE: new file
**/

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <Library/DeviceStateLib.h>
#include <Library/DebugLib.h>

CHAR8  *DEVICE_STATE_STRINGS[NUMBER_OF_DEVICE_STATE_STRINGS] = {
  DEVICE_STATE_SECUREBOOT_OFF_STRING,
  DEVICE_STATE_MANUFACTURING_MODE_STRING,
  DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED_STRING,
  DEVICE_STATE_SOURCE_DEBUG_ENABLED_STRING,
  DEVICE_STATE_UNDEFINED_STRING,
  DEVICE_STATE_UNIT_TEST_MODE_STRING,
};

/**
  Get String that represents the device's current insecure states, using the platform defined set of
  insecure device states. This can be used to measure insecure device states into the TPM or perform
  other required platform actions when the device enters an insecure state.

  The string convention to be returned is described as follows:
  A concatenation of all the insecure device states that are currently active, represented in
  MACRO_CASE, separated by a space. The string, if not empty, will end in a space followed by a Null terminator.

  Examples:
    - "SECUREBOOT_OFF MANUFACTURING_MODE "
    - "DEVELOPMENT_BUILD_ENABLED SOURCE_DEBUG_ENABLED UNIT_TEST_MODE "

  @param[out] Destination Buffer to write the string
  @param[in]  DestMax     Maximum size of the destination buffer
  @param[out] StringSize  Size of the Null-terminated Ascii string in bytes, including the Null terminator.

  @retval EFI_SUCCESS            String is written to buffer.
  @retval EFI_BUFFER_TOO_SMALL   If DestMax is NOT greater than StrLen to be written.
  @retval EFI_INVALID_PARAMETER  If Destination is NULL.
                                    If StringSize is NULL.
                                    If DestMax is 0.
                                    If the bitmask is out of range.
 **/
EFI_STATUS
EFIAPI
GetInsecureDeviceStateString (
  IN OUT CHAR8  *Destination,
  IN UINTN      DestMax,
  IN OUT UINTN  *StringSize
  )
{
  EFI_STATUS    Status;
  CHAR8         *String;
  UINTN         Index;
  DEVICE_STATE  BitmaskFromIndex;
  DEVICE_STATE  CurrentDeviceState         = GetDeviceState ();
  DEVICE_STATE  InsecureDeviceStateSetting = GetInsecureDeviceStateSetting ();

  if ((Destination == NULL) || (DestMax == 0) || (StringSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check standard device states
  //
  for (Index = 0; Index < NUMBER_OF_DEVICE_STATE_STRINGS; Index++) {
    BitmaskFromIndex = 1 << Index;
    if (BitmaskFromIndex > DEVICE_STATE_MAX) {
      // TODO shouldn't this be known from the macro max ?
      DEBUG ((DEBUG_ERROR, "Device State Bitmask is out of range. 0x%X\n", BitmaskFromIndex));
      return EFI_INVALID_PARAMETER;
    }

    if (((InsecureDeviceStateSetting & BitmaskFromIndex) != 0) &&
        ((CurrentDeviceState & BitmaskFromIndex) != 0))
    {
      //
      // An insecure device state is detected. Append to the insecure device state string.
      //
      String = DEVICE_STATE_STRINGS[Index];

      Status = AsciiStrnCatS (Destination, DestMax, String, AsciiStrnLenS (String, DestMax));
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Error creating insecure device state string. 0x%X\n", Status));
        return Status;
      }
    }
  }

  //
  // Check OEM-defined device states
  //
  for (Index = DEVICE_STATE_PLATFORM_MODE_INDEX_START; Index <= DEVICE_STATE_PLATFORM_MODE_INDEX_END; Index++) {
    BitmaskFromIndex = 1 << Index;
    if (((InsecureDeviceStateSetting & BitmaskFromIndex) != 0) &&
        ((CurrentDeviceState & BitmaskFromIndex) != 0))
    {
      //
      // An OEM insecure device state is detected. Append to the insecure device state string.
      //
      String = DEVICE_STATE_PLATFORM_MODE_STRING;
      Status = AsciiStrnCatS (Destination, DestMax, String, AsciiStrnLenS (String, DestMax));
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Error creating insecure device state string. 0x%X\n", Status));
        return Status;
      }

      break;
    }
  }

  *StringSize = AsciiStrnSizeS (Destination, DestMax);
  return EFI_SUCCESS;
}

/**
  Get the platform defined set of insecure device states. This can be used to measure insecure device
  states into the TPM or perform other required platform actions when the device enters an insecure state.

  @return The bitmask of insecure device states as defined by the platform.
**/
DEVICE_STATE
EFIAPI
GetInsecureDeviceStateSetting (
  )
{
  return FixedPcdGet32 (PcdInsecureDeviceState);
}

/**
  Get current device state

  @return the current DEVICE_STATE
**/
DEVICE_STATE
EFIAPI
GetDeviceState (
  VOID
  )
{
  return PcdGet32 (PcdDeviceStateBitmask);
}

/**
  Add an additional device state

  @param[in] AdditionalState  The additional state to add.

  @retval EFI_SUCCESS             The device state was added successfully.
  @retval RETURN_OUT_OF_RESOURCES The device state could not be added due to lack of resources.
  @retval RETURN_DEVICE_ERROR     The device state could not be added.
**/
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
