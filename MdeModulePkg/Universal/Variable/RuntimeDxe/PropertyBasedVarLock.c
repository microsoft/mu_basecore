/** @file -- PropertyBasedVarLock.c
This is an implementation of the EDKII variable lock interface
that leverages variable properties to enforce the lock.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// MS_CHANGE_90369: Separate the logic for VarLock from VarCheck.

#include <Library/DebugLib.h>
#include <Library/VarCheckLib.h>

#include <Protocol/VariableLock.h>

#include "Variable.h"

/**
  Mark a variable that will become read-only after leaving the DXE phase of execution.
  Write request coming from SMM environment through EFI_SMM_VARIABLE_PROTOCOL is allowed.

  @param[in] This          The VARIABLE_LOCK_PROTOCOL instance.
  @param[in] VariableName  A pointer to the variable name that will be made read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and the VendorGuid was marked
                                as pending to be read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock request.
**/
EFI_STATUS
EFIAPI
VariableLockRequestToLock (
  IN CONST EDKII_VARIABLE_LOCK_PROTOCOL *This,
  IN       CHAR16                       *VariableName,
  IN       EFI_GUID                     *VendorGuid
  )
{
  EFI_STATUS                    Status;
  VAR_CHECK_VARIABLE_PROPERTY   Property;

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = VarCheckLibVariablePropertyGet (VariableName, VendorGuid, &Property);
  if (!EFI_ERROR (Status)) {
    Property.Property |= VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY;
  } else {
    Property.Revision = VAR_CHECK_VARIABLE_PROPERTY_REVISION;
    Property.Property = VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY;
    Property.Attributes = 0;
    Property.MinSize = 1;
    Property.MaxSize = MAX_UINTN;
  }
  Status = VarCheckLibVariablePropertySet (VariableName, VendorGuid, &Property);

  DEBUG ((EFI_D_INFO, "[Variable] Lock: %g:%s %r\n", VendorGuid, VariableName, Status));

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  return Status;
}
