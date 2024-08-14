/** @file MockAcpiTable.cpp
  Google Test mock for Acpi Table Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockAcpiTable.h>

MOCK_INTERFACE_DEFINITION (MockAcpiTableProtocol);
MOCK_FUNCTION_DEFINITION (MockAcpiTableProtocol, InstallAcpiTable, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiTableProtocol, UninstallAcpiTable, 2, EFIAPI);

EFI_ACPI_TABLE_PROTOCOL  ACPI_TABLE_PROTOCOL_INSTANCE = {
  InstallAcpiTable,   // EFI_ACPI_TABLE_INSTALL_ACPI_TABLE
  UninstallAcpiTable  // EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE
};

extern "C" {
  EFI_ACPI_TABLE_PROTOCOL  *gAcpiTableProtocol = &ACPI_TABLE_PROTOCOL_INSTANCE;
}
