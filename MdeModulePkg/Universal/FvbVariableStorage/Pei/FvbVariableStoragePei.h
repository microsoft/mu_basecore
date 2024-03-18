/** @file
  Definitions for the Firmware Volume Block (FVB) Variable Storage PEIM.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FVB_VARIABLE_STORAGE_PEI_H_
#define FVB_VARIABLE_STORAGE_PEI_H_

#include <Ppi/VariableStorage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/SafeIntLib.h>
#include <Library/VariableFlashInfoLib.h>

#include <Guid/VariableFormat.h>
#include <Guid/VariableIndexTable.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/FaultTolerantWrite.h>
#include <Guid/FvbVariableStoragePpiInstance.h>

typedef enum {
  VariableStoreTypeNv,
  VariableStoreTypeMax
} VARIABLE_STORE_TYPE;

typedef struct {
  VARIABLE_STORE_HEADER                   *VariableStoreHeader;
  VARIABLE_INDEX_TABLE                    *IndexTable;
  //
  // If it is not NULL, it means there may be an inconsecutive variable whose
  // partial content is still in NV storage, but another partial content is backed up
  // in spare block.
  //
  FAULT_TOLERANT_WRITE_LAST_WRITE_DATA    *FtwLastWriteData;
  BOOLEAN                                 AuthFlag;
} VARIABLE_STORE_INFO;

/**
  Provide the functionality of FVB variable storage services.

  @param[in]   FileHandle     Handle of the file being invoked.
                              Type EFI_PEI_FILE_HANDLE is defined in FfsFindNextFile().
  @param[in]   PeiServices    A pointer to the PEI Services table.

  @retval EFI_SUCCESS  If the interface could be successfully installed
  @retval Others       Returned from PeiServicesInstallPpi()
**/
EFI_STATUS
EFIAPI
PeimInitializeFvbVariableStorageServices (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );

/**
  Retrieves a PPI instance-specific GUID.

  Returns a unique GUID per VARIABLE_STORAGE_PPI instance.

  @param[in]       This                   A pointer to this instance of the EDKII_VARIABLE_STORAGE_PPI.
  @param[out]      InstanceGuid           A pointer to an EFI_GUID that is this PPI instance's GUID.

  @retval          EFI_SUCCESS            The data was returned successfully.
  @retval          EFI_INVALID_PARAMETER  A required parameter is NULL.

**/
EFI_STATUS
EFIAPI
PeiFvbVariableStorageGetId (
  IN CONST  EDKII_VARIABLE_STORAGE_PPI  *This,
  OUT       EFI_GUID                    *InstanceGuid
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
EFI_STATUS
EFIAPI
PeiFvbVariableStorageGetVariable (
  IN CONST  EDKII_VARIABLE_STORAGE_PPI  *This,
  IN CONST  CHAR16                      *VariableName,
  IN CONST  EFI_GUID                    *VariableGuid,
  OUT       UINT32                      *Attributes,
  IN OUT    UINTN                       *DataSize,
  OUT       VOID                        *Data
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
EFI_STATUS
EFIAPI
PeiFvbVariableStorageGetNextVariableName (
  IN CONST  EDKII_VARIABLE_STORAGE_PPI  *This,
  IN OUT    UINTN                       *VariableNameSize,
  IN OUT    CHAR16                      *VariableName,
  IN OUT    EFI_GUID                    *VariableGuid,
  OUT       UINT32                      *VariableAttributes
  );

#endif
