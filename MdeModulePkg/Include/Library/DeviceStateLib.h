/** @file
Functions used to support Getting and Setting Device States.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

MU_CHANGE: new file
**/

#ifndef __DEVICE_STATE_LIB_H__
#define __DEVICE_STATE_LIB_H__

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

typedef UINT32 DEVICE_STATE;

/**
Function to Get current device state
@retval the current DEVICE_STATE
**/
DEVICE_STATE
EFIAPI
GetDeviceState (
  );

/**
Function to Add additional bits to the device state

@param AdditionalState - additional state to set active
@retval Status of operation.  EFI_SUCCESS on successful update.
**/
RETURN_STATUS
EFIAPI
AddDeviceState (
  DEVICE_STATE  AdditionalState
  );

#endif
