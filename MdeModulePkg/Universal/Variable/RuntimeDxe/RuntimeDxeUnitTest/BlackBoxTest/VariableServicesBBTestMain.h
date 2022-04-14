/** @file

  Copyright 2006 - 2016 Unified EFI, Inc.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at 
  http://opensource.org/licenses/bsd-license.php
 
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
**/
/*++

Module Name:
  VariableServicesBbTestMain.h

Abstract:
  Header file for Variable Services Black-Box Test.

--*/

#ifndef _VARIABLE_SERVICES_BB_TEST_MAIN_H
#define _VARIABLE_SERVICES_BB_TEST_MAIN_H

//
// Includes
//
#include <SctShim.h>
#include "Guid.h"

//
// Definitions
//

#define VARIABLE_SERVICES_BB_TEST_REVISION    0x00010000

#define VARIABLE_SERVICES_BB_TEST_GUID        \
  { 0xD6844631, 0x0A1E, 0x42d1, {0xA5, 0x94, 0x60, 0x35, 0x48, 0x47, 0xB6, 0x76 }}

#define MAX_BUFFER_SIZE                       256

#define TEST_VENDOR1_GUID                         \
  { 0xF6FAB04F, 0xACAF, 0x4af3, { 0xB9, 0xFA, 0xDC, 0xF9, 0x7F, 0xB4, 0x42, 0x6F } }
#define TEST_VENDOR2_GUID                         \
  { 0x49b08eba, 0xa56c, 0x4015, { 0xb7, 0x03, 0xe7, 0x73, 0xc3, 0x32, 0x62, 0x8b } }

#define GET_VARIABLE_CONF_TEST_GUID               \
  { 0xd90941aa, 0xb626, 0x4665, { 0xba, 0x14, 0x64, 0x08, 0x43, 0x96, 0xf3, 0x1d } }
#define GET_NEXT_VARIABLE_NAME_CONF_TEST_GUID     \
  { 0xe8014c92, 0x15c4, 0x42a8, { 0x8b, 0x0d, 0x60, 0x80, 0xc4, 0x7d, 0x37, 0x78 } }
#define SET_VARIABLE_CONF_TEST_GUID               \
  { 0xc0391b41, 0x591f, 0x4173, { 0x8c, 0xe3, 0x5f, 0xf9, 0x15, 0x8a, 0x94, 0x8c } }

#define GET_VARIABLE_FUNC_TEST_GUID               \
  { 0x5e7928aa, 0xcf97, 0x469c, { 0xa6, 0xca, 0xa5, 0x57, 0x71, 0x35, 0xc1, 0x0c } }
#define GET_NEXT_VARIABLE_NAME_FUNC_TEST_GUID     \
  { 0x66a7216f, 0xa855, 0x47d3, { 0x91, 0x9c, 0xb6, 0xd3, 0x46, 0x68, 0xbe, 0x8e } }
#define SET_VARIABLE_FUNC_TEST_GUID               \
  { 0xd4700fe8, 0x9832, 0x4353, { 0x96, 0x1f, 0x74, 0x68, 0x2d, 0x37, 0x01, 0xf6 } }

#define MULTIPLE_STRESS_TEST_GUID                 \
  { 0xe244a6b3, 0x3e18, 0x4fdc, { 0x92, 0x5a, 0x0b, 0xfc, 0xdd, 0x29, 0x28, 0xee } }
#define OVERFLOW_STRESS_TEST_GUID                 \
  { 0xa9f04a54, 0x1f65, 0x44d2, { 0x8b, 0x52, 0xe6, 0xd4, 0xa0, 0x7f, 0x82, 0x1e } }

//
// For QueryVariableInfo
//
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#define QUERY_VAR_CASE6_GUID1                         \
  { 0x5d1f058c, 0x08ae, 0x4f71, {0x9e, 0x81, 0xa6, 0x78, 0x30, 0x33, 0xa5, 0xed }}

#define QUERY_VAR_CASE6_GUID2                         \
  { 0xfea6828d, 0xa8c9, 0x4167, {0xb4, 0x16, 0x69, 0x73, 0x8b, 0xbc, 0xce, 0x81 }}

#define QUERY_VAR_CASE2_GUID3                         \
  { 0x2300fd85, 0xa65c, 0x462a, {0x86, 0x35, 0x0d, 0xc0, 0x3f, 0xac, 0x7b, 0x69 }}

