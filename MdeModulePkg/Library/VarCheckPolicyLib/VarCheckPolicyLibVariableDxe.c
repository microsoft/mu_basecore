/** @file -- VarCheckPolicyLib.c
This is a NULL library instance that leverages the VarCheck interface
and the business logic behind the VariablePolicy code to make its decisions.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/VarCheckLib.h>
#include <Library/DebugLib.h>

#include <Library/VariablePolicyLib.h>

// ================================================
// As a VarCheck library, we're linked into the VariableServices
// and may not be able to call them indirectly. To get around this,
// use the internal GetVariable function to query the variable store.
// ================================================
EFI_STATUS
EFIAPI
VariableServiceGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data
  );

/**
  Simple constructor function of VarCheckPolicyLib

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckPolicyLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  // Initialize the business logic with the internal GetVariable handler.
  Status = InitVariablePolicyLib (VariableServiceGetVariable);

  // Only proceed with init if the business logic could be initialized.
  if (!EFI_ERROR (Status)) {
    // Register the VarCheck handler for SetVariable filtering.
    // Forward the check to the business logic of the library.
    VarCheckLibRegisterSetVariableCheckHandler (ValidateSetVariable);
  }
  // Otherwise, there's not much we can do.
  else {
    DEBUG ((DEBUG_ERROR, "%a - Cannot Initialize VariablePolicyLib! %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
