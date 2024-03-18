/** @file
  Definitions shared across the Firmware Volume Block (FVB) variable storage driver.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FVB_VARIABLE_STORAGE_INTERNAL_H_
#define FVB_VARIABLE_STORAGE_INTERNAL_H_

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/VariableFlashInfoLib.h>
#include <Protocol/FaultTolerantWrite.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/VariableStorage.h>
#include <Protocol/VariableStorageSupport.h>
#include <Guid/FaultTolerantWrite.h>
#include <Guid/FvbVariableStorageProtocolInstance.h>
#include <Guid/VarErrorFlag.h>
#include <Guid/VariableFormat.h>

#define EFI_VARIABLE_ATTRIBUTES_MASK  (EFI_VARIABLE_NON_VOLATILE |\
                                      EFI_VARIABLE_BOOTSERVICE_ACCESS | \
                                      EFI_VARIABLE_RUNTIME_ACCESS | \
                                      EFI_VARIABLE_HARDWARE_ERROR_RECORD | \
                                      EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS | \
                                      EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | \
                                      EFI_VARIABLE_APPEND_WRITE)

extern EDKII_VARIABLE_STORAGE_PROTOCOL  mFvbVariableStorageProtocol;

///
/// FVB Instance for writting to NV Storage
///
extern EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *mFvbInstance;

///
/// VSS Protocol for communicating with the UEFI variable driver
///
extern EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL  *mVariableStorageSupport;

///
/// MMIO address of the FLASH device which contains the NV Storage FV
///
extern EFI_PHYSICAL_ADDRESS  mNonVolatileVariableBase;

///
/// A memory cache that improves the search performance and enables reclaim
///
extern VARIABLE_STORE_HEADER  *mNvVariableCache;

typedef struct {
  VARIABLE_HEADER    *CurrPtr;
  //
  // If both ADDED and IN_DELETED_TRANSITION variable are present,
  // InDeletedTransitionPtr will point to the IN_DELETED_TRANSITION one.
  // Otherwise, CurrPtr will point to the ADDED or IN_DELETED_TRANSITION one,
  // and InDeletedTransitionPtr will be NULL at the same time.
  //
  VARIABLE_HEADER    *InDeletedTransitionPtr;
  VARIABLE_HEADER    *EndPtr;
  VARIABLE_HEADER    *StartPtr;
} VARIABLE_POINTER_TRACK;

/**
  Performs common initialization needed for this module.

  @retval EFI_SUCCESS  The module was initialized successfully.
  @retval Others       The module could not be initialized.
**/
EFI_STATUS
EFIAPI
FvbVariableStorageCommonInitialize (
  VOID
  );

/**
  Initializes FVB write service after FTW is ready.

  @retval EFI_SUCCESS          Function successfully executed.
  @retval Others               Fail to initialize the variable service.

**/
EFI_STATUS
FvbVariableStorageWriteServiceInitialize (
  VOID
  );

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
  );

/**
  Calls SetVariable () on the core UEFI variable implementation

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.
  @param[in] Attributes                   Attribute value of the variable found
  @param[in] DataSize                     Size of Data found. If size is less than the data, this value contains the
                                          required size.
  @param[in] Data                         Data pointer.

  @return EFI_SUCCESS                     Set successfully.
  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_OUT_OF_RESOURCES            Insufficient memory resources.
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
  );

/**
  Writes a buffer to variable storage space in the working block.

  This function writes a buffer to variable storage space into a firmware volume block device. The destination is
  specified by the parameter VariableBase. The Fault Tolerant Write protocol is used for writing.

  @param[in]  VariableBase   Base address of variable to write
  @param[in]  VariableBuffer Point to the variable data buffer.

  @retval EFI_SUCCESS    The function completed successfully.
  @retval EFI_NOT_FOUND  Fail to locate the Fault Tolerant Write protocol.
  @retval EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
FtwVariableSpace (
  IN EFI_PHYSICAL_ADDRESS   VariableBase,
  IN VARIABLE_STORE_HEADER  *VariableBuffer
  );

/**
  Retrieves the SMM Fault Tolerent Write protocol.

  @param[out] FtwProtocol       A pointer to the FTW protocol instance located.

  @retval EFI_SUCCESS           A SMM FTW protocol instance was found and returned in FtwProtocol.
  @retval EFI_NOT_FOUND         A SMM FTW protocol instance was not found.
  @retval EFI_INVALID_PARAMETER FtwProtocol is NULL.

**/
EFI_STATUS
GetFtwProtocol (
  OUT VOID  **FtwProtocol
  );