#define QUERYVAR_GUID1 \
  { 0xcc648ba7, 0x1be7, 0x4262, {0xa8, 0xcb, 0x47, 0x08, 0x51, 0x6d, 0x86, 0xe8 }}

#define QUERYVAR_GUID2 \
  { 0xcc648ba7, 0x1be7, 0x4262, {0xa8, 0xcb, 0x47, 0x08, 0x51, 0x6d, 0x86, 0xe8 }}

#if ((ALIGNMENT == 0) || (ALIGNMENT == 1))
#define GET_PAD_SIZE(a) (0)
#else
#define GET_PAD_SIZE(a) (((~a) + 1) & (ALIGNMENT - 1))
#endif

#define MAX_DATA_SIZE 20
#endif


#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#define HARDWARE_ERROR_RECORD_FUNC_TEST_GUID \
  { 0xefaba332, 0x13e8, 0x4730, {0x97, 0xcb, 0x48, 0xce, 0x9f, 0x8, 0x26, 0xc5 }}

#define HARDWARE_ERROR_RECORD_CONF_TEST_GUID \
  { 0xcf94f695, 0x7917, 0x4f78, {0xa0, 0xa8, 0xf0, 0xb5, 0x9, 0x9, 0x92, 0x97 }}

#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x00000008

#define EFI_HARDWARE_ERROR_VARIABLE \
  { 0x414E6BDD, 0xE47B, 0x47cc, {0xB2, 0x44, 0xBB, 0x61, 0x02, 0x0C, 0xF5, 0x16 }}

// {9338D0EC-807B-4750-986A-8F2A91BB3616}
#define EFI_AUTHVARIABLE_DER_FUNC_TEST_GUID \
  { 0x9338d0ec, 0x807b, 0x4750, {0x98, 0x6a, 0x8f, 0x2a, 0x91, 0xbb, 0x36, 0x16 }}

// {28155531-80C5-4ad0-8471-A5E2AECF236C}
#define EFI_AUTHVARIABLE_DER_CONF_TEST_GUID \
  { 0x28155531, 0x80c5, 0x4ad0, {0x84, 0x71, 0xa5, 0xe2, 0xae, 0xcf, 0x23, 0x6c }}

#endif

//
// The Variable Name of Hardware Error Record Variables 
// defined in the UEFI Spec is HwErrRec####. For example,
// HwErrRec0001, HwErrRec0002, HwErrRecF31A, etc.
// The prefix length is 8, index length is 4.
// Consider the tail of string, the name length is 13.
//
#define HW_ERR_REC_VARIABLE_NAME_LEN           13
#define HW_ERR_REC_VARIABLE_NAME_PREFIX_LEN    8
#define HW_ERR_REC_VARIABLE_NAME_INDEX_LEN     4

//
// Global Variables
//

extern EFI_GUID gTestVendor1Guid;
extern EFI_GUID gTestVendor2Guid;

//
// Prototypes
//

//
// TDS 3.1
//
EFI_STATUS
GetVariableConfTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

//
// TDS 3.2
//
EFI_STATUS
GetNextVariableNameConfTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

//
// TDS 3.3
//
EFI_STATUS
SetVariableConfTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
//
// TDS 3.4
//
EFI_STATUS
QueryVariableInfoConfTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

EFI_STATUS
AuthVariableDERConfTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

EFI_STATUS
AuthVariableDERFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

#endif

//
// TDS 4.1
//
EFI_STATUS
GetVariableFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

//
// TDS 4.2
//
EFI_STATUS
GetNextVariableNameFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

//
// TDS 4.3
//
EFI_STATUS
SetVariableFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
//
// TDS 4.4
//
EFI_STATUS
QueryVariableInfoFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

EFI_STATUS
HardwareErrorRecordFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );
  
EFI_STATUS
HardwareErrorRecordConfTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );
  
#endif

//
// TDS 5.1
//
EFI_STATUS
MultipleStressTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

//
// TDS 5.2
//
EFI_STATUS
OverflowStressTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

//
// Support functions
//
EFI_STATUS
GetTestSupportLibrary (
  IN EFI_HANDLE                           SupportHandle,
  OUT EFI_STANDARD_TEST_LIBRARY_PROTOCOL  **StandardLib,
  OUT EFI_TEST_RECOVERY_LIBRARY_PROTOCOL  **RecoveryLib,
  OUT EFI_TEST_LOGGING_LIBRARY_PROTOCOL   **LoggingLib
  );
#endif

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)  
EFI_STATUS
Myitox (
  IN UINTN        Num,
  OUT CHAR16      *StringNum
  );

#endif
