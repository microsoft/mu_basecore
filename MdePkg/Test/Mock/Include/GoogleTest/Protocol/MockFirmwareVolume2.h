/** @file MockFirmwareVolume2.h
  This file declares a mock of Firmware Volume Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_FIRMWARE_VOLUME2_H
#define MOCK_FIRMWARE_VOLUME2_H

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Pi/PiFirmwareVolume.h>
  #include <Pi/PiFirmwareFile.h>
  #include <Protocol/FirmwareVolume2.h>
}

struct MockFirmwareVolume2Protocol {
  MOCK_INTERFACE_DECLARATION (MockFirmwareVolume2Protocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetVolumeAttributes,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     OUT       EFI_FV_ATTRIBUTES             *FvAttributes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetVolumeAttributes,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN OUT    EFI_FV_ATTRIBUTES             *FvAttributes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FV_ReadFile,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN CONST  EFI_GUID                      *NameGuid,
     IN OUT    VOID                          **Buffer,
     IN OUT    UINTN                         *BufferSize,
     OUT       EFI_FV_FILETYPE               *FoundType,
     OUT       EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
     OUT       UINT32                        *AuthenticationStatus
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReadSection,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN CONST  EFI_GUID                      *NameGuid,
     IN        EFI_SECTION_TYPE              SectionType,
     IN        UINTN                         SectionInstance,
     IN OUT    VOID                          **Buffer,
     IN OUT    UINTN                         *BufferSize,
     OUT       UINT32                        *AuthenticationStatus
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FV_WriteFile,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN        UINT32                        NumberOfFiles,
     IN        EFI_FV_WRITE_POLICY           WritePolicy,
     IN        EFI_FV_WRITE_FILE_DATA        *FileData
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetNextFile,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN OUT    VOID                          *Key,
     IN OUT    EFI_FV_FILETYPE               *FileType,
     OUT       EFI_GUID                      *NameGuid,
     OUT       EFI_FV_FILE_ATTRIBUTES        *Attributes,
     OUT       UINTN                         *Size
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetInfo,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN CONST  EFI_GUID                      *InformationType,
     IN OUT    UINTN                         *BufferSize,
     OUT       VOID                          *Buffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetInfo,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN CONST  EFI_GUID                      *InformationType,
     IN        UINTN                         BufferSize,
     IN CONST  VOID                          *Buffer
    )
    );
};

extern "C" {
  extern EFI_FIRMWARE_VOLUME2_PROTOCOL  *gFirmwareVolume2Protocol;
}

#endif // FIRMWARE_VOLUME2_H
