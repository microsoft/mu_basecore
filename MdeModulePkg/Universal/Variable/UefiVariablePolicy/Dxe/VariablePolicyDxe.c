/** @file -- VariablePolicyDxe.c
This protocol allows communication with Variable Policy Engine.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/SafeIntLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/VariablePolicy.h>
#include <Protocol/MmCommunication.h>

#include <Guid/PiSmmCommunicationRegionTable.h>

#include "../VarCheckPolicyMmiCommon.h"

VARIABLE_POLICY_PROTOCOL        mVariablePolicyProtocol;
EFI_MM_COMMUNICATION_PROTOCOL   *mMmCommunication;

VOID    *mMmCommunicationBuffer;
UINTN   mMmCommunicationBufferSize;


/**
  This API function disables the variable policy enforcement. If it's
  already been called once, will return EFI_ALREADY_STARTED.

  @retval     EFI_SUCCESS
  @retval     EFI_ALREADY_STARTED   Has already been called once this boot.
  @retval     EFI_WRITE_PROTECTED   Interface has been locked until reboot.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolDisableVariablePolicy (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_MM_COMMUNICATE_HEADER     *CommHeader;
  VAR_CHECK_POLICY_COMM_HEADER  *PolicyHeader;
  UINTN                         BufferSize;

  // Set up the MM communication.
  BufferSize    = mMmCommunicationBufferSize;
  CommHeader    = mMmCommunicationBuffer;
  PolicyHeader  = (VAR_CHECK_POLICY_COMM_HEADER*)&CommHeader->Data;
  CopyGuid( &CommHeader->HeaderGuid, &gVarCheckPolicyLibMmiHandlerGuid );
  CommHeader->MessageLength = BufferSize;
  PolicyHeader->Signature   = VAR_CHECK_POLICY_COMM_SIG;
  PolicyHeader->Revision    = VAR_CHECK_POLICY_COMM_REVISION;
  PolicyHeader->Command     = VAR_CHECK_POLICY_COMMAND_DISABLE;

  Status = mMmCommunication->Communicate( mMmCommunication, CommHeader, &BufferSize );
  DEBUG(( DEBUG_VERBOSE, "%a - MmCommunication returned %r.\n", __FUNCTION__, Status ));

  return (EFI_ERROR( Status )) ? Status : PolicyHeader->Result;
} // ProtocolDisableVariablePolicy()


/**
  This API function returns whether or not the policy engine is
  currently being enforced.

  @param[out]   State       Pointer to a return value for whether the policy enforcement
                            is currently enabled.

  @retval     EFI_SUCCESS
  @retval     Others        An error has prevented this command from completing.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolIsVariablePolicyEnabled (
  OUT BOOLEAN     *State
  )
{
  EFI_STATUS                                Status;
  EFI_MM_COMMUNICATE_HEADER                 *CommHeader;
  VAR_CHECK_POLICY_COMM_HEADER              *PolicyHeader;
  VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS   *CommandParams;
  UINTN                                     BufferSize;

  // Set up the MM communication.
  BufferSize    = mMmCommunicationBufferSize;
  CommHeader    = mMmCommunicationBuffer;
  PolicyHeader  = (VAR_CHECK_POLICY_COMM_HEADER*)&CommHeader->Data;
  CommandParams = (VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS*)(PolicyHeader + 1);
  CopyGuid( &CommHeader->HeaderGuid, &gVarCheckPolicyLibMmiHandlerGuid );
  CommHeader->MessageLength = BufferSize;
  PolicyHeader->Signature   = VAR_CHECK_POLICY_COMM_SIG;
  PolicyHeader->Revision    = VAR_CHECK_POLICY_COMM_REVISION;
  PolicyHeader->Command     = VAR_CHECK_POLICY_COMMAND_IS_ENABLED;

  Status = mMmCommunication->Communicate( mMmCommunication, CommHeader, &BufferSize );
  DEBUG(( DEBUG_VERBOSE, "%a - MmCommunication returned %r.\n", __FUNCTION__, Status ));

  if (!EFI_ERROR( Status )) {
    Status = PolicyHeader->Result;
    *State = CommandParams->State;
  }

  return Status;
}


/**
  This API function validates and registers a new policy with
  the policy enforcement engine.

  @param[in]  NewPolicy     Pointer to the incoming policy structure.

  @retval     EFI_SUCCESS
  @retval     EFI_INVALID_PARAMETER   NewPolicy is NULL or is internally inconsistent.
  @retval     EFI_ALREADY_STARTED     An identical matching policy already exists.
  @retval     EFI_WRITE_PROTECTED     The interface has been locked until the next reboot.
  @retval     EFI_UNSUPPORTED         Policy enforcement has been disabled. No reason to add more policies.
  @retval     EFI_ABORTED             A calculation error has prevented this function from completing.
  @retval     EFI_OUT_OF_RESOURCES    Cannot grow the table to hold any more policies.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolRegisterVariablePolicy (
  IN VARIABLE_POLICY_ENTRY    *NewPolicy
  )
{
  EFI_STATUS                                Status;
  EFI_MM_COMMUNICATE_HEADER                 *CommHeader;
  VAR_CHECK_POLICY_COMM_HEADER              *PolicyHeader;
  VOID                                      *PolicyBuffer;
  UINTN                                     BufferSize;
  UINTN                                     RequiredSize;

  // First, make sure that the required size does not exceed the capabilities
  // of the MmCommunication buffer.
  RequiredSize = OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data) + sizeof(VAR_CHECK_POLICY_COMM_HEADER);
  Status = SafeUintnAdd( RequiredSize, NewPolicy->Size, &RequiredSize );
  if (EFI_ERROR( Status ) || RequiredSize > mMmCommunicationBufferSize) {
    DEBUG(( DEBUG_ERROR, "%a - Policy too large for buffer! %r, %d > %d \n", __FUNCTION__,
            Status, RequiredSize, mMmCommunicationBufferSize ));
    return EFI_OUT_OF_RESOURCES;
  }

  // Set up the MM communication.
  BufferSize    = mMmCommunicationBufferSize;
  CommHeader    = mMmCommunicationBuffer;
  PolicyHeader  = (VAR_CHECK_POLICY_COMM_HEADER*)&CommHeader->Data;
  PolicyBuffer  = (VOID*)(PolicyHeader + 1);
  CopyGuid( &CommHeader->HeaderGuid, &gVarCheckPolicyLibMmiHandlerGuid );
  CommHeader->MessageLength = BufferSize;
  PolicyHeader->Signature   = VAR_CHECK_POLICY_COMM_SIG;
  PolicyHeader->Revision    = VAR_CHECK_POLICY_COMM_REVISION;
  PolicyHeader->Command     = VAR_CHECK_POLICY_COMMAND_REGISTER;

  // Copy the policy into place. This copy is safe because we've already tested above.
  CopyMem( PolicyBuffer, NewPolicy, NewPolicy->Size );

  Status = mMmCommunication->Communicate( mMmCommunication, CommHeader, &BufferSize );
  DEBUG(( DEBUG_VERBOSE, "%a - MmCommunication returned %r.\n", __FUNCTION__, Status ));

  return (EFI_ERROR( Status )) ? Status : PolicyHeader->Result;
}


/**
  This API function will dump the entire contents of the variable policy table.

  Similar to GetVariable, the first call can be made with a 0 size and it will return
  the size of the buffer required to hold the entire table.

  @param[out]     Policy  Pointer to the policy buffer. Can be NULL if Size is 0.
  @param[in,out]  Size    On input, the size of the output buffer. On output, the size
                          of the data returned.

  @retval     EFI_SUCCESS             Policy data is in the output buffer and Size has been updated.
  @retval     EFI_INVALID_PARAMETER   Size is NULL, or Size is non-zero and Policy is NULL.
  @retval     EFI_BUFFER_TOO_SMALL    Size is insufficient to hold policy. Size updated with required size.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolDumpVariablePolicy (
  IN OUT UINT8          *Policy,
  IN OUT UINT32         *Size
  )
{
  // TODO VARPOL: Figure out the complex bits around pagination for the comm buffer.
  // For now, let's say this isn't supported.
  return EFI_UNSUPPORTED;
}


/**
  This API function locks the interface so that no more policy updates
  can be performed or changes made to the enforcement until the next boot.

  @retval     EFI_SUCCESS
  @retval     Others        An error has prevented this command from completing.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolLockVariablePolicy (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_MM_COMMUNICATE_HEADER     *CommHeader;
  VAR_CHECK_POLICY_COMM_HEADER  *PolicyHeader;
  UINTN                         BufferSize;

  // Set up the MM communication.
  BufferSize    = mMmCommunicationBufferSize;
  CommHeader    = mMmCommunicationBuffer;
  PolicyHeader  = (VAR_CHECK_POLICY_COMM_HEADER*)&CommHeader->Data;
  CopyGuid( &CommHeader->HeaderGuid, &gVarCheckPolicyLibMmiHandlerGuid );
  CommHeader->MessageLength = BufferSize;
  PolicyHeader->Signature   = VAR_CHECK_POLICY_COMM_SIG;
  PolicyHeader->Revision    = VAR_CHECK_POLICY_COMM_REVISION;
  PolicyHeader->Command     = VAR_CHECK_POLICY_COMMAND_LOCK;

  Status = mMmCommunication->Communicate( mMmCommunication, CommHeader, &BufferSize );
  DEBUG(( DEBUG_VERBOSE, "%a - MmCommunication returned %r.\n", __FUNCTION__, Status ));

  return (EFI_ERROR( Status )) ? Status : PolicyHeader->Result;
}


/**
  This helper function locates the shared comm buffer and assigns it to input pointers.

  @param[in,out]  BufferSize      On input, the minimum buffer size required INCLUDING the MM communicate header.
                                  On output, the size of the matching buffer found.
  @param[out]     LocatedBuffer   A pointer to the matching buffer.

  @retval     EFI_SUCCESS
  @retval     Others        An error has prevented this command from completing.

**/
STATIC
EFI_STATUS
LocateMmCommonCommBuffer (
  IN OUT  UINTN       *BufferSize,
  OUT     VOID        **LocatedBuffer
  )
{
  EFI_STATUS                  Status = EFI_ABORTED;
  UINTN                       Index;
  UINTN                       CurrentRegionSize;
  EFI_MEMORY_DESCRIPTOR       *SmmCommMemRegion;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE   *PiSmmCommunicationRegionTable;

  // Make sure that we're working with good pointers.
  if (BufferSize == NULL || LocatedBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EfiGetSystemConfigurationTable( &gEdkiiPiSmmCommunicationRegionTableGuid, (VOID**)&PiSmmCommunicationRegionTable );
  if (EFI_ERROR( Status )) {
    DEBUG((DEBUG_ERROR, "%a - Failed to get system configuration table! %r\n", __FUNCTION__, Status));
    return Status;
  }

  // Walk through each of the entries trying to find one that will work for the target size.
  Status = EFI_OUT_OF_RESOURCES;
  CurrentRegionSize = 0;
  SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)(PiSmmCommunicationRegionTable + 1);
  for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
    if (SmmCommMemRegion->Type == EfiConventionalMemory) {
      CurrentRegionSize = EFI_PAGES_TO_SIZE((UINTN)SmmCommMemRegion->NumberOfPages);
      if (CurrentRegionSize >= *BufferSize) {
        Status = EFI_SUCCESS;
        break;
      }
    }
    SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)SmmCommMemRegion + PiSmmCommunicationRegionTable->DescriptorSize);
  }

  if (!EFI_ERROR( Status )) {
    *LocatedBuffer = (VOID*)(UINTN)SmmCommMemRegion->PhysicalStart;
    *BufferSize = CurrentRegionSize;
  }
  else {
    *LocatedBuffer = NULL;
    *BufferSize = 0;
  }

  return Status;
} // LocateMmCommonCommBuffer()


