/** @file
  Implementation of the Verified Policy library for DXE.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

#include <Protocol/Policy.h>
#include <Library/PolicyLib.h>

#include "../PolicyLibCommon.h"

EFI_EVENT  mEfiExitBootServicesEvent;
BOOLEAN    mEfiAtRuntime = FALSE;

/**
  A private helper function to retrieve the policy service protocol.

  @param[out]     PolicyInterface   Returns with the pointer to the protocol.

  @retval         EFI_SUCCESS       Policy protocol was found.
  @retval         EFI_NOT_FOUND     Policy protocol was not found.
  @retval         EFI_UNSUPPORTED   Policy service was called at runtime.
**/
EFI_STATUS
GetPolicyInterface (
  OUT POLICY_INTERFACE  **PolicyInterface
  )
{
  EFI_STATUS              Status;
  STATIC POLICY_PROTOCOL  *mPolicyProtocol = NULL;

  if (mEfiAtRuntime) {
    mPolicyProtocol = NULL;
    return EFI_UNSUPPORTED;
  }

  Status = EFI_SUCCESS;
  if (mPolicyProtocol == NULL) {
    Status = gBS->LocateProtocol (
                    &gPolicyProtocolGuid,
                    NULL,
                    (VOID **)&mPolicyProtocol
                    );

    if (EFI_ERROR (Status)) {
      mPolicyProtocol = NULL;
    }
  }

  if (mPolicyProtocol != NULL) {
    *PolicyInterface = mPolicyProtocol;
  }

  return Status;
}

/**
  Set AtRuntime flag as TRUE after ExitBootServices.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
RuntimeLibExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mEfiAtRuntime = TRUE;
}

/**
  This constructor sets up a callback for ExitBootServices to ensure policy
  service is not used at runtime.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS   Successfully initialized the policy library.
  @retval Other         Error returned by a subroutine.

**/
EFI_STATUS
EFIAPI
PolicyLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_NOTIFY,
                  RuntimeLibExitBootServicesEvent,
                  NULL,
                  &mEfiExitBootServicesEvent
                  );

  ASSERT_EFI_ERROR (Status);
  return Status;
}
