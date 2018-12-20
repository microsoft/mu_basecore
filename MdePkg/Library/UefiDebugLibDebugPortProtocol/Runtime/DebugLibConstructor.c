/** @file
  UEFI Runtime DebugLib constructor that gets the pointers to the system table and boot services table
  This is needed to prevent a circular dependency between UefiBootServices lib and DebugLib

  Differs from the DXE constructor in that it needs to register a callback on
  ExitBootServices to close off protocol calls

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

//
// Used to prevent boot services calls on runtime post exit boot servies
//
extern BOOLEAN mPostEBS; 
extern EFI_SYSTEM_TABLE *mST;
extern EFI_BOOT_SERVICES *mBS;
EFI_EVENT mExitBootServicesEvent;

/**
  This routine sets the Boolean for exit boot servies true
  to prevent DebugPort protocol dereferences when the pointer is nulled
**/
VOID
EFIAPI
ExitBootServicesCallback (
  EFI_EVENT Event,
  VOID* Context
  )
{
  mPostEBS = TRUE;
  return;
}

/**
* The constructor gets the pointers to the system table and boot services table
* and sets up the callback routines for runtime to disable debug prints after exit boot services
*
* @param  ImageHandle   The firmware allocated handle for the EFI image.
* @param  SystemTable   A pointer to the EFI System Table.
*
* @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
*
**/
EFI_STATUS
EFIAPI
RuntimeDebugLibConstructor(
    IN      EFI_HANDLE                ImageHandle,
    IN      EFI_SYSTEM_TABLE          *SystemTable
) {
  mST = SystemTable;
  mBS = mST->BootServices;

  mBS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                     TPL_NOTIFY,
                     ExitBootServicesCallback,
                     NULL,
                     &gEfiEventExitBootServicesGuid,
                     &mExitBootServicesEvent);
  
  return EFI_SUCCESS;
}