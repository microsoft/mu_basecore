/** @file MockAcpiSystemDescriptionTable.h
  This file declares a mock of ACPI system description tables protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ACPI_SYSTEM_DESCRIPTION_TABLE_H
#define MOCK_ACPI_SYSTEM_DESCRIPTION_TABLE_H

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/AcpiSystemDescriptionTable.h>
}

struct MockAcpiSdtProtocol {
  MOCK_INTERFACE_DECLARATION (MockAcpiSdtProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetAcpiTable,
    (
     IN    UINTN                   Index,
     OUT   EFI_ACPI_SDT_HEADER     **Table,
     OUT   EFI_ACPI_TABLE_VERSION  *Version,
     OUT   UINTN                   *TableKey
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RegisterNotify,
    (
     IN BOOLEAN                    Register,
     IN EFI_ACPI_NOTIFICATION_FN   Notification
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Open,
    (
     IN    VOID            *Buffer,
     OUT   EFI_ACPI_HANDLE *Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    OpenSdt,
    (
     IN    UINTN           TableKey,
     OUT   EFI_ACPI_HANDLE *Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Close,
    (
     IN EFI_ACPI_HANDLE Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetChild,
    (
     IN EFI_ACPI_HANDLE        ParentHandle,
     IN OUT EFI_ACPI_HANDLE    *Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetOption,
    (
     IN        EFI_ACPI_HANDLE     Handle,
     IN        UINTN               Index,
     OUT       EFI_ACPI_DATA_TYPE  *DataType,
     OUT CONST VOID                **Data,
     OUT       UINTN               *DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetOption,
    (
     IN        EFI_ACPI_HANDLE Handle,
     IN        UINTN           Index,
     IN CONST  VOID            *Data,
     IN        UINTN           DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FindPath,
    (
     IN    EFI_ACPI_HANDLE HandleIn,
     IN    VOID            *AcpiPath,
     OUT   EFI_ACPI_HANDLE *HandleOut
    )
    );
};

extern "C" {
  extern EFI_ACPI_SDT_PROTOCOL  *gAcpiSdtProtocol;
}

#endif // ACPI_SYSTEM_DESCRIPTION_TABLE_H
