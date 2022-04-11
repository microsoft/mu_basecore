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
#include <Library/VerifiedPolicy.h>

#include "../PolicyLibCommon.h"

/**
  A private helper function to retrieve the policy service protocol.

  @param[out]     PolicyInterface   Returns with the pointer to the protocol.

  @retval         EFI_SUCCESS       Policy protocol was found.
  @retval         EFI_NOT_FOUND     Policy protocol was not found.
**/
EFI_STATUS
GetPolicyInterface (
  OUT POLICY_INTERFACE  **PolicyInterface
  )
{
  EFI_STATUS              Status;
  STATIC POLICY_PROTOCOL  *mPolicyProtocol = NULL;

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
