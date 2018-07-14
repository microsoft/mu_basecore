/**

Copyright (c) 2017, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This library provides helper functions to create performance timing events.

**/

#include <Base.h>
#include <Guid/EventGroup.h>
#include <Guid/ZeroGuid.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/LocalApicLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/Performance2Lib.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/Performance2.h>
#include <Guid/PeiPerformanceHob.h>
#include "CorePerformance2Lib.h"

#define FIRMWARE_RECORD_BUFFER             0x10000
UINT8    *mPerformancePointer  = NULL;
UINTN    mPerformanceLength    = 0;
UINTN    mMaxPerformanceLength = 0;
BOOLEAN  mFpdtDataIsReported   = FALSE;
CHAR8    *mPlatformLanguage = NULL;

/**
  Get a module guid for the given image handle.
  If module guid can't be found, gZeroGuid will be returned.

  @param Handle     - Image handle or Controller handle.
  @param ModuleGuid - Point to the guid buffer to store the got module guid value.

  @retval EFI_SUCCESS           - Successfully get module name and guid.
  @retval EFI_INVALID_PARAMETER - Handle is a NULL pointer

**/
EFI_STATUS
GetModuleGuidFromHandle (
  IN EFI_HANDLE        Handle,
  OUT EFI_GUID         *ModuleGuid
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  EFI_GUID                    *TempGuid;
  BOOLEAN                     ModuleGuidIsGet;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FvFilePath;
  UINT64                      Signature;

  Status = EFI_INVALID_PARAMETER;
  LoadedImage     = NULL;
  ModuleGuidIsGet = FALSE;

  //
  // Initialize GUID as zero value.
  //
  TempGuid    = &gZeroGuid;

  if (Handle != NULL) {
    //
    // Try Handle as ImageHandle.
    //
    Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID**) &LoadedImage
                  );

    if (EFI_ERROR (Status)) {
      //
      // Try Handle as Controller Handle
      //
      Status = gBS->OpenProtocol (
                    Handle,
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **) &DriverBinding,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
      if (!EFI_ERROR (Status)) {
        //
        // Get Image protocol from ImageHandle
        //
        Status = gBS->HandleProtocol (
                      DriverBinding->ImageHandle,
                      &gEfiLoadedImageProtocolGuid,
                      (VOID**) &LoadedImage
                      );
      }
    }
  }

  if (!EFI_ERROR (Status) && LoadedImage != NULL) {
    //
    // Get Module Guid from DevicePath.
    //
    if (LoadedImage->FilePath != NULL &&
        LoadedImage->FilePath->Type == MEDIA_DEVICE_PATH &&
        LoadedImage->FilePath->SubType == MEDIA_PIWG_FW_FILE_DP
       ) {
      //
      // Determine GUID associated with module logging performance
      //
      ModuleGuidIsGet = TRUE;
      FvFilePath      = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LoadedImage->FilePath;
      TempGuid        = &FvFilePath->FvFileName;
    }
  }

  //
  // Copy Module Guid
  //
  if (ModuleGuid != NULL) {
    CopyMem (ModuleGuid, TempGuid, sizeof(EFI_GUID));
    if (CompareGuid(TempGuid, &gZeroGuid) && (Handle != NULL) && !ModuleGuidIsGet) {
        //
        // Copy a signature string UNKNHNDL into upper 8 bytes and the handle address into lower
        //
        *((UINT64 *)ModuleGuid) = (UINT64) Handle;
        Signature = SIGNATURE_64 ('U', 'N', 'K', 'N', 'H', 'N', 'D', 'L');
        CopyMem (((UINT8 *)ModuleGuid) + sizeof(UINT64), &Signature, sizeof(Signature));
    }
  }

  return Status;
} // GetModuleGuidFromHandle

