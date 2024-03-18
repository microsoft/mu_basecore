/** @file
  This driver implements Firmware Volume Block (FVB) Variable Storage Services
  and installs a SMM Variable Storage Protocol instance for FVB storage.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FvbVariableStorage.h"

#include <Protocol/SmmVariable.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <Protocol/SmmFaultTolerantWrite.h>

#include <Library/MmServicesTableLib.h>

static BOOLEAN  mWriteServiceReady                = FALSE;
static BOOLEAN  mVssNotifyWriteServiceReadyCalled = FALSE;

/**
  Calls GetVariable () on the core UEFI variable implementation

  @param[in]      VariableName      Name of Variable to be found.
  @param[in]      VendorGuid        Variable vendor GUID.
  @param[out]     Attributes        Attribute value of the variable found.
  @param[in, out] DataSize          Size of Data found. If size is less than the data, this value contains
                                    the required size.
  @param[out]     Data              Data pointer.

  @return EFI_SUCCESS               Found the specified variable.
  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
CoreGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data
  )
{
  EFI_STATUS                 Status;
  EFI_SMM_VARIABLE_PROTOCOL  *SmmVariableProtocol;

  Status = gMmst->MmLocateProtocol (
                    &gEfiSmmVariableProtocolGuid,
                    NULL,
                    (VOID **)&SmmVariableProtocol
                    );
  if (!EFI_ERROR (Status)) {
    Status = SmmVariableProtocol->SmmGetVariable (
                                    VariableName,
                                    VendorGuid,
                                    Attributes,
                                    DataSize,
                                    Data
                                    );
  } else {
    DEBUG ((DEBUG_WARN, "CoreGetVariable: Error getting EFI_SMM_VARIABLE_PROTOCOL: %r\n", Status));
  }

  return Status;
}

/**
  Calls SetVariable () on the core UEFI variable implementation

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.
  @param[in] Attributes                   Attribute value of the variable found
  @param[in] DataSize                     Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in] Data                         Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
CoreSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  EFI_STATUS                 Status;
  EFI_SMM_VARIABLE_PROTOCOL  *SmmVariableProtocol;

  Status = gMmst->MmLocateProtocol (
                    &gEfiSmmVariableProtocolGuid,
                    NULL,
                    (VOID **)&SmmVariableProtocol
                    );
  if (!EFI_ERROR (Status)) {
    Status = SmmVariableProtocol->SmmSetVariable (
                                    VariableName,
                                    VendorGuid,
                                    Attributes,
                                    DataSize,
                                    Data
                                    );
  } else {
    DEBUG ((DEBUG_WARN, "CoreSetVariable: Error getting EFI_SMM_VARIABLE_PROTOCOL: %r\n", Status));
  }

  return Status;
}

/**
  Retrieve the SMM Fault Tolerent Write protocol interface.

  @param[out] FtwProtocol       The interface of SMM FTW protocol

  @retval EFI_SUCCESS           The SMM FTW protocol instance was found and returned in FtwProtocol.
  @retval EFI_NOT_FOUND         The SMM FTW protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
GetFtwProtocol (
  OUT VOID  **FtwProtocol
  )
{
  EFI_STATUS  Status;

  //
  // Locate SMM Fault Tolerent Write protocol
  //
  Status = gMmst->MmLocateProtocol (
                    &gEfiSmmFaultTolerantWriteProtocolGuid,
                    NULL,
                    FtwProtocol
                    );
  return Status;
}

/**
  Retrieve the SMM FVB protocol interface by handle.

  @param[in]  FvBlockHandle     The handle of SMM FVB protocol that provides services for reading, writing, and
                                erasing the target block.
  @param[out] FvBlock           The interface of SMM FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the SMM FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.

**/
EFI_STATUS
GetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
{
  //
  // To get the SMM FVB protocol interface on the handle
  //
  return gMmst->MmHandleProtocol (
                  FvBlockHandle,
                  &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                  (VOID **)FvBlock
                  );
}

/**
  Function returns an array of handles that support the SMM FVB protocol in a buffer allocated from pool.

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to the buffer to return the requested
                                array of  handles that support SMM FVB protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer, and the number of
                                handles in Buffer was returned in NumberHandles.
  @retval EFI_NOT_FOUND         No SMM FVB handle was found.
  @retval EFI_OUT_OF_RESOURCES  There is not enough pool memory to store the matching results.
  @retval EFI_INVALID_PARAMETER NumberHandles is NULL or Buffer is NULL.

**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN       *NumberHandles,
  OUT EFI_HANDLE  **Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  if ((NumberHandles == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize     = 0;
  *NumberHandles = 0;
  *Buffer        = NULL;
  Status         = gMmst->MmLocateHandle (
                            ByProtocol,
                            &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                            NULL,
                            &BufferSize,
                            *Buffer
                            );
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    return EFI_NOT_FOUND;
  }

  *Buffer = AllocatePool (BufferSize);
  if (*Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gMmst->MmLocateHandle (
                    ByProtocol,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    NULL,
                    &BufferSize,
                    *Buffer
                    );

  *NumberHandles = BufferSize / sizeof (EFI_HANDLE);
  if (EFI_ERROR (Status)) {
    *NumberHandles = 0;
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return Status;
}

/**
  SMM Fault Tolerant Write protocol notification event handler.

  Non-Volatile variable write may needs FTW protocol to reclaim when
  writting variable.

  @param[in]  Protocol   Points to the protocol's unique identifier
  @param[in]  Interface  Points to the interface instance
  @param[in]  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS   The notification function ran successfully
  @retval EFI_NOT_FOUND The FVB protocol for variable is not found.

 **/