/**
  Get the proper FVB handle and/or FVB protocol by the given flash address.

  @param[in] Address        The flash address.
  @param[out] FvbHandle     On output, if it is not NULL, it points to the proper FVB handle.
  @param[out] FvbProtocol   On output, if it is not NULL, it points to the proper FVB protocol.

**/
EFI_STATUS
GetFvbInfoByAddress (
  IN  EFI_PHYSICAL_ADDRESS                Address,
  OUT EFI_HANDLE                          *FvbHandle OPTIONAL,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvbProtocol OPTIONAL
  );

/**
  Retrieves the FVB protocol interface by handle.

  @param[in]  FvBlockHandle     The handle of FVB protocol that provides services for  reading, writing, and erasing
                                the target block.
  @param[out] FvBlock           A pointer to a FVB protocol instance.

  @retval EFI_SUCCESS           The pointer for the specified protocol was updated.
  @retval EFI_UNSUPPORTED       The device does not support the FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.

**/
EFI_STATUS
GetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  );

/**
  Function returns an array of handles that support the FVB protocol in a buffer allocated from pool.

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to a buffer to return the requested array of handles that support the FVB
                                protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer and the number of handles in Buffer was
                                returned in NumberHandles.
  @retval EFI_NOT_FOUND         No FVB handles were found.
  @retval EFI_OUT_OF_RESOURCES  There is not enough pool memory to store the matching results.
  @retval EFI_INVALID_PARAMETER NumberHandles is NULL or Buffer is NULL.

**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN       *NumberHandles,
  OUT EFI_HANDLE  **Buffer
  );

/**
  Retrieves a protocol instance-specific GUID.

  Returns a unique GUID per VARIABLE_STORAGE_PROTOCOL instance.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[out]      VariableGuid           A pointer to an EFI_GUID that is this protocol instance's GUID.

  @retval          EFI_SUCCESS            The data was returned successfully.
  @retval          EFI_INVALID_PARAMETER  A required parameter is NULL.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetId (
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  OUT       EFI_GUID                         *InstanceGuid
  );

/**
  Retrieves an authenticated variable's value using its name and GUID.

  Reads the specified authenticated variable from the UEFI variable store. If the Data
  buffer is too small to hold the contents of the variable, the error EFI_BUFFER_TOO_SMALL
  is returned and DataSize is set to the required buffer size to obtain the data.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in]       AtRuntime              TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
  @param[in]       FromSmm                TRUE if GetVariable() is being called by SMM code, FALSE if called by DXE code.
  @param[in]       VariableName           A pointer to a null-terminated string that is the variable's name.
  @param[in]       VariableGuid           A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                          VariableGuid and VariableName must be unique.
  @param[out]      Attributes             If non-NULL, on return, points to the variable's attributes.
  @param[in, out]  DataSize               On entry, points to the size in bytes of the Data buffer.
                                          On return, points to the size of the data returned in Data.
  @param[out]      Data                   Points to the buffer which will hold the returned variable value.
  @param[out]      KeyIndex               Index of associated public key in database.
  @param[out]      MonotonicCount         Associated monotonic count value to protect against replay attack.
  @param[out]      TimeStamp              Associated TimeStamp value to protect against replay attack.

  @retval          EFI_SUCCESS            The variable was read successfully.
  @retval          EFI_NOT_FOUND          The variable could not be found.
  @retval          EFI_BUFFER_TOO_SMALL   The DataSize is too small for the resulting data.
                                          DataSize is updated with the size required for
                                          the specified variable.
  @retval          EFI_INVALID_PARAMETER  VariableName, VariableGuid, DataSize or Data is NULL.
  @retval          EFI_DEVICE_ERROR       The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetAuthenticatedVariable (
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN        BOOLEAN                          AtRuntime,
  IN        BOOLEAN                          FromSmm,
  IN CONST  CHAR16                           *VariableName,
  IN CONST  EFI_GUID                         *VariableGuid,
  OUT       UINT32                           *Attributes,
  IN OUT    UINTN                            *DataSize,
  OUT       VOID                             *Data,
  OUT       UINT32                           *KeyIndex,
  OUT       UINT64                           *MonotonicCount,
  OUT       EFI_TIME                         *TimeStamp
  );

/**
  Retrieves a variable's value using its name and GUID.

  Reads the specified variable from the UEFI variable store. If the Data buffer is too small
  to hold the contents of the variable the error EFI_BUFFER_TOO_SMALL is returned and DataSize
  is set to the required buffer size to obtain the data.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in]       AtRuntime              TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
  @param[in]       FromSmm                TRUE if GetVariable() is being called by SMM code, FALSE if called by DXE code.
  @param[in]       VariableName           A pointer to a null-terminated string that is the variable's name.
  @param[in]       VariableGuid           A pointer to an EFI_GUID that is the variable's vendor GUID. The combination
                                          of VariableGuid and VariableName must be unique.
  @param[out]      Attributes             If non-NULL, on return, points to the variable's attributes.
  @param[in, out]  DataSize               On entry, points to the size in bytes of the Data buffer.
                                          On return, points to the size of the data returned in Data.
  @param[out]      Data                   Points to the buffer which will hold the returned variable value.

  @retval          EFI_SUCCESS            The variable was read successfully.
  @retval          EFI_NOT_FOUND          The variable could not be found.
  @retval          EFI_BUFFER_TOO_SMALL   The DataSize is too small for the resulting data.
                                          DataSize is updated with the size required for the specified variable.
  @retval          EFI_INVALID_PARAMETER  VariableName, VariableGuid, DataSize or Data is NULL.
  @retval          EFI_DEVICE_ERROR       The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetVariable (
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN        BOOLEAN                          AtRuntime,
  IN        BOOLEAN                          FromSmm,
  IN CONST  CHAR16                           *VariableName,
  IN CONST  EFI_GUID                         *VariableGuid,
  OUT       UINT32                           *Attributes OPTIONAL,
  IN OUT    UINTN                            *DataSize,
  OUT       VOID                             *Data
  );

/**
  Return the next variable name and GUID.

  This function is called multiple times to retrieve the VariableName and VariableGuid of all variables
  currently available in the system. On each call, the previous results are passed into the interface,
  and, on return, the interface returns the data for the next interface. When the entire variable list
  has been returned, EFI_NOT_FOUND is returned.

  @param[in]      This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in, out] VariableNameSize       On entry, points to the size of the buffer pointed to by
                                         VariableName. On return, the size of the variable name buffer.
  @param[in, out] VariableName           On entry, a pointer to a null-terminated string that is the
                                         variable's name. On return, points to the next variable's
                                         null-terminated name string.
  @param[in, out] VariableGuid           On entry, a pointer to an EFI_GUID that is the variable's GUID.
                                         On return, a pointer to the next variable's GUID.
  @param[out]     VariableAttributes     A pointer to the variable attributes.

  @retval         EFI_SUCCESS            The variable was read successfully.
  @retval         EFI_NOT_FOUND          The variable could not be found.
  @retval         EFI_BUFFER_TOO_SMALL   The VariableNameSize is too small for the resulting data.
                                         VariableNameSize is updated with the size required for the specified
                                         variable.
  @retval         EFI_INVALID_PARAMETER  VariableName, VariableGuid or VariableNameSize is NULL.
  @retval         EFI_DEVICE_ERROR       The variable could not be retrieved because of a device error.
**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetNextVariableName (
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN OUT    UINTN                            *VariableNameSize,
  IN OUT    CHAR16                           *VariableName,
  IN OUT    EFI_GUID                         *VariableGuid,
  OUT       UINT32                           *VariableAttributes
  );

/**
  Returns information on the amount of space available in the variable store. If the amount of data that can be written
  depends on if the platform is in Pre-OS stage or OS stage, the AtRuntime parameter should be used to compute usage.

  @param[in]  This                           A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in]  AtRuntime                      TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
  @param[out] VariableStoreSize              The total size of the NV storage. Indicates the maximum amount
                                             of data that can be stored in this NV storage area.
  @param[out] CommonVariablesTotalSize       The total combined size of all the common UEFI variables that are
                                             stored in this NV storage area. Excludes variables with the
                                             EFI_VARIABLE_HARDWARE_ERROR_RECORD attribute set.
  @param[out] HwErrVariablesTotalSize        The total combined size of all the UEFI variables that have the
                                             EFI_VARIABLE_HARDWARE_ERROR_RECORD attribute set and which are
                                             stored in this NV storage area. Excludes all other variables.

  @retval     EFI_SUCCESS                    Space information was returned successfully.
  @retval     EFI_INVALID_PARAMETER          Any of the given parameters are NULL.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetStorageUsage (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN          BOOLEAN                          AtRuntime,
  OUT         UINT32                           *VariableStoreSize,
  OUT         UINT32                           *CommonVariablesTotalSize,
  OUT         UINT32                           *HwErrVariablesTotalSize
  );

/**
  Returns whether this NV storage area supports storing authenticated variables.

  @param[in]  This                           A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[out] AuthSupported                  TRUE if this NV storage area can store authenticated variables,
                                             FALSE otherwise.

  @retval     EFI_SUCCESS                    AuthSupported was returned successfully.
  @retval     EFI_INVALID_PARAMETER          A pointer parameter is NULL.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGetAuthenticatedSupport (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  OUT         BOOLEAN                          *AuthSupported
  );

/**
  This code sets a variable's value using its name and GUID.

  Caution: This function may receive untrusted input.

  - This function may be invoked in SMM mode and will be given untrusted input data.
  - This function must perform basic validation before parsing the data.
  - This function must parse the authentication carefully to prevent security issues, like
    buffer overflow and integer overflow.
  - This function must check variable attributes carefully to prevent authentication bypass.

  @param[in]  This                             A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[in]  AtRuntime                        TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
  @param[in]  FromSmm                          TRUE if SetVariable() is being called by SMM code, FALSE if called by
                                               DXE code.
  @param[in]  VariableName                     Name of Variable to be found.
  @param[in]  VendorGuid                       Variable vendor GUID.
  @param[in]  Attributes                       Attribute value of the variable found.
  @param[in]  DataSize                         Size of Data found. If size is less than the data, this value contains
                                               the required size.
  @param[in]  Data                             Data pointer.
  @param[in]  KeyIndex                         If writing an authenticated variable, the public key index
  @param[in]  MonotonicCount                   If writing a monotonic counter authenticated variable, the counter value
  @param[in]  TimeStamp                        If writing a timestamp authenticated variable, the timestamp value

  @retval     EFI_SUCCESS                      The variable was set successfully.
  @retval     EFI_INVALID_PARAMETER            An invalid parameter was given.
  @retval     EFI_OUT_OF_RESOURCES             Insufficient resources to set the variable.
  @retval     EFI_WRITE_PROTECTED              The variable is read-only.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageSetVariable (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This,
  IN          BOOLEAN                          AtRuntime,
  IN          BOOLEAN                          FromSmm,
  IN          CHAR16                           *VariableName,
  IN          EFI_GUID                         *VendorGuid,
  IN          UINT32                           Attributes,
  IN          UINTN                            DataSize,
  IN          VOID                             *Data,
  IN          UINT32                           KeyIndex       OPTIONAL,
  IN          UINT64                           MonotonicCount OPTIONAL,
  IN          EFI_TIME                         *TimeStamp     OPTIONAL
  );

/**
  Returns whether this NV storage area is ready to accept calls to SetVariable().

  @param[in]  This                           A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.

  @retval     TRUE                           The NV storage area is ready to accept calls to SetVariable().
  @retval     FALSE                          The NV storage area is not ready to accept calls to SetVariable().

**/
BOOLEAN
EFIAPI
FvbVariableStorageWriteServiceIsReady (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This
  );

/**
  Performs variable store garbage collection/reclaim operation.

  @param[in]  This                             A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.

  @retval     EFI_SUCCESS                      The garbage collection operation was successful.
  @retval     EFI_INVALID_PARAMETER            An invalid parameter was given.
  @retval     EFI_OUT_OF_RESOURCES             Insufficient resource to complete garbage collection.
  @retval     EFI_NOT_READY                    Write services are garbage collection in general is not ready.
  @retval     EFI_WRITE_PROTECTED              Write services are not available at this time.

**/
EFI_STATUS
EFIAPI
FvbVariableStorageGarbageCollect (
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL  *This
  );

#endif
