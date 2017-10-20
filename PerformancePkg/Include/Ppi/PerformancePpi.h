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

Performance PPI, allows logging performance data during PEI.

**/

#ifndef _PERFORMANCE_PPI_H_
#define _PERFORMANCE_PPI_H_

#define PERFORMANCE_PPI_GUID \
  { \
    0x82e1fb66, 0xce5d, 0x49fc, { 0xba, 0x8f, 0xac, 0x98, 0x82, 0xbf, 0x2f, 0x62 } \
  }

typedef struct _PERFORMANCE_PPI PERFORMANCE_PPI;

/**
  Create performance record with event description and a timestamp.

  @param This    - Pointer to this PERFORMANCE_PPI
  @param Handle  - Image handle
  @param Guid    - Pointer to a GUID
  @param String  - Pointer to a string describing the measurement
  @param Address - Pointer to a location in memory relevant to the measurement
  @param PerfId  - Performance identifier describing the type of measurement

  @retval EFI_SUCCESS           - Successfully created performance record
  @retval EFI_OUT_OF_RESOURCES  - Ran out of space to store the records
  @retval EFI_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                  pointer or invalid PerfId

**/
typedef
EFI_STATUS
(EFIAPI *PEI_CREATE_PERFORMANCE_MEASUREMENT)(
  IN       PERFORMANCE_PPI  *This,
  IN CONST VOID             *Handle,  OPTIONAL
  IN CONST VOID             *Guid,    OPTIONAL
  IN CONST CHAR8            *String,  OPTIONAL
  IN       UINT64           Address,  OPTIONAL
  IN       UINT16           PerfId
  );

//
// Performance PPI
//
struct _PERFORMANCE_PPI {
  PEI_CREATE_PERFORMANCE_MEASUREMENT    CreatePerformanceMeasurement;
};

extern EFI_GUID gPerformancePpiGuid;

#endif // _PERFORMANCE_PPI_H_

