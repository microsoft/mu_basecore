/** @file
  The Variable Storage PPI is specific for the EDK II implementation of UEFI variables.

  This PPI abstracts non-volatile media storage access details from the EDK II PEI UEFI variable
  module. A platform may produce an arbitrary number of variable storage PPTs that are used by the
  PEI variable module for variable management during the PEI boot phase.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PEI_VARIABLE_STORAGE_PPI_H_
#define PEI_VARIABLE_STORAGE_PPI_H_

#define EDKII_VARIABLE_STORAGE_PPI_GUID \
  { \
    0x90d915c5, 0xe4c1, 0x4da8, { 0xa7, 0x6f, 0x9,  0xe5, 0x78, 0x91, 0x65, 0x48 }\
  }

///
/// Revision
///
#define EDKII_VARIABLE_STORAGE_PPI_REVISION  1

typedef struct _EDKII_VARIABLE_STORAGE_PPI EDKII_VARIABLE_STORAGE_PPI;

/**
  Retrieves a PPI instance-specific GUID.

  Returns a unique GUID per VARIABLE_STORAGE_PPI instance.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PPI.
  @param[out]      InstanceGuid           A pointer to an EFI_GUID that is this PPI instance's GUID.

  @retval          EFI_SUCCESS            The data was returned successfully.
  @retval          EFI_INVALID_PARAMETER  A required parameter is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_EDKII_VARIABLE_STORAGE_GET_ID)(
  IN CONST  EDKII_VARIABLE_STORAGE_PPI      *This,
  OUT       EFI_GUID                        *InstanceGuid
  );

/**
  Retrieves a variable's value using its name and GUID.

  Reads the specified variable from the UEFI variable store. If the Data buffer is too small
  to hold the contents of the variable the error EFI_BUFFER_TOO_SMALL is returned and DataSize
  is set to the required buffer size to obtain the data.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PPI.
  @param[in]       VariableName           A pointer to a null-terminated string that is the variable's name.
  @param[in]       VariableGuid           A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                          VariableGuid and VariableName must be unique.
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
(EFIAPI *PEI_EDKII_VARIABLE_STORAGE_GET_VARIABLE)(
  IN CONST  EDKII_VARIABLE_STORAGE_PPI      *This,
  IN CONST  CHAR16                          *VariableName,
  IN CONST  EFI_GUID                        *VariableGuid,
  OUT       UINT32                          *Attributes OPTIONAL,
  IN OUT    UINTN                           *DataSize,
  OUT       VOID                            *Data
  );

/**
  Return the next variable name and GUID.

  This function is called multiple times to retrieve the VariableName and VariableGuid of all variables
  currently available in the system. On each call, the previous results are passed into the interface,
  and, on return, the interface returns the data for the next interface. When the entire variable list
  has been returned, EFI_NOT_FOUND is returned.

  @param[in]      This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PPI.
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
(EFIAPI *PEI_EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME)(
  IN CONST  EDKII_VARIABLE_STORAGE_PPI      *This,
  IN OUT    UINTN                           *VariableNameSize,
  IN OUT    CHAR16                          *VariableName,
  IN OUT    EFI_GUID                        *VariableGuid,
  OUT       UINT32                          *VariableAttributes
  );

///
/// Variable Storage PPI
/// Interface functions for variable non-volatile storage access in the PEI boot phase.
///
struct _EDKII_VARIABLE_STORAGE_PPI {
  PEI_EDKII_VARIABLE_STORAGE_GET_ID                    GetId;                ///< Retrieves a PPI instance-specific GUID
  PEI_EDKII_VARIABLE_STORAGE_GET_VARIABLE              GetVariable;          ///< Retrieves a variable's value given its name and GUID
  PEI_EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME    GetNextVariableName;  ///< Return the next variable name and GUID
};

extern EFI_GUID  gEdkiiVariableStoragePpiGuid;

#endif
