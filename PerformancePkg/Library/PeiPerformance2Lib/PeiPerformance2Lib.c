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
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/Performance2Lib.h>
#include <Ppi/PerformancePpi.h>

/**
  Create performance record with event description and a timestamp.
  This function locates the Performance PPI and calls it with the same parameters.

  @param CallerIdentifier  - Image handle or caller ID GUID
  @param Guid    - Pointer to a GUID
  @param String  - Pointer to a string describing the measurement
  @param Address - Pointer to a location in memory relevant to the measurement
  @param PerfId  - Performance identifier describing the type of measurement

  @retval RETURN_SUCCESS           - Successfully created performance record
  @retval RETURN_OUT_OF_RESOURCES  - Ran out of space to store the records
  @retval RETURN_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                     pointer or invalid PerfId
  @retval RETURN_NOT_READY         - PPI can't be located

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
  EFI_STATUS        Status;
  PERFORMANCE_PPI   *PerformancePpi;

  Status = PeiServicesLocatePpi (
                &gPerformancePpiGuid,
                0,
                NULL,
                (VOID **) &PerformancePpi
                );

  if (!EFI_ERROR(Status)) {
    return (RETURN_STATUS) PerformancePpi->CreatePerformanceMeasurement(PerformancePpi, CallerIdentifier, Guid, String, Address, PerfId);
  } else {
    return RETURN_NOT_READY;
  }
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
