/** @file
  The Variable Storage Selector Protocol is specific for the EDK II implementation of UEFI variables.

  This protocol is used by the EDK II UEFI variable driver to acquire the variable storage instance ID (GUID)
  for a particular variable name and vendor GUID. This ID is used to locate the appropriate variable storage
  protocol.

  A platform must only install a single Variable Storage Selector protocol. The protocol should only be installed
  after all Variable Storage protocols that will used on the platform have been installed. The installation of the
  Variable Storage Selector protocol is a signal that the platform has accounted for all of its variable storage
  methods and is ready to initialize UEFI variable services which will utilize those methods.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef VARIABLE_STORAGE_SELECTOR_PROTOCOL_H_
#define VARIABLE_STORAGE_SELECTOR_PROTOCOL_H_

#define EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL_GUID \
  { \
    0xd56ede61, 0x81f5, 0x48f3, { 0xaf, 0x4b, 0x8d, 0x54, 0x83, 0xb9, 0xec, 0xc9 } \
  }

#define EDKII_SMM_VARIABLE_STORAGE_SELECTOR_PROTOCOL_GUID \
  { \
    0x1dbbe48c, 0x9087, 0x44e7, { 0x96, 0x6e, 0x80, 0x75, 0xe4, 0x12, 0x0b, 0xd8 } \
  }

///
/// Revision
///
#define EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL_REVISION  1

typedef struct _EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL;

/**
  Gets the ID (GUID) for the variable storage instance that is used to store a given variable.

  This function will be invoked any time the UEFI variable driver needs to perform a variable operation
  (e.g. GetVariable (), GetNextVariableName (), SetVariable (), etc.) and must determine what Variable
  Storage Protocol should be used for the transaction.

  This function may be implemented with arbitrary logic to select a given Variable Storage Protocol. For
  example, if only firmware volume block based (FVB) storage is used, the platform should install a single
  Variable Storage Protocol and simply always return that protocol instance GUID unconditionally in this
  function implementation.

  As another example, if a platform wishes to define two non-volatile regions of SPI flash to store variables,
  it could produce two FVB-backed Variable Storage protocol instances. This function implementation could be
  implemented to store a targeted set of variables in one store and all other variables in the other store. It
  could do this by defining the targeted subset via a known list of variable name and variable GUID or simply
  give all variables for a secondary store a special GUID and conditonalize on that.

  In any case, the ID returned must be consistent for a given set of inputs (i.e. variable name and variable GUID).
  This ensures get and set operations throughout boot and the platform's lifetime refer to the same store of data.

  @param[in]  VariableName       A pointer to a null-terminated string that is the variable's name.
  @param[in]  VariableGuid       A pointer to an EFI_GUID that is the variable's vendor GUID.
  @param[out] VariableStorageId  The ID for the variable storage instance that stores a given variable.

  @retval     EFI_SUCCESS        Variable storage instance ID that was retrieved.
  @retval     EFI_NOT_FOUND      A variable storage instance is not available to store this variable.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID)(
  IN  CONST  CHAR16       *VariableName,
  IN  CONST  EFI_GUID     *VendorGuid,
  OUT        EFI_GUID     *VariableStorageId
  );

///
/// Variable Storage Selector Protocol
///
struct _EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL {
  EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID    GetId;
};

extern EFI_GUID  gEdkiiVariableStorageSelectorProtocolGuid;
extern EFI_GUID  gEdkiiSmmVariableStorageSelectorProtocolGuid;

#endif
