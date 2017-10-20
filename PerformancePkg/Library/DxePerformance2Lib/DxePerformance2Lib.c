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


#include <PiDxe.h>
#include <Library/Performance2Lib.h>
#include <Protocol/Performance2.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>

//
// The cached Performance 2 Protocol interface
//
PERFORMANCE_2_PROTOCOL     *mPerformance2 = NULL;

/**
  The function caches the pointers to Performance2 protocol

  @retval EFI_SUCCESS     Performance2 protocol is successfully located
  @retval EFI_NOT_FOUND   Protocol is not located to log performance

**/
EFI_STATUS
GetPerformance2Protocol (
  VOID
  )
{
  EFI_STATUS                Status;
  PERFORMANCE_2_PROTOCOL   *Performance2;

  if (mPerformance2 != NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (&gPerformance2ProtocolGuid, NULL, (VOID **) &Performance2);
  if (!EFI_ERROR (Status) && Performance2 != NULL) {
    //
    // Cache Performance2 Protocol.
    //
    mPerformance2 = Performance2;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Create performance record with event description and a timestamp.
  This function calls the Performance2 protocol to record the measurement.

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
  EFI_STATUS  Status;

  Status = GetPerformance2Protocol ();
  if (EFI_ERROR (Status) || mPerformance2 == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  Status = mPerformance2->CreatePerformanceMeasurement(CallerIdentifier, Guid, String, Address, PerfId);

  return (RETURN_STATUS) Status;
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
