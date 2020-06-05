/** @file
  Builds and installs the HEST ACPI table.

  Defines the protocol interfaces that allow, creation of HEST ACPI table,
  adding of error source descriptors to the table and installation of the
  of the dynamically generated HEST ACPI table.

  Copyright (c) 2020 - 2021, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  MU_CHANGE: Cherry-pick HEST Driver
**/

#ifndef HEST_TABLE_PROTOCOL_H_
#define HEST_TABLE_PROTOCOL_H_

#define HEST_TABLE_PROTOCOL_GUID \
  { \
    0x705bdcd9, 0x8c47, 0x457e, \
    { 0xad, 0x0d, 0xf7, 0x86, 0xf3, 0x4a, 0x0d, 0x63 } \
  }

/**
  Add HEST error source descriptor protocol interface.

  Protocol interface used to add error source descriptors to HEST table.
  Linked list is implemented to hold the HEST table error source descriptor
  information. Every error source descriptor(s) is added as a new node. First
  call to this interface will create new linked list and add HEST header
  information as a head node of the list.

  @param[in] ErrorSourceDescriptorList      List of Error Source Descriptors.
  @param[in] ErrorSourceDescriptorListSize  Total Size of Error Source
                                            Descriptors.
  @param[in] ErrorSourceDescriptorCount     Total count of error source
                                            descriptors.

  @retval EFI_SUCCESS            Adding error source descriptors to list
                                 successful.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval EFI_INVALID_PARAMETER  ErrorSourceDescriptorList is NULL or
                                 ErrorSourceDescriptorListSize is 0.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_ADD_ERROR_SOURCE_DESCRIPTOR)(
  IN CONST VOID *ErrorSourceDescriptorList,
  IN UINTN      ErrorSourceDescriptorListSize,
  IN UINTN      ErrorSourceDescriptorCount
  );

/**
  Install HEST table protocol interface.

  Builds HEST table from the linked list carrying HEST header and HEST error
  source descriptor information and installs it.

  @retval EFI_SUCCESS    HEST table is installed successfully.
  @retval EFI_NOT_FOUND  List is NULL.
  @retval Other          Install interface call failed.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_INSTALL_HEST_TABLE)(VOID);

//
// HEST_TABLE_PROTOCOL enables creation and installation of HEST table.
//
typedef struct {
  EDKII_ADD_ERROR_SOURCE_DESCRIPTOR    AddErrorSourceDescriptors;
  EDKII_INSTALL_HEST_TABLE             InstallHestTable;
} EDKII_HEST_TABLE_PROTOCOL;

extern EFI_GUID  gHestTableProtocolGuid;
#endif // HEST_TABLE_PROTOCOL_H_
