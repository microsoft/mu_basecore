/** @file MockAcpiSystemDescriptionTable.cpp
  Google Test mock for ACPI system description tables protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockAcpiSystemDescriptionTable.h>

MOCK_INTERFACE_DEFINITION (MockAcpiSdtProtocol);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, GetAcpiTable, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, RegisterNotify, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, Open, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, OpenSdt, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, Close, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, GetChild, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, GetOption, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, SetOption, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAcpiSdtProtocol, FindPath, 3, EFIAPI);

EFI_ACPI_SDT_PROTOCOL  ACPI_SDT_PROTOCOL_INSTANCE = {
  0,
  GetAcpiTable,   // EFI_ACPI_TABLE_INSTALL_ACPI_TABLE
  RegisterNotify, // EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE
  Open,
  OpenSdt,
  Close,
  GetChild,
  GetOption,
  SetOption,
  FindPath
};

extern "C" {
  EFI_ACPI_SDT_PROTOCOL  *gAcpiSdtProtocol = &ACPI_SDT_PROTOCOL_INSTANCE;
}
