/** @file
  Library to get and set the device state

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __DEVICE_STATE_LIB_H__
#define __DEVICE_STATE_LIB_H__

//
// DEFINE the possible device states.
// Current API is defined as a 31-bit bitmask  (The 32nd bit is reserved for MAX)
//
#define DEVICE_STATE_SECUREBOOT_OFF             BIT0
#define DEVICE_STATE_MANUFACTURING_MODE         BIT1
#define DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED  BIT2
#define DEVICE_STATE_SOURCE_DEBUG_ENABLED       BIT3
#define DEVICE_STATE_UNDEFINED                  BIT4

#define DEVICE_STATE_PLATFORM_MODE_0  BIT20
#define DEVICE_STATE_PLATFORM_MODE_1  BIT21
#define DEVICE_STATE_PLATFORM_MODE_2  BIT22
#define DEVICE_STATE_PLATFORM_MODE_3  BIT23
#define DEVICE_STATE_PLATFORM_MODE_4  BIT24
#define DEVICE_STATE_PLATFORM_MODE_5  BIT25
#define DEVICE_STATE_PLATFORM_MODE_6  BIT26
#define DEVICE_STATE_PLATFORM_MODE_7  BIT27

#define DEVICE_STATE_MAX  BIT31

typedef UINT32 DEVICE_STATE;

/**
  Function to get current device state

  @retval the current DEVICE_STATE
**/
DEVICE_STATE
EFIAPI
GetDeviceState (
  VOID
  );

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
  );

#endif
