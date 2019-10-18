# DeviceStateLib

## About

The MsCorePkg provides the necessary functions to store platform specific device
states.  These device states can then be queried by any element within the boot
environment to enable special code paths.  In this library implementation a
bitmask is stored in a PCD to signify what modes are active.

The default bits in the bitmask are set in DeviceStateLib.h - but each platform
is expected to implement its own header to define the platform specific device
states or to define any of the unused bits:

* BIT 0:  DEVICE_STATE_SECUREBOOT_OFF - UEFI Secure Boot disabled
* BIT 1:  DEVICE_STATE_MANUFACTURING_MODE - Device is in an OEM defined
  manufacturing mode
* BIT 2:  DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED - Device is a development
  build.  Non-production features might be enabled
* BIT 3:  DEVICE_STATE_SOURCE_DEBUG_ENABLED - Source debug mode is enabled
  allowing a user to connect and control the device
* BIT 4:  DEVICE_STATE_UNDEFINED - Set by the platform
* BIT 5:  DEVICE_STATE_UNIT_TEST_MODE - Device has a unit test build. Some
  features are disabled to allow for unit tests in UEFI Shell
* BIT 24: DEVICE_STATE_PLATFORM_MODE_0
* BIT 25: DEVICE_STATE_PLATFORM_MODE_1
* BIT 26: DEVICE_STATE_PLATFORM_MODE_2
* BIT 27: DEVICE_STATE_PLATFORM_MODE_3

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
