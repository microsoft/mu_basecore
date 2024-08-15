/** @file MockAcpiTable.h
  This file declares a mock of Acpi table protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ACPI_TABLE_H_
#define MOCK_ACPI_TABLE_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/AcpiTable.h>
}

struct MockAcpiTableProtocol {
  MOCK_INTERFACE_DECLARATION (MockAcpiTableProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    InstallAcpiTable,
    (
     IN   EFI_ACPI_TABLE_PROTOCOL       *This,
     IN   VOID                          *AcpiTableBuffer,
     IN   UINTN                         AcpiTableBufferSize,
     OUT  UINTN                         *TableKey
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    UninstallAcpiTable,
    (
     IN  EFI_ACPI_TABLE_PROTOCOL       *This,
     IN  UINTN                         TableKey
    )
    );
};

extern "C" {
  extern EFI_ACPI_TABLE_PROTOCOL  *gAcpiTableProtocol;
}

#endif // MOCK_ACPI_TABLE_H_
