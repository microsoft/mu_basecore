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

#define DEVICE_STATE_PLATFORM_MODE_0  (1 << (20))
#define DEVICE_STATE_PLATFORM_MODE_1  (1 << (21))
#define DEVICE_STATE_PLATFORM_MODE_2  (1 << (22))
#define DEVICE_STATE_PLATFORM_MODE_3  (1 << (23))
#define DEVICE_STATE_PLATFORM_MODE_4  (1 << (24))
#define DEVICE_STATE_PLATFORM_MODE_5  (1 << (25))
#define DEVICE_STATE_PLATFORM_MODE_6  (1 << (26))
#define DEVICE_STATE_PLATFORM_MODE_7  (1 << (27))

#define DEVICE_STATE_MAX  (1 << (31))

#define DEVICE_STATE_PLATFORM_MODE_INDEX_START  20
#define DEVICE_STATE_PLATFORM_MODE_INDEX_END    27

#define MAX_INSECURE_DEVICE_STATE_STRING_SIZE  512

//
// DEFINE the device state strings here. These are used for logging and measuring purposes.
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
  IN OUT CHAR8  *Buffer,
  IN UINTN      MaxSize
  );

/**
 * Get the platform defined set of insecure device states. This can be used to measure insecure device
 * states into the TPM or perform other required platform actions when the device enters an insecure state.
 *
 * @retval The bitmask of insecure device states as defined by the platform.
 */
DEVICE_STATE
EFIAPI
GetInsecureDeviceStateSetting (
  );

/**
 * Function to Get current device state
 *
 * @retval the current DEVICE_STATE
 */
DEVICE_STATE
EFIAPI
GetDeviceState (
  );

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
  );

#endif