/**
  Get a string description for device for the given controller handle and update record
  length. If ComponentName2 GetControllerName is supported, the value is included in the string,
  followed by device path, otherwise just device path.

  @param Handle              - Image handle
  @param ControllerHandle    - Controller handle.
  @param ComponentNameString - Pointer to a location where the string will be saved
  @param Length              - Pointer to record length to be updated

  @retval EFI_SUCCESS     - Successfully got string description for device
  @retval EFI_UNSUPPORTED - Neither ComponentName2 ControllerName nor DevicePath were found

**/
EFI_STATUS
GetDeviceInfoFromHandleAndUpdateLength (
  IN CONST VOID        *Handle,
  IN EFI_HANDLE        ControllerHandle,
  OUT CHAR8            *ComponentNameString,
  IN OUT UINT8         *Length
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *DevicePathProtocol;
  EFI_COMPONENT_NAME2_PROTOCOL  *ComponentName2;
  EFI_STATUS                    Status;
  CHAR16                        *StringPtr = NULL;
  CHAR8                         *AsciiStringPtr;
  UINTN                         ControllerNameStringSize;
  UINTN                         DevicePathStringSize;

  ControllerNameStringSize = 0;

  Status = gBS->HandleProtocol (
                  (EFI_HANDLE) Handle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **) &ComponentName2
                  );

  if (!EFI_ERROR(Status)) {
    //
    // Get the current platform language setting
    //
    if (mPlatformLanguage == NULL) {
      GetEfiGlobalVariable2 (L"PlatformLang", &mPlatformLanguage, NULL);
    }

    Status = ComponentName2->GetControllerName (
                               ComponentName2,
                               ControllerHandle,
                               NULL,
                               mPlatformLanguage != NULL ? mPlatformLanguage : "en-US",
                               &StringPtr
                               );
  }

  if (!EFI_ERROR (Status)) {
    //
    // This will produce the size of the unicode string, which is twice as large as the ASCII one
    // This must be an even number, so ok to divide by 2
    //
    ControllerNameStringSize = StrSize(StringPtr) / 2;

    //
    // The + 1 is because we want to add a space between the ControllerName and the device path
    //
    if ((ControllerNameStringSize + (*Length) + 1) > MAX_PERF_RECORD_SIZE) {
      //
      // Only copy enough to fill MAX_PERF_RECORD_SIZE worth of the record
      //
      ControllerNameStringSize = MAX_PERF_RECORD_SIZE - (*Length) - 1;
    }

    UnicodeStrToAsciiStrS(StringPtr, ComponentNameString, ControllerNameStringSize);

    //
    // Add a space in the end of the ControllerName
    //
    AsciiStringPtr = ComponentNameString + ControllerNameStringSize - 1;
    *AsciiStringPtr = 0x20;
    AsciiStringPtr++;
    *AsciiStringPtr = 0;
    ControllerNameStringSize++;

    *Length += (UINT8)ControllerNameStringSize;
  }

  //
  // This function returns the device path protocol from the handle specified by Handle.  If Handle is
  // NULL or Handle does not contain a device path protocol, then NULL is returned.
  //
  DevicePathProtocol = DevicePathFromHandle(ControllerHandle);

  if (DevicePathProtocol != NULL) {
    StringPtr = ConvertDevicePathToText (DevicePathProtocol, TRUE, FALSE);
    if (StringPtr != NULL) {
      //
      // This will produce the size of the unicode string, which is twice as large as the ASCII one
      // This must be an even number, so ok to divide by 2
      //
      DevicePathStringSize = StrSize(StringPtr) / 2;

      if ((DevicePathStringSize + (*Length)) > MAX_PERF_RECORD_SIZE) {
        //
        // Only copy enough to fill MAX_PERF_RECORD_SIZE worth of the record
        //
        DevicePathStringSize = MAX_PERF_RECORD_SIZE - (*Length);
        if (DevicePathStringSize > 0) {
          StringPtr[DevicePathStringSize - 1] = 0;    // Terminate the source string to MAX length
        }

      }

      if (ControllerNameStringSize != 0) {
        AsciiStringPtr = ComponentNameString + ControllerNameStringSize - 1;
      } else {
        AsciiStringPtr = ComponentNameString;
      }

      UnicodeStrToAsciiStrS(StringPtr, AsciiStringPtr, DevicePathStringSize);
      *Length += (UINT8)DevicePathStringSize;

      FreePool (StringPtr);

      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
} // GetDeviceInfoFromHandleAndUpdateLength

/**
  Create performance record with event description and a timestamp.

  @param CallerIdentifier  - Image handle or pointer to caller ID GUID
  @param Guid    - Pointer to a GUID
  @param String  - Pointer to a string describing the measurement
  @param Address - Pointer to a location in memory relevant to the measurement
  @param PerfId  - Performance identifier describing the type of measurement

  @retval EFI_SUCCESS           - Successfully created performance record
  @retval EFI_OUT_OF_RESOURCES  - Ran out of space to store the records
  @retval EFI_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                  pointer or invalid PerfId

**/
EFI_STATUS
CreatePerformanceMeasurement (
  IN CONST VOID   *CallerIdentifier,  OPTIONAL
  IN CONST VOID   *Guid,    OPTIONAL
  IN CONST CHAR8  *String,  OPTIONAL
  IN UINT64       Address,  OPTIONAL
  IN UINT16       PerfId
  )
{
  EFI_GUID                         ModuleGuid;
  FPDT_RECORD_PTR                  FpdtRecordPtr;
  UINT64                           Ticker;
  PEI_CORE_PERFORMANCE_HOB_HEADER  *PerfHobHeader;
  STATIC UINT16                    LoadImageCount = 0;
  EFI_HOB_GUID_TYPE                *GuidHob;

  //
  // On first entry into this function let's initialize the LoadImageCount
  // by getting the value from the PEI library
  //
  if (LoadImageCount == 0) {
    GuidHob = GetFirstGuidHob (&gPeiPerformanceHobGuid);
    if (GuidHob != NULL) {
      PerfHobHeader = GET_GUID_HOB_DATA (GuidHob);
      if (PerfHobHeader->Signature == PEI_CORE_PERF_HOB_SIG) {
        LoadImageCount = PerfHobHeader->LoadImageCount;
      }
    }
  }

  if (mFpdtDataIsReported) {
    //
    // Cached FPDT data has been reported. Now, report FPDT record one by one.
    //
    FpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *) mPerformancePointer;
    if (mPerformancePointer == NULL) {
      DEBUG((DEBUG_ERROR, "DxeCorePerformanceLib - %a - Out of space\n", __FUNCTION__));
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // Check if pre-allocated buffer is full
    //
    if (mPerformanceLength + MAX_PERF_RECORD_SIZE > mMaxPerformanceLength) {
      mPerformancePointer = ReallocatePool (
                              mPerformanceLength,
                              mPerformanceLength + MAX_PERF_RECORD_SIZE + FIRMWARE_RECORD_BUFFER,
                              mPerformancePointer
                              );

      if (mPerformancePointer == NULL) {
        DEBUG((DEBUG_ERROR, "DxeCorePerformanceLib - %a - Out of space\n", __FUNCTION__));
        ASSERT(FALSE);
        return EFI_OUT_OF_RESOURCES;
      }

      mMaxPerformanceLength = mPerformanceLength + MAX_PERF_RECORD_SIZE + FIRMWARE_RECORD_BUFFER;
    }

    //
    // Convert buffer to FPDT Ptr Union type.
    //
    FpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mPerformancePointer + mPerformanceLength);
  }
  FpdtRecordPtr.RecordHeader->Length = 0;

  //
  // We'll need the ticker for all record types
  //
  Ticker = GetPerformanceCounter();

  //
  // This is the same for all record types
  //
  FpdtRecordPtr.RecordHeader->Revision = RECORD_REVISION_1;

  switch (PerfId) {
    case (PERF_ENTRYPOINT_BEGIN_ID):
    case (PERF_ENTRYPOINT_END_ID):
      //
      // GUID event or Handle event
      //
      if (CallerIdentifier == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      //
      // GUID event
      //
      FpdtRecordPtr.RecordHeader->Type = GUID_EVENT_TYPE;
      GetModuleGuidFromHandle((EFI_HANDLE) CallerIdentifier, &ModuleGuid);
      CopyMem(&FpdtRecordPtr.GuidEvent->Guid, &ModuleGuid, sizeof(EFI_GUID));

      FpdtRecordPtr.RecordHeader->Length  = sizeof(GUID_EVENT_RECORD);
      FpdtRecordPtr.GuidEvent->ProgressID = PerfId;
      FpdtRecordPtr.GuidEvent->ApicID     = GetApicId();
      FpdtRecordPtr.GuidEvent->Timestamp  = GetTimeInNanoSecond(Ticker);
      break;

    case (PERF_LOADIMAGE_BEGIN_ID):
      //
      // GUID Qword event with an unpopulated GUID field
      //
      FpdtRecordPtr.RecordHeader->Type          = GUID_QWORD_EVENT_TYPE;
      FpdtRecordPtr.RecordHeader->Length        = sizeof(GUID_QWORD_EVENT_RECORD);
      FpdtRecordPtr.GuidQwordEvent->ProgressID  = PerfId;
      FpdtRecordPtr.GuidQwordEvent->ApicID      = GetApicId();
      FpdtRecordPtr.GuidQwordEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      LoadImageCount++;
      FpdtRecordPtr.GuidQwordEvent->Qword       = LoadImageCount;
      break;

    case (PERF_LOADIMAGE_END_ID):
      //
      // GUID Qword event or Handle Qword event
      //
      if (CallerIdentifier == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      //
      // GUID Qword event
      //
      FpdtRecordPtr.RecordHeader->Type = GUID_QWORD_EVENT_TYPE;
      GetModuleGuidFromHandle((EFI_HANDLE) CallerIdentifier, &ModuleGuid);
      CopyMem(&FpdtRecordPtr.GuidQwordEvent->Guid, &ModuleGuid, sizeof(EFI_GUID));

      FpdtRecordPtr.RecordHeader->Length        = sizeof(GUID_QWORD_EVENT_RECORD);
      FpdtRecordPtr.GuidQwordEvent->ProgressID  = PerfId;
      FpdtRecordPtr.GuidQwordEvent->ApicID      = GetApicId();
      FpdtRecordPtr.GuidQwordEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      FpdtRecordPtr.GuidQwordEvent->Qword       = LoadImageCount;
      break;

    case (PERF_BINDING_SUPPORT_BEGIN_ID):
    case (PERF_BINDING_SUPPORT_END_ID):
    case (PERF_BINDING_START_BEGIN_ID):
    case (PERF_BINDING_STOP_BEGIN_ID):
    case (PERF_BINDING_STOP_END_ID):
      //
      // GUID Qword event or Handle Qword event
      //
      if (CallerIdentifier == NULL || Address == 0) {
        return EFI_INVALID_PARAMETER;
      }

      //
      // GUID Qword event
      //
      FpdtRecordPtr.RecordHeader->Type = GUID_QWORD_EVENT_TYPE;
      GetModuleGuidFromHandle((EFI_HANDLE) CallerIdentifier, &ModuleGuid);
      CopyMem (&FpdtRecordPtr.GuidQwordEvent->Guid, &ModuleGuid, sizeof(EFI_GUID));

      FpdtRecordPtr.RecordHeader->Length        = sizeof(GUID_QWORD_EVENT_RECORD);
      FpdtRecordPtr.GuidQwordEvent->ProgressID  = PerfId;
      FpdtRecordPtr.GuidQwordEvent->ApicID      = GetApicId();
      FpdtRecordPtr.GuidQwordEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      FpdtRecordPtr.GuidQwordEvent->Qword       = Address;
      break;

    case (PERF_BINDING_START_END_ID):
      //
      // GUID Qword String event or Handle Qword String event
      //
      if (CallerIdentifier == NULL || Address == 0) {
        return EFI_INVALID_PARAMETER;
      }

      //
      // GUID Qword String event
      //
      FpdtRecordPtr.RecordHeader->Type = GUID_QWORD_STRING_EVENT_TYPE;
      GetModuleGuidFromHandle ((EFI_HANDLE) CallerIdentifier, &ModuleGuid);
      CopyMem(&FpdtRecordPtr.GuidQwordStringEvent->Guid, &ModuleGuid, sizeof(EFI_GUID));

      FpdtRecordPtr.GuidQwordStringEvent->ProgressID  = PerfId;
      FpdtRecordPtr.GuidQwordStringEvent->ApicID      = GetApicId();
      FpdtRecordPtr.GuidQwordStringEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      FpdtRecordPtr.GuidQwordStringEvent->Qword       = Address;
      FpdtRecordPtr.RecordHeader->Length              = sizeof(GUID_QWORD_STRING_EVENT_RECORD);
      // In IA32 casting UINT64 param Address into EFI handle causes a warning
      // This is ok because PERF_BINDING_START_END is called with EFI_HANDLE-type parameter
      #pragma warning(suppress: 4305)
      GetDeviceInfoFromHandleAndUpdateLength(CallerIdentifier, (EFI_HANDLE) Address, FpdtRecordPtr.GuidQwordStringEvent->String, &FpdtRecordPtr.RecordHeader->Length);
      break;

    case (PERF_EVENTSIGNAL_BEGIN_ID):
    case (PERF_EVENTSIGNAL_END_ID):
    case (PERF_CALLBACK_BEGIN_ID):
    case (PERF_CALLBACK_END_ID):
      //
      // Dual GUID String event
      //
      if (Guid == 0 || String == NULL || CallerIdentifier == NULL) {
        return EFI_INVALID_PARAMETER;
      }
      FpdtRecordPtr.RecordHeader->Type = DUAL_GUID_STRING_EVENT_TYPE;
      CopyMem(&FpdtRecordPtr.DualGuidStringEvent->Guid1, Guid, sizeof(EFI_GUID));
      CopyMem(&FpdtRecordPtr.DualGuidStringEvent->Guid2, CallerIdentifier, sizeof(EFI_GUID));
      FpdtRecordPtr.DualGuidStringEvent->ProgressID  = PerfId;
      FpdtRecordPtr.DualGuidStringEvent->ApicID      = GetApicId();
      FpdtRecordPtr.DualGuidStringEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      FpdtRecordPtr.RecordHeader->Length             = sizeof(DUAL_GUID_STRING_EVENT_RECORD);
      CopyStringIntoPerfRecordAndUpdateLength(FpdtRecordPtr.DualGuidStringEvent->String,
                                              String,
                                              &FpdtRecordPtr.RecordHeader->Length
                                              );
      break;

    case (PERF_EVENT_ID):
    case (PERF_FUNCTION_BEGIN_ID):
    case (PERF_FUNCTION_END_ID):
    case (PERF_INMODULE_BEGIN_ID):
    case (PERF_INMODULE_END_ID):
    case (PERF_CROSSMODULE_BEGIN_ID):
    case (PERF_CROSSMODULE_END_ID):
      //
      // Dynamic String Event
      //
      if (String == NULL || CallerIdentifier == NULL) {
        return EFI_INVALID_PARAMETER;
      }
      FpdtRecordPtr.RecordHeader->Type = DYNAMIC_STRING_EVENT_TYPE;
      CopyMem(&FpdtRecordPtr.DynamicStringEvent->Guid, CallerIdentifier, sizeof(EFI_GUID));
      FpdtRecordPtr.DynamicStringEvent->ProgressID  = PerfId;
      FpdtRecordPtr.DynamicStringEvent->ApicID      = GetApicId();
      FpdtRecordPtr.DynamicStringEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      FpdtRecordPtr.RecordHeader->Length            = sizeof(DYNAMIC_STRING_EVENT_RECORD);
      CopyStringIntoPerfRecordAndUpdateLength(FpdtRecordPtr.DynamicStringEvent->String,
                                              String,
                                              &FpdtRecordPtr.RecordHeader->Length
                                              );
      break;

    default:
      DEBUG((DEBUG_ERROR, "DxeCorePerformanceLib - %a - unsupported PerfId 0x%x!\n", __FUNCTION__, PerfId));
      return EFI_INVALID_PARAMETER;
  }

  //
  // Report record one by one after records have been reported together.
  //
  if (mFpdtDataIsReported) {
    REPORT_STATUS_CODE_EX (
      EFI_PROGRESS_CODE,
      EFI_SOFTWARE_DXE_BS_DRIVER,
      0,
      NULL,
      &gEfiFirmwarePerformanceGuid,
      FpdtRecordPtr.RecordHeader,
      FpdtRecordPtr.RecordHeader->Length
      );
  } else {
    //
    // Update the cached FPDT record buffer.
    //
    ASSERT(FpdtRecordPtr.RecordHeader->Length > 0);
    mPerformanceLength += FpdtRecordPtr.RecordHeader->Length;
  }

  return EFI_SUCCESS;
} // CreatePerformanceMeasurement

/**
  End of DXE callback that:
   1. Unpacks the PEI HOB with perf records and reports its contents as a status code
   2. Converts all handle-type records to GUID-type records
   3. Reports all cached records as status codes, in 64K blocks if necessary
   4. Sets mFpdtDataIsReported to TRUE

  @param  Event    The event of notify protocol.
  @param  Context  Notify event context.

**/
VOID
EFIAPI
ReportBufferedRecords (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  UINT8                            *PerfBuffer;
  PEI_CORE_PERFORMANCE_HOB_HEADER  *FirmwarePerformanceHob;
  EFI_HOB_GUID_TYPE                *GuidHob;

  GuidHob = GetFirstGuidHob (&gPeiPerformanceHobGuid);
  if (GuidHob != NULL) {
    FirmwarePerformanceHob = GET_GUID_HOB_DATA (GuidHob);
    if (FirmwarePerformanceHob->Signature != PEI_CORE_PERF_HOB_SIG) {
      DEBUG((DEBUG_ERROR, "DxeCorePerformanceLib - %a - Perf HOB unexpected signature %x\n", __FUNCTION__, FirmwarePerformanceHob->Signature));
    } else {
      //
      // No need to check HobLength - it can't be more than 64k, HOB max size
      //
      DEBUG((DEBUG_INFO, "DxeCorePerformanceLib - %a - Perf HOB length: 0x%x\n", __FUNCTION__, FirmwarePerformanceHob->HobLength));

      //
      // Don't report the HOB header, just the perf data following it
      //
      PerfBuffer = ((UINT8*) FirmwarePerformanceHob) + sizeof(PEI_CORE_PERFORMANCE_HOB_HEADER);
      REPORT_STATUS_CODE_EX (
        EFI_PROGRESS_CODE,
        EFI_SOFTWARE_DXE_BS_DRIVER,
        0,
        NULL,
        &gEfiFirmwarePerformanceGuid,
        PerfBuffer,
        FirmwarePerformanceHob->HobLength - sizeof(PEI_CORE_PERFORMANCE_HOB_HEADER)
        );
    }
  }

  PerfBuffer = mPerformancePointer;

  if (mPerformanceLength > 0) {
    while (mPerformanceLength > MAX_UINT16) {
      //
      // Report extension data size is UINT16. So, the size of report data can't exceed MAX_UINT16.
      //
      REPORT_STATUS_CODE_EX (
        EFI_PROGRESS_CODE,
        EFI_SOFTWARE_DXE_BS_DRIVER,
        0,
        NULL,
        &gEfiFirmwarePerformanceGuid,
        PerfBuffer,
        MAX_UINT16
        );
      mPerformanceLength = mPerformanceLength - MAX_UINT16;
      PerfBuffer         = PerfBuffer + MAX_UINT16;
    }

    REPORT_STATUS_CODE_EX (
      EFI_PROGRESS_CODE,
      EFI_SOFTWARE_DXE_BS_DRIVER,
      0,
      NULL,
      &gEfiFirmwarePerformanceGuid,
      PerfBuffer,
      mPerformanceLength
      );
  }

  //
  // Free the local buffer we've been using for caching records until now
  //
  FreePool (mPerformancePointer);

  //
  // Allocate enough space to store just one record
  // We are not going to FreePool this allocation, it'll be released at ExitBootServices
  //
  mPerformancePointer = AllocatePool(MAX_PERF_RECORD_SIZE);
  if (mPerformancePointer == NULL) {
    DEBUG((DEBUG_ERROR, "DxeCorePerformanceLib - %a - Out of space\n", __FUNCTION__));
  }

  //
  // The below globals won't be used once mFpdtDataIsReported is set to TRUE
  //
  mPerformanceLength    = 0;
  mMaxPerformanceLength = 0;

  //
  // Set FPDT report state to TRUE.
  //
  mFpdtDataIsReported = TRUE;
} // ReportBufferedRecords

//
// Interface for the Performance2 Protocol
//
PERFORMANCE_2_PROTOCOL mPerformance2Interface = {
  CreatePerformanceMeasurement
  };

/**
  Library constructor:
   1. Checks if performance tracing is enabled via the PCD, if not returns
   2. Installs gPerformance2ProtocolGuid
   3. Creates event notify to call ReportBufferedRecords at EndOfDxe

  @param ImageHandle - DxeMain image handle.
  @param SystemTable - Pointer to system table.

  @retval EFI_SUCCESS - On success

**/
EFI_STATUS
EFIAPI
DxeCorePerformanceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS   Status;
  EFI_HANDLE   Handle;
  EFI_EVENT    EndOfDxeEvent;

  if (GetPerformanceVerbosityLevel () == 0) {
    //
    // Do not initialize performance infrastructure if not required.
    //
    return EFI_SUCCESS;
  }

  //
  // Install the protocol interface for DXE performance library instance.
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gPerformance2ProtocolGuid,
                  &mPerformance2Interface,
                  NULL
                  );

  //
  // Register End of DXE event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  ReportBufferedRecords,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );

  return Status;
} // DxeCorePerformanceLibConstructor

