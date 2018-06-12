/** @file
  This file include all platform action which can be customized
  by IBV/OEM.

Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PlatformBootManagerLib.h>

/**
  Do the platform specific action before the console is connected.

  Such as:
    Update console variable;
    Register new Driver#### or Boot####;
    Signal ReadyToLock event.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
{
  return;
}

/**
  Do the platform specific action after the console is connected.

  Such as:
    Dynamically switch output mode;
    Signal console ready platform customized event;
    Run diagnostics like memory testing;
    Connect certain devices;
    Dispatch aditional option roms.
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
{
  return;
}

/**
  This function is called each second during the boot manager waits the timeout.

  @param TimeoutRemain  The remaining timeout.
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16  TimeoutRemain
  )
{
  return;
}

/**
  The function is called when no boot option could be launched,
  including platform recovery options and options pointing to applications
  built into firmware volumes.

  If this function returns, BDS attempts to enter an infinite loop.
**/
VOID
EFIAPI
PlatformBootManagerUnableToBoot (
  VOID
  )
{
  return;
}

/**   MSCHANGE begin
   Do Platform specific action required at start of BDS

**/
VOID
EFIAPI
PlatformBootManagerBdsEntry (
  VOID
  )
{
  return;
}

/**
ProcessBootCompletion
*/
VOID
EFIAPI
PlatformBootManagerProcessBootCompletion (
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  return;
}

/**
 HardKeyBoot
*/
VOID
EFIAPI
PlatformBootManagerPriorityBoot (
  UINT16  **BootNext
  )
{
  return;
}

/**
 BDS Deadloop - error, unable to boot any boot option
*/
VOID
EFIAPI
PlatformBootManagerDeadloop (
  VOID
  )
{
  return;
}

/**
  OnDemandConInCOnnect
 */
VOID
EFIAPI
PlatformBootManagerOnDemandConInConnect (
  VOID
  )
{
  return;
}

/**   MSCHANGE end */