EFI_STATUS
EFIAPI
SmmFtwNotificationEvent (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                              Status;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvbProtocol;
  EFI_SMM_FAULT_TOLERANT_WRITE_PROTOCOL   *FtwProtocol;
  EFI_PHYSICAL_ADDRESS                    NvStorageVariableBase;
  UINT32                                  NvStorageVariableSize;
  UINT64                                  NvStorageVariableSize64;
  UINTN                                   FtwMaxBlockSize;

  if (mFvbInstance != NULL) {
    return EFI_SUCCESS;
  }

  //
  // Ensure SMM FTW protocol is installed.
  //
  Status = GetFtwProtocol ((VOID **)&FtwProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find the proper FVB protocol for variable.
  //
  Status = GetVariableFlashNvStorageInfo (&NvStorageVariableBase, &NvStorageVariableSize64);
  ASSERT_EFI_ERROR (Status);

  Status = SafeUint64ToUint32 (NvStorageVariableSize64, &NvStorageVariableSize);
  // This driver currently assumes the size will be UINT32 so assert the value is safe for now.
  ASSERT_EFI_ERROR (Status);

  Status = FtwProtocol->GetMaxBlockSize (FtwProtocol, &FtwMaxBlockSize);
  if (!EFI_ERROR (Status)) {
    ASSERT (NvStorageVariableSize <= FtwMaxBlockSize);
  }

  Status = GetFvbInfoByAddress (NvStorageVariableBase, NULL, &FvbProtocol);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  mFvbInstance = FvbProtocol;

  Status = FvbVariableStorageWriteServiceInitialize ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FVB write service initialization failed. Status = %r\n", Status));
  } else {
    //
    // Notify the variable driver that FVB writes are ready
    //
    if (mVariableStorageSupport != NULL) {
      mVariableStorageSupport->NotifyWriteServiceReady ();
      mVssNotifyWriteServiceReadyCalled = TRUE;
    } else {
      mWriteServiceReady = TRUE;
    }
  }

  return Status;
}

/**
  SMM Variable Storage Support protocol notification event handler.

  The VSS protocol is needed to enable Non-Volatile variable writes

  @param[in]  Protocol   Points to the protocol's unique identifier
  @param[in]  Interface  Points to the interface instance
  @param[in]  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS   SmmEventCallback runs successfully
  @retval EFI_NOT_FOUND The Vss protocol is not found.

 **/
EFI_STATUS
EFIAPI
SmmVssNotificationEvent (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS  Status;

  if (mVssNotifyWriteServiceReadyCalled) {
    return EFI_SUCCESS;
  }

  //
  // Locate Smm Variable Storage Support protocol
  //
  Status = gMmst->MmLocateProtocol (
                    &gEdkiiVariableStorageSupportProtocolGuid,
                    NULL,
                    (VOID **)&mVariableStorageSupport
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (mVariableStorageSupport == NULL) {
    return EFI_NOT_FOUND;
  }

  if (mWriteServiceReady) {
    mVariableStorageSupport->NotifyWriteServiceReady ();
    mVssNotifyWriteServiceReadyCalled = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Initializes this variable storage driver.

  @param[in]  ImageHandle  The image handle.
  @param[in]  SystemTable  The system table.

  @retval EFI_SUCCESS  The protocol was installed successfully.
  @retval Others       Protocol could not be installed.
**/
EFI_STATUS
EFIAPI
FvbVariableStorageSmmInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *SmmFtwRegistration;
  VOID        *SmmVssRegistration;
  EFI_HANDLE  Handle = NULL;

  if (PcdGetBool (PcdEmuVariableNvModeEnable) || !PcdGetBool (PcdEnableFvbVariableStorage)) {
    DEBUG ((DEBUG_INFO, "FVB Variable Storage Protocol is disabled.\n"));
    return EFI_SUCCESS;
  }

  Status = FvbVariableStorageCommonInitialize ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Install the EDKII_VARIABLE_STORAGE_PROTOCOL
  //
  Status = gMmst->MmInstallProtocolInterface (
                    &Handle,
                    &gEdkiiVariableStorageProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mFvbVariableStorageProtocol
                    );

  //
  // Register FtwNotificationEvent () notify function.
  //
  Status = gMmst->MmRegisterProtocolNotify (
                    &gEfiSmmFaultTolerantWriteProtocolGuid,
                    SmmFtwNotificationEvent,
                    &SmmFtwRegistration
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Register FtwNotificationEvent () notify function.
  //
  Status = gMmst->MmRegisterProtocolNotify (
                    &gEdkiiVariableStorageSupportProtocolGuid,
                    SmmVssNotificationEvent,
                    &SmmVssRegistration
                    );
  ASSERT_EFI_ERROR (Status);

  SmmFtwNotificationEvent (NULL, NULL, NULL);
  SmmVssNotificationEvent (NULL, NULL, NULL);

  return Status;
}
