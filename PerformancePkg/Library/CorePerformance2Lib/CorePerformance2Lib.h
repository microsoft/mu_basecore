/**

Copyright (c) 2017, Microsoft Corporation
Copyright (c) 2012 - 2016, Intel Corporation

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

#ifndef __CORE_PERFORMANCE_LIB_H__
#define __CORE_PERFORMANCE_LIB_H__

#include <IndustryStandard/Acpi50.h>

//
// Misc defines
//
#define RECORD_REVISION_1      (0x01)

//
// Length field in EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER is a UINT8, thus:
//
#define MAX_PERF_RECORD_SIZE   (0xFF)

//
// FPDT Record Types
//
#define GUID_EVENT_TYPE               0x1010
#define DYNAMIC_STRING_EVENT_TYPE     0x2001
#define DUAL_GUID_STRING_EVENT_TYPE   0x2002
#define GUID_QWORD_EVENT_TYPE         0x2003
#define GUID_QWORD_STRING_EVENT_TYPE  0x2004

//
// Intermediate record types
//
#define HANDLE_EVENT_TYPE               0x9010
#define HANDLE_QWORD_EVENT_TYPE         0xA003
#define HANDLE_QWORD_STRING_EVENT_TYPE  0xA004

//
// Data structures for performance records
//
#pragma pack(1)
//
// FPDT Boot Performance Guid Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  //
  // Progress Identifier
  //
  UINT16                                      ProgressID;
  //
  // APIC ID for the processor in the system used as a timestamp clock source.
  //
  UINT32                                      ApicID;
  //
  // 64-bit value describing elapsed time since the most recent deassertion of processor reset.
  //
  UINT64                                      Timestamp;
  //
  // GUID of the module logging the event
  //
  EFI_GUID                                    Guid;
} GUID_EVENT_RECORD;

//
// FPDT Boot Performance Dynamic String Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  //
  // Progress Identifier
  //
  UINT16                                      ProgressID;
  //
  // APIC ID for the processor in the system used as a timestamp clock source.
  //
  UINT32                                      ApicID;
  //
  // 64-bit value describing elapsed time since the most recent deassertion of processor reset.
  //
  UINT64                                      Timestamp;
  //
  // GUID of the module logging the event.
  //
  EFI_GUID                                    Guid;
  //
  // String describing the record
  //
  CHAR8                                       String[];
} DYNAMIC_STRING_EVENT_RECORD;

//
// FPDT Boot Performance Dual GUID String Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  //
  // Progress Identifier
  //
  UINT16                                      ProgressID;
  //
  // APIC ID for the processor in the system used as a timestamp clock source.
  //
  UINT32                                      ApicID;
  //
  // 64-bit value describing elapsed time since the most recent deassertion of processor reset.
  //
  UINT64                                      Timestamp;
  //
  // First GUID
  //
  EFI_GUID                                    Guid1;
  //
  // Second GUID
  //
  EFI_GUID                                    Guid2;
  //
  // String describing the record
  //
  CHAR8                                       String[];
} DUAL_GUID_STRING_EVENT_RECORD;

//
// FPDT Boot Performance GUID Qword Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  //
  // Progress Identifier
  //
  UINT16                                      ProgressID;
  //
  // APIC ID for the processor in the system used as a timestamp clock source.
  //
  UINT32                                      ApicID;
  //
  // 64-bit value describing elapsed time since the most recent deassertion of processor reset.
  //
  UINT64                                      Timestamp;
  //
  // GUID of the module logging the event
  //
  EFI_GUID                                    Guid;
  //
  // Qword of misc data, meaning depends on the ProgressId
  //
  UINT64                                      Qword;
} GUID_QWORD_EVENT_RECORD;

//
// FPDT Boot Performance GUID Qword String Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  //
  // Progress Identifier
  //
  UINT16                                      ProgressID;
  //
  // APIC ID for the processor in the system used as a timestamp clock source.
  //
  UINT32                                      ApicID;
  //
  // 64-bit value describing elapsed time since the most recent deassertion of processor reset.
  //
  UINT64                                      Timestamp;
  //
  // GUID of the module logging the event
  //
  EFI_GUID                                    Guid;
  //
  // Qword of misc data, meaning depends on the ProgressId
  //
  UINT64                                      Qword;
  //
  // String describing the record
  //
  CHAR8                                       String[];
} GUID_QWORD_STRING_EVENT_RECORD;

//
// Handle Record types (for temp storage)
// The size and layout, apart from GUID replaced w/ Handle and a Reserved field, should be same
// as the GUID version of the event record
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  //
  // Progress Identifier
  //
  UINT16                                      ProgressID;
  //
  // APIC ID for the processor in the system used as a timestamp clock source.
  //
  UINT32                                      ApicID;
  //
  // 64-bit value describing elapsed time since the most recent deassertion of processor reset.
  //
  UINT64                                      Timestamp;
  //
  // Handle address
  //
  UINT64                                      Handle;
  //
  // This reserved UINT64 is padding, so that Handle and Reserved take up the same amount of space as
  // the Guid field in the appropriate GUID record type
  //
  UINT64                                      Reserved;
} HANDLE_EVENT_RECORD;

typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  //
  // Progress Identifier
  //
  UINT16                                      ProgressID;
  //
  // APIC ID for the processor in the system used as a timestamp clock source.
  //
  UINT32                                      ApicID;
  //
  // 64-bit value describing elapsed time since the most recent deassertion of processor reset.
  //
  UINT64                                      Timestamp;
  //
  // Handle address
  //
  UINT64                                      Handle;
  //
  // This reserved UINT64 is padding, so that Handle and Reserved take up the same amount of space as
  // the Guid field in the appropriate GUID record type
  //
  UINT64                                      Reserved;
  //
  // Qword of misc data, meaning depends on the ProgressId
  //
  UINT64                                      Qword;
} HANDLE_QWORD_EVENT_RECORD;

typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  //
  // Progress Identifier
  //
  UINT16                                      ProgressID;
  //
  // APIC ID for the processor in the system used as a timestamp clock source.
  //
  UINT32                                      ApicID;
  //
  // 64-bit value describing elapsed time since the most recent deassertion of processor reset.
  //
  UINT64                                      Timestamp;
  //
  // Handle address
  //
  UINT64                                      Handle;
  //
  // This reserved UINT64 is padding, so that Handle and Reserved take up the same amount of space as
  // the Guid field in the appropriate GUID record type
  //
  UINT64                                      Reserved;
  //
  // Qword of misc data, meaning depends on the ProgressId
  //
  UINT64                                      Qword;
  //
  // String describing the record
  //
  CHAR8                                       String[];
} HANDLE_QWORD_STRING_EVENT_RECORD;
#pragma pack()

//
// Union of all pointers to FPDT records
//
typedef union {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER  *RecordHeader;
  GUID_EVENT_RECORD                            *GuidEvent;
  DYNAMIC_STRING_EVENT_RECORD                  *DynamicStringEvent;
  DUAL_GUID_STRING_EVENT_RECORD                *DualGuidStringEvent;
  GUID_QWORD_EVENT_RECORD                      *GuidQwordEvent;
  GUID_QWORD_STRING_EVENT_RECORD               *GuidQwordStringEvent;
  HANDLE_EVENT_RECORD                          *HandleEvent;
  HANDLE_QWORD_EVENT_RECORD                    *HandleQwordEvent;
  HANDLE_QWORD_STRING_EVENT_RECORD             *HandleQwordStringEvent;
} FPDT_RECORD_PTR;

//
// Helper function declarations
//

/**
  Copies the string from Source into Destination and updates Length with the
  size of the string.

  @param Destination - destination of the string copy
  @param Source      - pointer to the source string which will get copied
  @param Length      - pointer to a length variable to be updated

**/
VOID
CopyStringIntoPerfRecordAndUpdateLength (
  CHAR8  *Destination,
  CONST CHAR8  *Source,
  UINT8  *Length
  );

#endif // __CORE_PERFORMANCE_LIB_H__