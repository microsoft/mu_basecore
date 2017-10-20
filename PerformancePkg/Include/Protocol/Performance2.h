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

Performance 2 protocol, allows logging performance data during DXE.

**/

#ifndef __PERFORMANCE_2_H__
#define __PERFORMANCE_2_H__

//
// GUID for Performance 2 Protocol
//
// {13B70AAE-C04E-4264-9E5E-14870DBAFB78}
#define PERFORMANCE_2_PROTOCOL_GUID \
  { 0x13b70aae, 0xc04e, 0x4264, { 0x9e, 0x5e, 0x14, 0x87, 0xd, 0xba, 0xfb, 0x78 } }


typedef struct _PERFORMANCE_2_PROTOCOL PERFORMANCE_2_PROTOCOL;

/**
  Create performance record with event description and a timestamp.

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
(EFIAPI *DXE_CREATE_PERFORMANCE_MEASUREMENT)(
  IN CONST VOID                    *Handle,  OPTIONAL
  IN CONST VOID                    *Guid,    OPTIONAL
  IN CONST CHAR8                   *String,  OPTIONAL
  IN       UINT64                   Address,  OPTIONAL
  IN       UINT16                   PerfId
  );


struct _PERFORMANCE_2_PROTOCOL {
  DXE_CREATE_PERFORMANCE_MEASUREMENT CreatePerformanceMeasurement;
};

extern EFI_GUID gPerformance2ProtocolGuid;

#endif // __PERFORMANCE_2_H__
