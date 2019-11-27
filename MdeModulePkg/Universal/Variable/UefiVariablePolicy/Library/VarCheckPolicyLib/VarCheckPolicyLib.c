/** @file -- VarCheckPolicyLib.c
This is an instance of a VarCheck lib that leverages the business logic behind
the VariablePolicy code to make its decisions.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/VarCheckLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/SafeIntLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/VariablePolicy.h>
#include <Library/UefiVariablePolicyLib.h>

#include <Protocol/VariableLock.h>

#include "../../VarCheckPolicyMmiCommon.h"

//================================================
// As a VarCheck library, we're linked into the VariableServices
// and may not be able to call them indirectly. To get around this,
// use the internal GetVariable function to query the variable store.
//================================================
EFI_STATUS
EFIAPI
VariableServiceGetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data
  );


/**
  MM Communication Handler to recieve commands from the DXE protocol for
  Variable Policies. This communication channel is used to register new policies
  and poll and toggle the enforcement of variable policies.

  @param[in]      DispatchHandle      All parameters standard to MM communications convention.
  @param[in]      RegisterContex      All parameters standard to MM communications convention.
  @param[in,out]  CommBuffer          All parameters standard to MM communications convention.
  @param[in,out]  CommBufferSize      All parameters standard to MM communications convention.

  @retval     EFI_SUCCESS
  @retval     EFI_INVALID_PARAMETER   CommBuffer or CommBufferSize is null pointer.
  @retval     EFI_INVALID_PARAMETER   CommBuffer size is wrong.
  @retval     EFI_INVALID_PARAMETER   Revision or signature don't match.

**/
STATIC
EFI_STATUS
EFIAPI
VarCheckPolicyLibMmiHandler (
  IN     EFI_HANDLE                   DispatchHandle,
  IN     CONST VOID                   *RegisterContext,
  IN OUT VOID                         *CommBuffer,
  IN OUT UINTN                        *CommBufferSize
  )
{
  EFI_STATUS                                Status = EFI_SUCCESS;
  VAR_CHECK_POLICY_COMM_HEADER              *PolicyCommmHeader;
  VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS   *IsEnabledParams;
  // VAR_CHECK_POLICY_COMM_DUMP_PARAMS         *DumpParams;     // Not yet implemented.
  VARIABLE_POLICY_ENTRY                     *PolicyEntry;
  UINTN                                     ExpectedSize;

  // TODO VARPOL: May need speculation barriers to ensure that size boundaries are fully validated before proceeding.

  //
  // Validate some input parameters.
  //
  // If either of the pointers are NULL, we can't proceed.
  if (CommBuffer == NULL || CommBufferSize == NULL) {
    DEBUG(( DEBUG_INFO, "%a - Invalid comm buffer pointers!\n", __FUNCTION__ ));
    return EFI_INVALID_PARAMETER;
  }
  // If the size does not meet a minimum threshold, we cannot proceed.
  ExpectedSize = sizeof(VAR_CHECK_POLICY_COMM_HEADER);
  if (*CommBufferSize < ExpectedSize) {
    DEBUG(( DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __FUNCTION__, *CommBufferSize, ExpectedSize ));
    return EFI_INVALID_PARAMETER;
  }
  // Check the revision and the signature of the comm header.
  PolicyCommmHeader = CommBuffer;
  if (PolicyCommmHeader->Signature != VAR_CHECK_POLICY_COMM_SIG ||
      PolicyCommmHeader->Revision != VAR_CHECK_POLICY_COMM_REVISION) {
    DEBUG(( DEBUG_INFO, "%a - Signature or revision are incorrect!\n", __FUNCTION__ ));
    // We have verified the buffer is not null and have enough size to hold Result field.
    PolicyCommmHeader->Result = EFI_INVALID_PARAMETER;
    return EFI_SUCCESS;
  }

  //
  // Now we can process the command as it was sent.
  //
  PolicyCommmHeader->Result = EFI_ABORTED;    // Set a default return for incomplete commands.
  switch(PolicyCommmHeader->Command) {
    case VAR_CHECK_POLICY_COMMAND_DISABLE:
      // IMPORTANT TODO VARPOL: Should we make any PCR changes if this is disabled?
      //                  Is that something we can do from MM?
      PolicyCommmHeader->Result = DisableVariablePolicy();
      break;

    case VAR_CHECK_POLICY_COMMAND_IS_ENABLED:
      // Make sure that we're dealing with a reasonable size.
      // This add should be safe because these are fixed sizes so far.
      ExpectedSize += sizeof(VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS);
      if (*CommBufferSize < ExpectedSize) {
        DEBUG(( DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __FUNCTION__, *CommBufferSize, ExpectedSize ));
        PolicyCommmHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      // Now that we know we've got a valid size, we can fill in the rest of the data.
      IsEnabledParams = (VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS*)((UINT8*)CommBuffer + sizeof(VAR_CHECK_POLICY_COMM_HEADER));
      IsEnabledParams->State = IsVariablePolicyEnabled();
      PolicyCommmHeader->Result = EFI_SUCCESS;
      break;

    case VAR_CHECK_POLICY_COMMAND_REGISTER:
      // Make sure that we're dealing with a reasonable size.
      // This add should be safe because these are fixed sizes so far.
      ExpectedSize += sizeof(VARIABLE_POLICY_ENTRY);
      if (*CommBufferSize < ExpectedSize) {
        DEBUG(( DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __FUNCTION__, *CommBufferSize, ExpectedSize ));
        PolicyCommmHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      // At the very least, we can assume that we're working with a valid policy entry.
      // Time to compare its internal size.
      PolicyEntry = (VARIABLE_POLICY_ENTRY*)((UINT8*)CommBuffer + sizeof(VAR_CHECK_POLICY_COMM_HEADER));
      if (PolicyEntry->Version != VARIABLE_POLICY_ENTRY_REVISION ||
          PolicyEntry->Size < sizeof(VARIABLE_POLICY_ENTRY) ||
          EFI_ERROR(SafeUintnAdd(sizeof(VAR_CHECK_POLICY_COMM_HEADER), PolicyEntry->Size, &ExpectedSize)) ||
          *CommBufferSize < ExpectedSize) {
        DEBUG(( DEBUG_INFO, "%a - Bad policy entry contents!\n", __FUNCTION__ ));
        PolicyCommmHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      PolicyCommmHeader->Result = RegisterVariablePolicy( PolicyEntry );
      break;

    case VAR_CHECK_POLICY_COMMAND_DUMP:
      // TODO VARPOL: Figure out the complex bits around pagination for the comm buffer.
      // For now, let's say this isn't supported.
      PolicyCommmHeader->Result = EFI_UNSUPPORTED;
      break;

    case VAR_CHECK_POLICY_COMMAND_LOCK:
      PolicyCommmHeader->Result = LockVariablePolicy();
      break;

    default:
      // Mark unknown requested command as EFI_UNSUPPORTED.
      DEBUG(( DEBUG_INFO, "%a - Invalid command requested! %d\n", __FUNCTION__, PolicyCommmHeader->Command ));
      PolicyCommmHeader->Result = EFI_UNSUPPORTED;
      break;
  }

  DEBUG(( DEBUG_VERBOSE, "%a - Command %d returning %r.\n", __FUNCTION__,
          PolicyCommmHeader->Command, PolicyCommmHeader->Result ));

  return Status;
} // VarCheckPolicyLibMmiHandler()


/**
  Constructor function of VarCheckPolicyLib to register VarCheck handler and
  SW MMI handlers.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckPolicyLibConstructor (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    DiscardedHandle;

  // Initialize the business logic with the internal GetVariable handler.
  Status = InitVariablePolicyLib( VariableServiceGetVariable );

  // Only proceed with init if the business logic could be initialized.
  if (!EFI_ERROR( Status )) {
    // TODO: [Optional] Pre-populate the library with any embedded policies.
    // TODO: [Optional] Create (and test) a new "non-serial policy store" that can be used to separate
    //                  repeated data (like GUIDs and strings) and variable-length data (like strings)
    //                  from the well-structured data. This might also be a good way to pre-define the
    //                  tables at build-time.

    // Register the VarCheck handler for SetVariable filtering.
    // Forward the check to the business logic of the library.
    VarCheckLibRegisterSetVariableCheckHandler( ValidateSetVariable );

    // Register the MMI handlers for receiving policy commands.
    DiscardedHandle = NULL;
    Status = gMmst->MmiHandlerRegister( VarCheckPolicyLibMmiHandler,
                                        &gVarCheckPolicyLibMmiHandlerGuid,
                                        &DiscardedHandle );
  }
  // Otherwise, there's not much we can do.
  else {
    DEBUG(( DEBUG_ERROR, "%a - Cannot Initialize VariablePolicyLib! %r\n", __FUNCTION__, Status ));
    ASSERT_EFI_ERROR( Status );
    // TODO VARPOL: Telemetry!
  }

  // IMPORTANT TODO VARPOL: Should we return Status if the init failed?
  //                 This might prevent VariableServices from being installed which may
  //                 brick the system. The alternative would be that we don't have variable policies.
  return Status;
} // VarCheckPolicyLibConstructor()