/**
  Create performance record with event description and a timestamp.
  This function is here so that perf calls from the DXE core will be able to work.

  @param CallerIdentifier  - Image handle or caller ID GUID
  @param Guid    - Pointer to a GUID
  @param String  - Pointer to a string describing the measurement
  @param Address - Pointer to a location in memory relevant to the measurement
  @param PerfId  - Performance identifier describing the type of measurement

  @retval RETURN_SUCCESS           - Successfully created performance record
  @retval RETURN_OUT_OF_RESOURCES  - Ran out of space to store the records
  @retval RETURN_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                     pointer or invalid PerfId

**/
RETURN_STATUS
EFIAPI
LogPerformanceMeasurement (
  IN CONST VOID   *CallerIdentifier,  OPTIONAL
  IN CONST VOID   *Guid,    OPTIONAL
  IN CONST CHAR8  *String,  OPTIONAL
  IN UINT64       Address,  OPTIONAL
  IN UINT16       PerfId
  )
{
  return (RETURN_STATUS) CreatePerformanceMeasurement(CallerIdentifier, Guid, String, Address, PerfId);
}

/**
  Returns performance logging verbosity obtained from
  PcdPerformance2LibraryVerbosityLevel.

  @retval - 0 - Performance logging turned off.
  @retval - 1 - Low verbosity performance logging.
  @retval - 2 - Standard verbosity performance logging.
  @retval - 3 - High verbosity performance logging.
**/
UINT8
EFIAPI
GetPerformanceVerbosityLevel (
  VOID
  )
{
  return (UINT8) PcdGet8(PcdPerformance2LibraryVerbosityLevel);
}

/**
  Returns performnce library's core functionality mask with bits defined as:
    BIT0 - 1 - Enable Entrypoint Logging.
           0 - Disable Entrypoint Logging.
    BIT1 - 1 - Enable Load Image logging.
           0 - Disable Load Image Logging.
    BIT2 - 1 - Enable Binding Support logging.
           0 - Disable Binding Support Logging.
    BIT3 - 1 - Enable Binding Start logging.
           0 - Disable Binding Start Logging.
    BIT4 - 1 - Enable Binding Stop logging.
           0 - Disable Binding Stop Logging.
    Bits 7-5 - Reserved
  The mask is obtained from PcdPerformance2LibraryCoreFunctionalityMask.

  @retval - Performnce library's core functionality mask.
**/
UINT8
EFIAPI
GetPerformanceCoreFunctionalityMask (
  VOID
  )
{
  return (UINT8) PcdGet8(PcdPerformance2LibraryCoreFunctionalityMask);
}
