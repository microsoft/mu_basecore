/** @file
  The Variable Storage Selector PPI is specific for the EDK II implementation of UEFI variables.

  This PPI is used by the EDK II PEI UEFI variable module to acquire the variable storage instance ID (GUID)
  for a particular variable name and vendor GUID. This ID is used to locate the appropriate variable storage
  PPI.

  A platform must only install a single Variable Storage Selector PPI. The PPI should only be installed
  after all Variable Storage PPIs that will used on the platform have been installed. The installation of the
  Variable Storage Selector PPI is a signal that the platform has accounted for all of its variable storage
  methods and is ready to initialize PEI variable services which will utilize those methods.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PEI_VARIABLE_STORAGE_SELECTOR_PPI_H_
#define PEI_VARIABLE_STORAGE_SELECTOR_PPI_H_

#define EDKII_VARIABLE_STORAGE_SELECTOR_PPI_GUID \
  { \
    0x782546d1, 0x03ab, 0x41e4, { 0xa0, 0x1d, 0x7a, 0x9b, 0x22, 0xba, 0x2e, 0x1e } \
  }

///
/// Revision
///
#define PEI_VARIABLE_STORAGE_PPI_REVISION  1

typedef struct _EDKII_VARIABLE_STORAGE_SELECTOR_PPI EDKII_VARIABLE_STORAGE_SELECTOR_PPI;

/**
  Gets the ID (GUID) for the variable storage instance that is used to store a given variable.

  This function will be invoked any time the UEFI variable module needs to perform a variable operation
  (e.g. GetVariable (), GetNextVariableName (), SetVariable (), etc.) and must determine what Variable
  Storage PPI should be used for the transaction.

  This function may be implemented with arbitrary logic to select a given Variable Storage PPI. For
  example, if only firmware volume block based (FVB) storage is used, the platform should install a single
  Variable Storage PPI and simply always return that PPI instance GUID unconditionally in this function
  implementation.

  As another example, if a platform wishes to define two non-volatile regions of SPI flash to store variables,
  it could produce two FVB-backed Variable Storage PPI instances. This function implementation could be
  implemented to store a targeted set of variables in one store and all other variables in the other store. It
  could do this by defining the targeted subset via a known list of variable name and variable GUID or simply
  give all variables for a secondary store a special GUID and conditonalize on that.

  In any case, the ID returned must be consistent for a given set of inputs (i.e. variable name and variable GUID).
  This ensures get and set operations throughout boot and the platform's lifetime refer to the same store of data.

  @param[in]  VariableName       A pointer to a null-terminated string that is the variable's name.
  @param[in]  VariableGuid       A pointer to an EFI_GUID that is the variable's GUID.
  @param[out] VariableStorageId  The ID for the variable storage instance that stores a given variable.

  @retval     EFI_SUCCESS        Variable storage instance ID that was retrieved.
  @retval     EFI_NOT_FOUND      A variable storage instance is not available to store this variable.
**/
typedef
EFI_STATUS
(EFIAPI *PEI_EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID)(
  IN  CONST  CHAR16       *VariableName,
  IN  CONST  EFI_GUID     *VendorGuid,
  OUT        EFI_GUID     *VariableStorageId
  );

///
/// Variable Storage PPI
///
struct _EDKII_VARIABLE_STORAGE_SELECTOR_PPI {
  PEI_EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID    GetId;      ///< Retrieves an instance-specific variable storage ID
};

extern EFI_GUID  gEdkiiVariableStorageSelectorPpiGuid;

#endif
