/** @file
  DXE Reset System Library instance that calls gRT->ResetSystem().

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/ResetSystemLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  This function causes a system-wide reset (cold reset), in which
  all circuitry within the system returns to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.

  If this function returns, it means that the system does not support cold reset.
**/
VOID
EFIAPI
ResetCold (
  VOID
  )
{
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
}

/**
  This function causes a system-wide initialization (warm reset), in which all processors
  are set to their initial state. Pending cycles are not corrupted.

  If this function returns, it means that the system does not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
  VOID
  )
{
  gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
}

/**
  This function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  If this function returns, it means that the system does not support shut down reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  )
{
  gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
}

/**
  This function causes the system to enter S3 and then wake up immediately.

  If this function returns, it means that the system does not support S3 feature.
**/
VOID
EFIAPI
EnterS3WithImmediateWake (
  VOID
  )
{
}

/**
  This function causes a systemwide reset. The exact type of the reset is
  defined by the EFI_GUID that follows the Null-terminated Unicode string passed
  into ResetData. If the platform does not recognize the EFI_GUID in ResetData
  the platform must pick a supported reset type to perform.The platform may
  optionally log the parameters from any non-normal reset that occurs.

  @param[in]  DataSize   The size, in bytes, of ResetData.
  @param[in]  ResetData  The data buffer starts with a Null-terminated string,
                         followed by the EFI_GUID.
**/
VOID
EFIAPI
ResetPlatformSpecific (
  IN UINTN   DataSize,
  IN VOID    *ResetData
  )
{
  gRT->ResetSystem (EfiResetPlatformSpecific, EFI_SUCCESS, DataSize, ResetData);
}

// MS_CHANGE [BEGIN] - Move EfiResetSystem out of UefiRuntimeLib and into ResetSystemLib.
/**
  This service is a wrapper for the UEFI Runtime Service ResetSystem().

  The ResetSystem()function resets the entire platform, including all processors and devices,and reboots the system.
  Calling this interface with ResetType of EfiResetCold causes a system-wide reset. This sets all circuitry within
  the system to its initial state. This type of reset is asynchronous to system operation and operates without regard
  to cycle boundaries. EfiResetCold is tantamount to a system power cycle.
  Calling this interface with ResetType of EfiResetWarm causes a system-wide initialization. The processors are set to
  their initial state, and pending cycles are not corrupted. If the system does not support this reset type, then an
  EfiResetCold must be performed.
  Calling this interface with ResetType of EfiResetShutdown causes the system to enter a power state equivalent to the
  ACPI G2/S5 or G3 states. If the system does not support this reset type, then when the system is rebooted, it should
  exhibit the EfiResetCold attributes.
  The platform may optionally log the parameters from any non-normal reset that occurs.
  The ResetSystem() function does not return.

  @param  ResetType   The type of reset to perform.
  @param  ResetStatus The status code for the reset. If the system reset is part of a normal operation, the status code
                      would be EFI_SUCCESS. If the system reset is due to some type of failure the most appropriate EFI
                      Status code would be used.
  @param  DataSizeThe size, in bytes, of ResetData.
  @param  ResetData   For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown the data buffer starts with a
                      Null-terminated Unicode string, optionally followed by additional binary data. The string is a
                      description that the caller may use to further indicate the reason for the system reset. ResetData
                      is only valid if ResetStatus is something other then EFI_SUCCESS. This pointer must be a physical
                      address. For a ResetType of EfiResetPlatformSpecific the data buffer also starts with a Null-terminated
                      string that is followed by an EFI_GUID that describes the specific type of reset to perform.
**/
VOID
EFIAPI
EfiResetSystem (
  IN EFI_RESET_TYPE               ResetType,
  IN EFI_STATUS                   ResetStatus,
  IN UINTN                        DataSize,
  IN VOID                         *ResetData OPTIONAL
  )
{
  gRT->ResetSystem (ResetType, ResetStatus, DataSize, ResetData);
}
// MS_CHANGE [END] - Move EfiResetSystem out of UefiRuntimeLib and into ResetSystemLib.
