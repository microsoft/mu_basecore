/** @file DeviceStateLib.h
Functions used to support Getting and Setting Device States.

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

MU_CHANGE: new file
**/

#ifndef DEVICE_STATE_LIB_H__
#define DEVICE_STATE_LIB_H__

//
// DEFINE the possible device states here.
// Current API is defined as a 31bit bitmask  (bit 32 is reserved for MAX)
//
#define DEVICE_STATE_SECUREBOOT_OFF             (1 << (0))
#define DEVICE_STATE_MANUFACTURING_MODE         (1 << (1))
#define DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED  (1 << (2))
#define DEVICE_STATE_SOURCE_DEBUG_ENABLED       (1 << (3))
#define DEVICE_STATE_UNDEFINED                  (1 << (4))
#define DEVICE_STATE_UNIT_TEST_MODE             (1 << (5))

#define DEVICE_STATE_PLATFORM_MODE_INDEX_START  20
#define DEVICE_STATE_PLATFORM_MODE_INDEX_END    27

#define DEVICE_STATE_PLATFORM_MODE_0  (1 << (DEVICE_STATE_PLATFORM_MODE_INDEX_START))
#define DEVICE_STATE_PLATFORM_MODE_1  (1 << (21))
#define DEVICE_STATE_PLATFORM_MODE_2  (1 << (22))
#define DEVICE_STATE_PLATFORM_MODE_3  (1 << (23))
#define DEVICE_STATE_PLATFORM_MODE_4  (1 << (24))
#define DEVICE_STATE_PLATFORM_MODE_5  (1 << (25))
#define DEVICE_STATE_PLATFORM_MODE_6  (1 << (26))
#define DEVICE_STATE_PLATFORM_MODE_7  (1 << (DEVICE_STATE_PLATFORM_MODE_INDEX_END))

#define DEVICE_STATE_MAX  (1 << (31))

#define MAX_INSECURE_DEVICE_STATE_STRING_SIZE  512

// The number of library-defined device states, not including OEM-defined states.
#define NUMBER_OF_DEVICE_STATE_STRINGS  6

//
// These strings are used to represent device state for logging and measurements.
//
#define DEVICE_STATE_SECUREBOOT_OFF_STRING             "SECURE_BOOT_OFF "
#define DEVICE_STATE_MANUFACTURING_MODE_STRING         "MANUFACTURING_MODE "
#define DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED_STRING  "DEVELOPMENT_BUILD_ENABLED "
#define DEVICE_STATE_SOURCE_DEBUG_ENABLED_STRING       "SOURCE_DEBUG_ENABLED "
#define DEVICE_STATE_UNDEFINED_STRING                  "UNDEFINED "
#define DEVICE_STATE_UNIT_TEST_MODE_STRING             "UNIT_TEST_MODE "
#define DEVICE_STATE_PLATFORM_MODE_STRING              "OEM_DEFINED "

typedef UINT32 DEVICE_STATE;

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
  );

/**
  Get the platform defined set of insecure device states. This can be used to measure insecure device
  states into the TPM or perform other required platform actions when the device enters an insecure state.

  @return The bitmask of insecure device states as defined by the platform.
**/
DEVICE_STATE
EFIAPI
GetInsecureDeviceStateSetting (
  );

/**
  Get current device state

  @return the current DEVICE_STATE
**/
DEVICE_STATE
EFIAPI
GetDeviceState (
  VOID
  );

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
  );

#endif
