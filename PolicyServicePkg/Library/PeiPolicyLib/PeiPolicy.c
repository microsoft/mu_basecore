/** @file
  Implementation of the Verified Policy library for PEI.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

#include <Ppi/Policy.h>
#include <Library/PolicyLib.h>

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
  return PeiServicesLocatePpi (
           &gPeiPolicyPpiGuid,
           0,
           NULL,
           (VOID **)PolicyInterface
           );
}
