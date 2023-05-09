/** @file
  Bad example for Google Test.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <PiDxe.h>

#include <Guid/ZeroGuid.h>
#include <Protocol/BootManagerPolicy.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Get policy protocol from our database.

  @retval EFI_SUCCESS       Policy is located successfully.
  @retval EFI_ERROR         Failed to locate policy protocol.
**/
EFI_STATUS
EFIAPI
GetPolicy (
  VOID
  )
{
  EFI_STATUS                        Status;
  EFI_BOOT_MANAGER_POLICY_PROTOCOL  *PolicyProtocol;

  Status = gBS->LocateProtocol (
                  &gEfiBootManagerPolicyProtocolGuid,
                  NULL,
                  (VOID **)&PolicyProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: Failed to locate policy protocol - %r!\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Need to keep this here, so that the false negative will happen.
  Status = PolicyProtocol->ConnectDevicePath (PolicyProtocol, NULL, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to ConnectDevicePath - %r!\n", __FUNCTION__, Status));
    return Status;
  }

  return Status;
}
