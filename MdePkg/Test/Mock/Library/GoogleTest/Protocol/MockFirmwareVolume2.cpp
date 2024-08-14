/** @file MockFirmwareVolume2.cpp
  Google Test mock for Firmware Volume Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockFirmwareVolume2.h>

MOCK_INTERFACE_DEFINITION (MockFirmwareVolume2Protocol);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, GetVolumeAttributes, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, SetVolumeAttributes, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, FV_ReadFile, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, ReadSection, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, FV_WriteFile, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, GetNextFile, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, GetInfo, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, SetInfo, 4, EFIAPI);

EFI_FIRMWARE_VOLUME2_PROTOCOL  FIRMWARE_VOLUME2_PROTOCOL_MOCK = {
  GetVolumeAttributes,    // EFI_FV_GET_ATTRIBUTES    GetVolumeAttributes;
  SetVolumeAttributes,    // EFI_FV_SET_ATTRIBUTES    SetVolumeAttributes;
  FV_ReadFile,            // EFI_FV_READ_FILE         ReadFile;
  ReadSection,            // EFI_FV_READ_SECTION      ReadSection;
  FV_WriteFile,           // EFI_FV_WRITE_FILE        WriteFile;
  GetNextFile,            // EFI_FV_GET_NEXT_FILE     GetNextFile;
  0,                      // UINT32                   KeySize;
  0,                      // EFI_HANDLE               ParentHandle;
  GetInfo,                // EFI_FV_GET_INFO          GetInfo;
  SetInfo                 // EFI_FV_SET_INFO          SetInfo;
};

extern "C" {
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *gFirmwareVolume2Protocol = &FIRMWARE_VOLUME2_PROTOCOL_MOCK;
}
