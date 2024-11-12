/** @file MockFirmwareVolumeBlock.h
  This file declares a mock of Firmware Volume Block Protocol

  Copyright (c) Microsoft Corporation.
  Your use of this software is governed by the terms of the Microsoft agreement under which you obtained the software.
**/

#ifndef MOCK_FIRMWARE_VOLUME_BLOCK_H_
#define MOCK_FIRMWARE_VOLUME_BLOCK_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/FirmwareVolumeBlock.h>
}

//
// Declarations to handle usage of the FirmwareVolumeBlockProtocol
//
struct MockFirmwareVolumeBlockProtocol {
  MOCK_INTERFACE_DECLARATION (MockFirmwareVolumeBlockProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetAttributes,
    (
      IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
      OUT       EFI_FVB_ATTRIBUTES_2* Attributes)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetAttributes,
    (
      IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
      IN OUT    EFI_FVB_ATTRIBUTES_2* Attributes)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetPhysicalAddress,
    (
      IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
      OUT       EFI_PHYSICAL_ADDRESS* Address)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetBlockSize,
    (
      IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
      IN        EFI_LBA                             Lba,
      OUT       UINTN* BlockSize,
      OUT       UINTN* NumberOfBlocks)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Read,
    (
      IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
      IN        EFI_LBA                             Lba,
      IN        UINTN                               Offset,
      IN OUT    UINTN* NumBytes,
      IN OUT    UINT8* Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Write,
    (
      IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
      IN        EFI_LBA                             Lba,
      IN        UINTN                               Offset,
      IN OUT    UINTN* NumBytes,
      IN        UINT8* Buffer)
    );
};

MOCK_INTERFACE_DEFINITION (MockFirmwareVolumeBlockProtocol);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolumeBlockProtocol, GetAttributes, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolumeBlockProtocol, SetAttributes, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolumeBlockProtocol, GetPhysicalAddress, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolumeBlockProtocol, GetBlockSize, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolumeBlockProtocol, Read, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolumeBlockProtocol, Write, 5, EFIAPI);

EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  FIRMWARE_VOLUME_BLOCK_PROTOCOL_MOCK = {
  GetAttributes,           // EFI_FVB_GET_ATTRIBUTES          GetAttributes;
  SetAttributes,           // EFI_FVB_SET_ATTRIBUTES          SetAttributes;
  GetPhysicalAddress,      // EFI_FVB_GET_PHYSICAL_ADDRESS    GetPhysicalAddress;
  GetBlockSize,            // EFI_FVB_GET_BLOCK_SIZE          GetBlockSize;
  Read,                    // EFI_FVB_READ                    Read;
  Write                    // EFI_FVB_WRITE                   Write;
};

extern "C" {
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *Fvb = &FIRMWARE_VOLUME_BLOCK_PROTOCOL_MOCK;
}

#endif // MOCK_FIRMWARE_VOLUME_BLOCK_H_
