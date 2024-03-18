/** @file
  The Variable Storage Protocol is specific for the EDK II implementation of UEFI variables.

  This protocol abstracts non-volatile media storage access details from the EDK II UEFI variable
  driver. A platform may produce an arbitrary number of variable storage protocols that are used
  by the variable driver for variable management.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef VARIABLE_STORAGE_PROTOCOL_H_
#define VARIABLE_STORAGE_PROTOCOL_H_

#define EDKII_VARIABLE_STORAGE_PROTOCOL_GUID \
  { \
    0xa073a3a6, 0x96ec, 0x4173, { 0xa9, 0xbc, 0x39, 0x95, 0x06, 0xcd, 0xea, 0xc6 } \
  }

///
/// Revision
///
#define EDKII_VARIABLE_STORAGE_PROTOCOL_REVISION  1

typedef struct _EDKII_VARIABLE_STORAGE_PROTOCOL EDKII_VARIABLE_STORAGE_PROTOCOL;

/**
  Retrieves a protocol instance-specific GUID.

  Returns a unique GUID per VARIABLE_STORAGE_PROTOCOL instance.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[out]      VariableGuid           A pointer to an EFI_GUID that is this protocol instance's GUID.

  @retval          EFI_SUCCESS            The data was returned successfully.
  @retval          EFI_INVALID_PARAMETER  A required parameter is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_GET_ID)(
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL   *This,
  OUT       EFI_GUID                          *InstanceGuid
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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_GET_VARIABLE)(
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL   *This,
  IN        BOOLEAN                           AtRuntime,
  IN        BOOLEAN                           FromSmm,
  IN CONST  CHAR16                            *VariableName,
  IN CONST  EFI_GUID                          *VariableGuid,
  OUT       UINT32                            *Attributes OPTIONAL,
  IN OUT    UINTN                             *DataSize,
  OUT       VOID                              *Data
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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_VARIABLE)(
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL   *This,
  IN        BOOLEAN                           AtRuntime,
  IN        BOOLEAN                           FromSmm,
  IN CONST  CHAR16                            *VariableName,
  IN CONST  EFI_GUID                          *VariableGuid,
  OUT       UINT32                            *Attributes,
  IN OUT    UINTN                             *DataSize,
  OUT       VOID                              *Data,
  OUT       UINT32                            *KeyIndex,
  OUT       UINT64                            *MonotonicCount,
  OUT       EFI_TIME                          *TimeStamp
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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME)(
  IN CONST  EDKII_VARIABLE_STORAGE_PROTOCOL   *This,
  IN OUT    UINTN                             *VariableNameSize,
  IN OUT    CHAR16                            *VariableName,
  IN OUT    EFI_GUID                          *VariableGuid,
  OUT       UINT32                            *VariableAttributes
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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_GET_STORAGE_USAGE)(
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL   *This,
  IN          BOOLEAN                           AtRuntime,
  OUT         UINT32                            *VariableStoreSize,
  OUT         UINT32                            *CommonVariablesTotalSize,
  OUT         UINT32                            *HwErrVariablesTotalSize
  );

/**
  Returns whether this NV storage area supports storing authenticated variables.

  @param[in]  This                           A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.
  @param[out] AuthSupported                  TRUE if this NV storage area can store authenticated variables,
                                             FALSE otherwise.

  @retval     EFI_SUCCESS                    AuthSupported was returned successfully.
  @retval     EFI_INVALID_PARAMETER          A pointer parameter is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_SUPPORT)(
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL   *This,
  OUT         BOOLEAN                           *AuthSupported
  );

/**
  Returns whether this NV storage area is ready to accept calls to SetVariable().

  @param[in]  This                           A pointer to this instance of the EDKII_VARIABLE_STORAGE_PROTOCOL.

  @retval     TRUE                           The NV storage area is ready to accept calls to SetVariable().
  @retval     FALSE                          The NV storage area is not ready to accept calls to SetVariable().

**/
typedef
BOOLEAN
(EFIAPI *EDKII_VARIABLE_STORAGE_WRITE_SERVICE_IS_READY)(
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL   *This
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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_SET_VARIABLE)(
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL   *This,
  IN          BOOLEAN                           AtRuntime,
  IN          BOOLEAN                           FromSmm,
  IN          CHAR16                            *VariableName,
  IN          EFI_GUID                          *VendorGuid,
  IN          UINT32                            Attributes,
  IN          UINTN                             DataSize,
  IN          VOID                              *Data,
  IN          UINT32                            KeyIndex       OPTIONAL,
  IN          UINT64                            MonotonicCount OPTIONAL,
  IN          EFI_TIME                          *TimeStamp     OPTIONAL
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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_GARBAGE_COLLECT)(
  IN CONST    EDKII_VARIABLE_STORAGE_PROTOCOL   *This
  );

///
/// Variable Storage Protocol
/// Interface functions for variable non-volatile storage access in the DXE boot phase.
///
struct _EDKII_VARIABLE_STORAGE_PROTOCOL {
  EDKII_VARIABLE_STORAGE_GET_ID                        GetId;                             ///< Retrieves a protocol instance-specific GUID
  EDKII_VARIABLE_STORAGE_GET_VARIABLE                  GetVariable;                       ///< Retrieves a variable's value given its name and GUID
  EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_VARIABLE    GetAuthenticatedVariable;          ///< Retrieves an authenticated variable's value given its name and GUID
  EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME        GetNextVariableName;               ///< Return the next variable name and GUID
  EDKII_VARIABLE_STORAGE_GET_STORAGE_USAGE             GetStorageUsage;                   ///< Returns information on storage usage in the variable store
  EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_SUPPORT     GetAuthenticatedSupport;           ///< Returns whether this non-volatile storage area supports authenticated variables
  EDKII_VARIABLE_STORAGE_SET_VARIABLE                  SetVariable;                       ///< Sets a variable's value using its name and GUID.
  EDKII_VARIABLE_STORAGE_WRITE_SERVICE_IS_READY        WriteServiceIsReady;               ///< Indicates if SetVariable() is ready or not
  EDKII_VARIABLE_STORAGE_GARBAGE_COLLECT               GarbageCollect;                    ///< Performs variable store garbage collection/reclaim operation.
};

extern EFI_GUID  gEdkiiVariableStorageProtocolGuid;

#endif
