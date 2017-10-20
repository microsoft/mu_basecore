/** @file
  Instance of Performance2 Library with empty functions.

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

**/


#include <Base.h>
#include <Library/Performance2Lib.h>
#include <Library/PcdLib.h>

/**
  Creates a record for a performance measurement.

  @param  CallerIdentifier        Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Guid                    Pointer to a GUID identifying  a module, event,
                                  or a trigger.
  @param  String                  Pointer to a Null-terminated ASCII string that
                                  identifies the measurement.
  @param  Address                 Pointer to a 64-bit memory address value that
                                  identifies the measurement.
  @param  PerfId                  16-bit identifier describing the type of performance
                                  measurement.

  @retval RETURN_SUCCESS          The measurement was recorded.
  @retval RETURN_OUT_OF_RESOURCES There are not enough resources to record the measurement.
  @retval RETURN_DEVICE_ERROR     A device error reading the time stamp.

**/
RETURN_STATUS
EFIAPI
LogPerformanceMeasurement (
  IN CONST VOID   *CallerIdentifier,  OPTIONAL
  IN CONST VOID   *Guid,              OPTIONAL
  IN CONST CHAR8  *String,            OPTIONAL
  IN UINT64       Address,            OPTIONAL
  IN UINT16       PerfId
  )
{
  return RETURN_SUCCESS;
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
