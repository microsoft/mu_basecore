/** @file
  Dynamically builds and installs HEST ACPI table.

  This driver implements protocol interfaces that can be used to create and
  install HEST ACPI table. The interface allows one or more error source
  producers to add the error source descriptors into the HEST table. And also
  allow the resulting HEST table to be installed.

  The HEST table is created as a Linked List. Wherein the list head node holds
  the HEST header data. The Error source descriptors are added to subsequent
  list nodes.

  Copyright (c) 2020 - 2022, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - ACPI 6.4, Table 18.2, Hardware Error Source Table

  MU_CHANGE: Cherry-pick HEST Driver
**/

#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/HestTableProtocol.h>

typedef struct {
  LIST_ENTRY    Link;
  VOID          *HestTableData;     /// HEST table data.
  UINT32        DataLength;         /// HEST table data length.
} HEST_DXE_DRIVER_DATA;

STATIC EFI_ACPI_TABLE_PROTOCOL  *gAcpiTableProtocol = NULL;

// This error source descriptor list.
STATIC LIST_ENTRY  gHestErrorSourceList =
  INITIALIZE_LIST_HEAD_VARIABLE (gHestErrorSourceList);

/**
  Creates list head node and populates it with HEST header information.

  Helper function that populates the HEST table header. Called only once during
  the execution of add error source descriptor protocol interface.

  @retval EFI_SUCCESS           On successful creation of HEST header.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
STATIC
EFI_STATUS
BuildHestHeader (
  VOID
  )
{
  EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *HestHeaderPtr;
  HEST_DXE_DRIVER_DATA                             *HestDataList;

  //
  // Allocate memory for the head node of the list and HEST table header
  // structure.
  //
  HestDataList = AllocateZeroPool (sizeof (HEST_DXE_DRIVER_DATA));

  if (HestDataList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HestDataList->HestTableData =
    AllocateZeroPool (
      sizeof (EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_HEADER)
      );

  if (HestDataList->HestTableData == NULL) {
    FreePool (HestDataList);
    return EFI_OUT_OF_RESOURCES;
  }

  HestHeaderPtr = (EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_HEADER *)
                  HestDataList->HestTableData;
  //
  // Populate list head with HEST header data.
  //
  HestHeaderPtr->Header.Signature =
    EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE;
  HestHeaderPtr->Header.Length +=
    sizeof (EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_HEADER);
  HestHeaderPtr->Header.Revision =
    EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_REVISION;
  CopyMem (
    &HestHeaderPtr->Header.OemId,
    FixedPcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (HestHeaderPtr->Header.OemId)
    );
  HestHeaderPtr->Header.OemTableId      = FixedPcdGet64 (PcdAcpiDefaultOemTableId);
  HestHeaderPtr->Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  HestHeaderPtr->Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  HestHeaderPtr->Header.CreatorRevision =
    PcdGet32 (PcdAcpiDefaultCreatorRevision);
  HestHeaderPtr->ErrorSourceCount = 0;

  // HEST header data length.
  HestDataList->DataLength =
    sizeof (EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_HEADER);

  // Insert the head list.
  InsertHeadList (&gHestErrorSourceList, &HestDataList->Link);
  return EFI_SUCCESS;
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
STATIC
EFI_STATUS
EFIAPI
AddErrorSourceDescriptor (
  IN CONST VOID  *ErrorSourceDescriptorList,
  IN UINTN       ErrorSourceDescriptorListSize,
  IN UINTN       ErrorSourceDescriptorCount
  )
{
  EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *HestHeaderPtr;
  HEST_DXE_DRIVER_DATA                             *HestDataList;
  EFI_STATUS                                       Status;
  LIST_ENTRY                                       *Link;

  if ((ErrorSourceDescriptorList == NULL) ||
      (ErrorSourceDescriptorListSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Create a HEST table header if not already created.
  //
  if (IsListEmpty (&gHestErrorSourceList)) {
    Status = BuildHestHeader ();
    if (Status == EFI_OUT_OF_RESOURCES) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to build HEST header, status: %r\n",
        __FUNCTION__,
        Status
        ));
      return Status;
    }
  }

  //
  // Create a new node to store Error Source Descriptor(s) information. Add the
  // newly created node to the end of list.
  //
  HestDataList                = AllocateZeroPool (sizeof (HEST_DXE_DRIVER_DATA));
  HestDataList->HestTableData =
    AllocateZeroPool (ErrorSourceDescriptorListSize);
  if ((HestDataList == NULL) || (HestDataList->HestTableData == NULL)) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (
    HestDataList->HestTableData,
    ErrorSourceDescriptorList,
    ErrorSourceDescriptorListSize
    );
  HestDataList->DataLength = ErrorSourceDescriptorListSize;

  InsertTailList (&gHestErrorSourceList, &HestDataList->Link);

  // Update length and error source count fields information in HEST header i.e
  // in head node of the list.
  Link          = GetFirstNode (&gHestErrorSourceList);
  HestDataList  = BASE_CR (Link, HEST_DXE_DRIVER_DATA, Link);
  HestHeaderPtr = (EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_HEADER *)
                  HestDataList->HestTableData;
  HestHeaderPtr->Header.Length    += ErrorSourceDescriptorListSize;
  HestHeaderPtr->ErrorSourceCount += ErrorSourceDescriptorCount;

  DEBUG ((
    DEBUG_INFO,
    "HestDxe: %d Error source descriptor(s) added \n",
    ErrorSourceDescriptorCount
    ));
  return EFI_SUCCESS;
}

/**
  Install HEST table protocol interface.

  Builds HEST table from the linked list carrying HEST header and HEST error
  source descriptor information and installs it.

  @retval EFI_SUCCESS    HEST table is installed successfully.
  @retval EFI_NOT_FOUND  List is NULL.
  @retval Other          Install interface call failed.
**/
STATIC
EFI_STATUS
EFIAPI
InstallHestAcpiTable (
  VOID
  )
{
  EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *HestHeaderPtr;
  HEST_DXE_DRIVER_DATA                             *HestDataList;
  EFI_STATUS                                       Status;
  LIST_ENTRY                                       *Link;
  UINTN                                            AcpiTableHandle;
  UINTN                                            HestTableSize;
  VOID                                             *HestTable;

  //
  // Check if the list is empty. If the list is empty there are no error
  // sources supported by the platform and no HEST table to publish, return.
  //
  if (IsListEmpty (&gHestErrorSourceList)) {
    DEBUG ((
      DEBUG_INFO,
      "HestDxe: No data available to generate HEST table\n"
      ));
    return EFI_NOT_FOUND;
  }

  //
  // Create a HEST table from the error source descriptor list nodes.
  //
  // Get the size of entire HEST table from the HEST header list node.
  Link          = GetFirstNode (&gHestErrorSourceList);
  HestDataList  = BASE_CR (Link, HEST_DXE_DRIVER_DATA, Link);
  HestHeaderPtr = (EFI_ACPI_6_4_HARDWARE_ERROR_SOURCE_TABLE_HEADER *)
                  HestDataList->HestTableData;
  HestTableSize = HestHeaderPtr->Header.Length;

  // Allocate sufficient memory to hold HEST table.
  HestTable = AllocateZeroPool (HestTableSize);

  // Loop over the list and create a HEST table from all the list nodes.
  while (!IsNull (&gHestErrorSourceList, Link)) {
    Link         = GetFirstNode (&gHestErrorSourceList);
    HestDataList = BASE_CR (Link, HEST_DXE_DRIVER_DATA, Link);
    CopyMem (
      HestTable,
      HestDataList->HestTableData,
      HestDataList->DataLength
      );
    HestTable += HestDataList->DataLength;

    // Free List HEST data nodes.
    Link = RemoveEntryList (Link);
    FreePool (HestDataList->HestTableData);
    // FreePool (HestDataList);
  }

  HestTable -= HestTableSize;
  // Install the HEST table.
  Status = gAcpiTableProtocol->InstallAcpiTable (
                                 gAcpiTableProtocol,
                                 HestTable,
                                 HestTableSize,
                                 &AcpiTableHandle
                                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: HEST table installation failed, status: %r\n",
      __FUNCTION__,
      Status
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "HestDxe: Installed HEST table \n"
      ));
  }

  // Free the HEST table buffer.
  FreePool (HestTable);

  return Status;
}

//
// HEST table generation protocol instance.
//
STATIC EDKII_HEST_TABLE_PROTOCOL  mHestProtocol = {
  AddErrorSourceDescriptor,
  InstallHestAcpiTable
};

/**
  The Entry Point for HEST Dxe driver.

  This function installs the HEST table protocol.

  @param[in] ImageHandle  Handle to the Efi image.
  @param[in] SystemTable  A pointer to the Efi System Table.

  @retval EFI_SUCCESS  On successful installation of protocol interfaces and
                       location the ACPI table protocol.
  @retval Other        On Failure to locate ACPI table protocol or install
                       of HEST table generation protocol.
**/
EFI_STATUS
EFIAPI
HestInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HANDLE  Handle = NULL;
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&gAcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to locate ACPI table protocol, status: %r\n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gHestTableProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mHestProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to install HEST table generation protocol status: %r\n",
      __FUNCTION__,
      Status
      ));
  }

  return Status;
}
