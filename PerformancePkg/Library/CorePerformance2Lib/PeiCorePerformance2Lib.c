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

#include <PiPei.h>
#include <Base.h>
#include <Library/PeiServicesLib.h>
#include <Library/Performance2Lib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LocalApicLib.h>
#include <Guid/PeiPerformanceHob.h>
#include "CorePerformance2Lib.h"
#include <Ppi/PerformancePpi.h>

/**
  Create performance record with event description and a timestamp.
  This function is here so that PPI calls land in it

  @param This    - Pointer to this PERFORMANCE_PPI
  @param CallerIdentifier  - Image handle
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
CreatePerformanceMeasurementPpiCall (
  IN       PERFORMANCE_PPI  *This,
  IN CONST VOID             *CallerIdentifier,  OPTIONAL
  IN CONST VOID             *Guid,    OPTIONAL
  IN CONST CHAR8            *String,  OPTIONAL
  IN       UINT64           Address,  OPTIONAL
  IN       UINT16           PerfId
  );

PERFORMANCE_PPI           mPerformancePpi = {CreatePerformanceMeasurementPpiCall};

EFI_PEI_PPI_DESCRIPTOR    mPerformancePpiDesc = {
      (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
      &gPerformancePpiGuid,
      &mPerformancePpi};

/**
  Calculates perf record size.

  @param PerfId     - Performance identifier describing the type of measurement
  @param String     - Pointer to a string describing the measurement
  @param RecordSize - Pointer to a record size variable to be updated

  @retval EFI_SUCCESS           - Successfully calculcated record size
  @retval EFI_INVALID_PARAMETER - Invalid PerfId

**/
EFI_STATUS
GetRecordSize (
  IN         UINT16 PerfId,
  IN   CONST CHAR8  *String,    OPTIONAL
  OUT        UINTN  *RecordSize
  )
{
  UINTN StringSize;

  StringSize = 0;
  if (String != NULL) {
    StringSize = AsciiStrnSizeS(String, MAX_PERF_RECORD_SIZE);
  }

  switch (PerfId) {
    case (PERF_ENTRYPOINT_BEGIN_ID):
    case (PERF_ENTRYPOINT_END_ID):
      *RecordSize = sizeof(GUID_EVENT_RECORD);
      return EFI_SUCCESS;
    case (PERF_LOADIMAGE_BEGIN_ID):
    case (PERF_LOADIMAGE_END_ID):
      *RecordSize = sizeof(GUID_QWORD_EVENT_RECORD);
      return EFI_SUCCESS;
    case(PERF_EVENTSIGNAL_BEGIN_ID):
    case(PERF_EVENTSIGNAL_END_ID):
    case (PERF_CALLBACK_BEGIN_ID):
    case (PERF_CALLBACK_END_ID):
      *RecordSize = StringSize + sizeof(DUAL_GUID_STRING_EVENT_RECORD);
      if (*RecordSize > MAX_PERF_RECORD_SIZE) {
        *RecordSize = MAX_PERF_RECORD_SIZE;
      }
      return EFI_SUCCESS;
    case (PERF_EVENT_ID):
    case (PERF_FUNCTION_BEGIN_ID):
    case (PERF_FUNCTION_END_ID):
    case (PERF_INMODULE_BEGIN_ID):
    case (PERF_INMODULE_END_ID):
    case (PERF_CROSSMODULE_BEGIN_ID):
    case (PERF_CROSSMODULE_END_ID):
      *RecordSize = StringSize + sizeof(DYNAMIC_STRING_EVENT_RECORD);
      if (*RecordSize > MAX_PERF_RECORD_SIZE) {
        *RecordSize = MAX_PERF_RECORD_SIZE;
      }
      return EFI_SUCCESS;
    case(PERF_BINDING_SUPPORT_BEGIN_ID):
    case(PERF_BINDING_SUPPORT_END_ID):
    case(PERF_BINDING_START_BEGIN_ID):
    case(PERF_BINDING_START_END_ID):
    case(PERF_BINDING_STOP_BEGIN_ID):
    case(PERF_BINDING_STOP_END_ID):
    default:
      return EFI_INVALID_PARAMETER;
  }
} // GetRecordSize

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
  EFI_HOB_GUID_TYPE                *GuidHob;
  UINTN                            PeiPerformanceSize;
  UINTN                            RecordSize;
  PEI_CORE_PERFORMANCE_HOB_HEADER  *PerfHobHeader;
  FPDT_RECORD_PTR                  PerfRecordPtr;
  UINT64                           Ticker;
  EFI_STATUS                       Status;

  GuidHob = GetFirstGuidHob(&gPeiPerformanceHobGuid);
  if (GuidHob != NULL) {
    //
    // PEI Performance HOB was found, then return the existing one.
    //
    PerfHobHeader = GET_GUID_HOB_DATA(GuidHob);
  } else {
    //
    // PEI Performance HOB was not found, then build one.
    //
    PeiPerformanceSize = PcdGet16(PcdMaxPeiPerformanceLogSize);
    PerfHobHeader = BuildGuidHob(&gPeiPerformanceHobGuid, PeiPerformanceSize);
    ASSERT(PerfHobHeader != NULL);
    ZeroMem(PerfHobHeader, PeiPerformanceSize);
    PerfHobHeader->Signature = PEI_CORE_PERF_HOB_SIG;
    PerfHobHeader->HobLength = sizeof(PEI_CORE_PERFORMANCE_HOB_HEADER);
    PerfHobHeader->LoadImageCount = 0;

    //
    // Also install the PPI here. Since the first perf calls come from the PEI Core,
    // this assures that by the time loaded modules execute perf calls via the PPI,
    // the PPI will have been installed by then
    //
    Status = PeiServicesInstallPpi(&mPerformancePpiDesc);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "PeiCorePerformanceLib - %a - InstallPpi returned %r!\n", __FUNCTION__, Status));
    }
  }

  //
  // Check if there's enough space to fit the incoming record, need to pre-calculate the size here
  //
  Status = GetRecordSize(PerfId, String, &RecordSize);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "PeiCorePerformanceLib - %a - GetRecordSize returned %r!\n", __FUNCTION__, Status));
    return Status;
  }

  if ((PerfHobHeader->HobLength + RecordSize) > PcdGet16(PcdMaxPeiPerformanceLogSize)) {
    DEBUG((DEBUG_ERROR, "PeiCorePerformanceLib - %a - Perf HOB is full\n", __FUNCTION__));
    return EFI_BUFFER_TOO_SMALL;
  }

  PerfRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(((UINT8*) PerfHobHeader) + PerfHobHeader->HobLength);

  //
  // Get ticker value.
  //
  Ticker = GetPerformanceCounter();

  //
  // This is the same for all records
  //
  PerfRecordPtr.RecordHeader->Revision = RECORD_REVISION_1;

  switch(PerfId) {
    case(PERF_ENTRYPOINT_BEGIN_ID):
    case(PERF_ENTRYPOINT_END_ID):
      //
      // Craft a GUID event record
      //
      if (CallerIdentifier == NULL) {
        return EFI_INVALID_PARAMETER;
      }
      PerfRecordPtr.RecordHeader->Type = GUID_EVENT_TYPE;
      //
      // We can copy directly from the location pointed to by CallerIdentifier because EFI_PEI_FILE_HANDLE
      // points to EFI_FFS_FILE_HEADER which has an EFI_GUID as its first element
      //
      CopyMem(&PerfRecordPtr.GuidEvent->Guid, CallerIdentifier, sizeof(EFI_GUID));
      PerfRecordPtr.RecordHeader->Length  = sizeof(GUID_EVENT_RECORD);
      PerfRecordPtr.GuidEvent->ProgressID = PerfId;
      PerfRecordPtr.GuidEvent->ApicID     = GetApicId();
      PerfRecordPtr.GuidEvent->Timestamp  = GetTimeInNanoSecond(Ticker);
      break;

    case(PERF_LOADIMAGE_BEGIN_ID):
      //
      // Craft a GUID Qword event with an unpopulated GUID field
      //
      PerfRecordPtr.RecordHeader->Type          = GUID_QWORD_EVENT_TYPE;
      PerfRecordPtr.RecordHeader->Length        = sizeof(GUID_QWORD_EVENT_RECORD);
      PerfRecordPtr.GuidQwordEvent->ProgressID  = PerfId;
      PerfRecordPtr.GuidQwordEvent->ApicID      = GetApicId();
      PerfRecordPtr.GuidQwordEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      PerfHobHeader->LoadImageCount++;
      PerfRecordPtr.GuidQwordEvent->Qword       = PerfHobHeader->LoadImageCount;
      break;

    case(PERF_LOADIMAGE_END_ID):
      //
      // Craft a Qword GUID event record
      //
      if (CallerIdentifier == NULL) {
        return EFI_INVALID_PARAMETER;
      }
      PerfRecordPtr.RecordHeader->Type = GUID_QWORD_EVENT_TYPE;
      CopyMem(&PerfRecordPtr.GuidQwordEvent->Guid, CallerIdentifier, sizeof(EFI_GUID));
      PerfRecordPtr.RecordHeader->Length        = sizeof(GUID_QWORD_EVENT_RECORD);
      PerfRecordPtr.GuidQwordEvent->ProgressID  = PerfId;
      PerfRecordPtr.GuidQwordEvent->ApicID      = GetApicId();
      PerfRecordPtr.GuidQwordEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      PerfRecordPtr.GuidQwordEvent->Qword       = PerfHobHeader->LoadImageCount;
      break;

    case(PERF_EVENTSIGNAL_BEGIN_ID):
    case(PERF_EVENTSIGNAL_END_ID):
    case(PERF_CALLBACK_BEGIN_ID):
    case(PERF_CALLBACK_END_ID):
      //
      // Craft a Dual GUID String event record
      //
      if (Guid == 0 || String == NULL || CallerIdentifier == NULL) {
        return EFI_INVALID_PARAMETER;
      }
      PerfRecordPtr.RecordHeader->Type = DUAL_GUID_STRING_EVENT_TYPE;
      CopyMem(&PerfRecordPtr.DualGuidStringEvent->Guid1, Guid, sizeof(EFI_GUID));
      CopyMem(&PerfRecordPtr.DualGuidStringEvent->Guid2, CallerIdentifier, sizeof(EFI_GUID));
      PerfRecordPtr.DualGuidStringEvent->ProgressID  = PerfId;
      PerfRecordPtr.DualGuidStringEvent->ApicID      = GetApicId();
      PerfRecordPtr.DualGuidStringEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      PerfRecordPtr.RecordHeader->Length             = sizeof(DUAL_GUID_STRING_EVENT_RECORD);
      CopyStringIntoPerfRecordAndUpdateLength(PerfRecordPtr.DualGuidStringEvent->String, String, &PerfRecordPtr.RecordHeader->Length);
      break;

    case(PERF_EVENT_ID):
    case(PERF_FUNCTION_BEGIN_ID):
    case(PERF_FUNCTION_END_ID):
    case(PERF_INMODULE_BEGIN_ID):
    case(PERF_INMODULE_END_ID):
    case(PERF_CROSSMODULE_BEGIN_ID):
    case(PERF_CROSSMODULE_END_ID):
      //
      // Craft a Dynamic String event record
      //
      if (String == NULL || CallerIdentifier == NULL) {
        return EFI_INVALID_PARAMETER;
      }
      PerfRecordPtr.RecordHeader->Type = DYNAMIC_STRING_EVENT_TYPE;
      CopyMem(&PerfRecordPtr.DynamicStringEvent->Guid, CallerIdentifier, sizeof(EFI_GUID));
      PerfRecordPtr.DynamicStringEvent->ProgressID  = PerfId;
      PerfRecordPtr.DynamicStringEvent->ApicID      = GetApicId();
      PerfRecordPtr.DynamicStringEvent->Timestamp   = GetTimeInNanoSecond(Ticker);
      PerfRecordPtr.RecordHeader->Length            = sizeof(DYNAMIC_STRING_EVENT_RECORD);
      CopyStringIntoPerfRecordAndUpdateLength(PerfRecordPtr.DynamicStringEvent->String, String, &PerfRecordPtr.RecordHeader->Length);
      break;

    case(PERF_BINDING_SUPPORT_BEGIN_ID):
    case(PERF_BINDING_SUPPORT_END_ID):
    case(PERF_BINDING_START_BEGIN_ID):
    case(PERF_BINDING_START_END_ID):
    case(PERF_BINDING_STOP_BEGIN_ID):
    case(PERF_BINDING_STOP_END_ID):
    default:
      DEBUG((DEBUG_ERROR, "PeiCorePerformanceLib - %a - unsupported PerfId 0x%x!\n", __FUNCTION__, PerfId));
      return EFI_UNSUPPORTED;
  }

  PerfHobHeader->HobLength += PerfRecordPtr.RecordHeader->Length;

  return RETURN_SUCCESS;
} // CreatePerformanceMeasurement

/**
  Create performance record with event description and a timestamp.
  This function is here so that PPI calls land in it

  @param This    - Pointer to this PERFORMANCE_PPI
  @param CallerIdentifier  - Image handle or pointer to call ID GUID
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
CreatePerformanceMeasurementPpiCall (
  IN       PERFORMANCE_PPI  *This,
  IN CONST VOID             *CallerIdentifier,  OPTIONAL
  IN CONST VOID             *Guid,    OPTIONAL
  IN CONST CHAR8            *String,  OPTIONAL
  IN       UINT64           Address,  OPTIONAL
  IN       UINT16           PerfId
  )
{
  return CreatePerformanceMeasurement(CallerIdentifier, Guid, String, Address, PerfId);
} // CreatePerformanceMeasurementPpiCall

/**
  Create performance record with event description and a timestamp.
  This fnction is here for calls from the PEI core. All other PEIMs will land in this library via PPI.

  @param CallerIdentifier  - Image handle or pointer to caller ID GUID
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
} // LogPerformanceMeasurement

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
} // GetPerformanceVerbosityLevel

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
} // GetPerformanceCoreFunctionalityMask