/**
  This helper is responsible for telemetry and any other actions that
  need to be taken if the VariablePolicy fails to lock.

  NOTE: It's possible that parts of this handling will need to become
        part of a platform policy.

  @param[in]  FailureStatus   The failure that was reported by LockVariablePolicy

**/
STATIC
VOID
VariablePolicyHandleFailureToLock (
  IN  EFI_STATUS      FailureStatus
  )
{
  // TODO VARPOL: Telemetry and reporting if necessary.
  // TODO VARPOL: Should we force a reboot or something if the interface fails to lock?
  //              Would need to account for: already locked and disabled.
  return;
} // VariablePolicyHandleFailureToLock()


/**
  ReadyToBoot Callback
  Lock the VariablePolicy interface if it hasn't already been locked.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
STATIC
VOID
EFIAPI
LockPolicyInterfaceAtReadyToBoot (
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS  Status;

  Status = ProtocolLockVariablePolicy();

  if (EFI_ERROR( Status )) {
    VariablePolicyHandleFailureToLock( Status );
  }
  else {
    gBS->CloseEvent( Event );
  }

} // LockPolicyInterfaceAtReadyToBoot()


/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
VariablePolicyDxeMain (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS              Status = EFI_SUCCESS;
  BOOLEAN                 ProtocolInstalled = FALSE;
  BOOLEAN                 CallbackRegistered = FALSE;
  EFI_EVENT               ReadyToBootEvent;

  // Update the minimum buffer size.
  mMmCommunicationBufferSize = VAR_CHECK_POLICY_MIN_MM_BUFFER_SIZE;
  // Locate the shared comm buffer to use for sending MM commands.
  Status = LocateMmCommonCommBuffer( &mMmCommunicationBufferSize, &mMmCommunicationBuffer );
  if (EFI_ERROR( Status )) {
    // TODO VARPOL: Telemetry.
    DEBUG((DEBUG_ERROR, "%a - Failed to locate a viable MM comm buffer! %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR( Status );
    return Status;
  }

  // Locate the MmCommunication protocol.
  Status = gBS->LocateProtocol( &gEfiMmCommunicationProtocolGuid, NULL, (VOID**)&mMmCommunication );
  if (EFI_ERROR( Status )) {
    DEBUG((DEBUG_ERROR, "%a - Failed to locate MmCommunication protocol! %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR( Status );
    return Status;
  }

  // Configure the VariablePolicy protocol structure.
  mVariablePolicyProtocol.Revision                = VARIABLE_POLICY_PROTOCOL_REVISION;
  mVariablePolicyProtocol.DisableVariablePolicy   = ProtocolDisableVariablePolicy;
  mVariablePolicyProtocol.IsVariablePolicyEnabled = ProtocolIsVariablePolicyEnabled;
  mVariablePolicyProtocol.RegisterVariablePolicy  = ProtocolRegisterVariablePolicy;
  mVariablePolicyProtocol.DumpVariablePolicy      = ProtocolDumpVariablePolicy;
  mVariablePolicyProtocol.LockVariablePolicy      = ProtocolLockVariablePolicy;

  // Register all the protocols and return the status.
  Status = gBS->InstallMultipleProtocolInterfaces( &ImageHandle,
                                                   &gVariablePolicyProtocolGuid, &mVariablePolicyProtocol,
                                                   NULL );
  if (EFI_ERROR( Status )) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to install protocol! %r\n", __FUNCTION__, Status ));
    goto Exit;
  }
  else {
    ProtocolInstalled = TRUE;
  }

  //
  // Register a callback for ReadyToBoot so that the interface is at least locked before
  // dispatching any bootloaders or UEFI apps.
  Status = gBS->CreateEventEx( EVT_NOTIFY_SIGNAL,
                               TPL_CALLBACK,
                               LockPolicyInterfaceAtReadyToBoot,
                               NULL,
                               &gEfiEventReadyToBootGuid,
                               &ReadyToBootEvent );
  if (EFI_ERROR( Status )) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to create ReadyToBoot event! %r\n", __FUNCTION__, Status ));
    goto Exit;
  }
  else {
    CallbackRegistered = TRUE;
  }

Exit:
  //
  // If we're about to return a failed status (and unload this driver), we must first undo anything that
  // has been successfully done.
  if (EFI_ERROR( Status )) {
    if (ProtocolInstalled) {
      gBS->UninstallProtocolInterface( &ImageHandle, &gVariablePolicyProtocolGuid, &mVariablePolicyProtocol );
    }
    if (CallbackRegistered) {
      gBS->CloseEvent( ReadyToBootEvent );
    }
  }

  return Status;
} // VariablePolicyDxeMain()
