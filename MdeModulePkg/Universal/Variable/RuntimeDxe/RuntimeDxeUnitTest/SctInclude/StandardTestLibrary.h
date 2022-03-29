/** @file

  Copyright 2006 - 2016 Unified EFI, Inc.<BR>
  Copyright (c) 2010 - 2016, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
/*++

Module Name:

  StandardTestLibrary.h

Abstract:

  This file defines the EFI Standard Test Library Protocol.

--*/

#ifndef _EFI_STANDARD_TEST_LIBRARY_H_
#define _EFI_STANDARD_TEST_LIBRARY_H_

//
// Includes
//

//
// EFI Standard Test Library Protocol Definitions
//

#define EFI_STANDARD_TEST_LIBRARY_GUID      \
  { 0x1f9c2ae7, 0xf147, 0x4d19, {0xa5, 0xe8, 0x25, 0x5a, 0xd0, 0x05, 0xeb, 0x3e }}

#define EFI_STANDARD_TEST_LIBRARY_REVISION  0x00010000

//
// Forward reference for pure ANSI compatibility
//

typedef struct _EFI_STANDARD_TEST_LIBRARY_PROTOCOL EFI_STANDARD_TEST_LIBRARY_PROTOCOL;

//
// Related Definitions
//

//
// EFI Test Level
//
typedef UINT32 EFI_TEST_LEVEL;

#define EFI_TEST_LEVEL_MINIMAL     0x01
#define EFI_TEST_LEVEL_DEFAULT     0x02
#define EFI_TEST_LEVEL_EXHAUSTIVE  0x04

//
// EFI Test Case Attribute
//
typedef UINT32 EFI_TEST_ATTRIBUTE;

#define EFI_TEST_CASE_AUTO            0x00
#define EFI_TEST_CASE_MANUAL          0x01
#define EFI_TEST_CASE_DESTRUCTIVE     0x02
#define EFI_TEST_CASE_RESET_REQUIRED  0x04

//
// EFI Test Assertion Types
//
typedef enum {
  EFI_TEST_ASSERTION_PASSED,
  EFI_TEST_ASSERTION_WARNING,
  EFI_TEST_ASSERTION_FAILED
} EFI_TEST_ASSERTION;

//
// EFI Test Verbose Levels
//
typedef enum {
  EFI_VERBOSE_LEVEL_QUIET,
  EFI_VERBOSE_LEVEL_MINIMAL,
  EFI_VERBOSE_LEVEL_DEFAULT,
  EFI_VERBOSE_LEVEL_NOISY,
  EFI_VERBOSE_LEVEL_EXHAUSTIVE
} EFI_VERBOSE_LEVEL;

//
// EFI Standard Test Library Protocol API - RecordAssertion
//
typedef
EFI_STATUS
(EFIAPI *EFI_RECORD_ASSERTION)(
  IN  EFI_STANDARD_TEST_LIBRARY_PROTOCOL          *This,
  IN  EFI_TEST_ASSERTION                          Type,
  IN  EFI_GUID                                    EventId,
  IN  CHAR16                                      *Description,
  IN  CHAR16                                      *Detail,
  ...
  )

/*++

Routine Description:

  Records the test result.

Arguments:

  This          - Standard test library protocol instance.

  Type          - Test result.

  EventId       - GUID for the checkpoint.

  Description   - Simple description for the checkpoint.

  Detail        - Format string for the detail test information.

Returns:

  EFI_SUCCESS if everything is correct.

--*/
;

//
// EFI Standard Test Library Protocol API - RecordMessage
//
typedef
EFI_STATUS
(EFIAPI *EFI_RECORD_MESSAGE)(
  IN  EFI_STANDARD_TEST_LIBRARY_PROTOCOL          *This,
  IN  EFI_VERBOSE_LEVEL                           VerboseLevel,
  IN  CHAR16                                      *Message,
  ...
  )

/*++

Routine Description:

  Records the test message.

Arguments:

  This          - Standard test library protocol instance.

  VerboseLevel  - Minimal verbose level to record this message. For example,
                  EFI_VERBOSE_LEVEL_QUIET means this message should be recorded
                  even the test is run in QUIET mode. On the contrary,
                  EFI_VERBOSE_LEVEL_EXHAUSTIVE means this message will only be
                  recorded when the test is run in EXHAUSTIVE mode.

  Message       - Format string for the detail test information.

Returns:

  EFI_SUCCESS if everything is correct.

--*/
;

//
// EFI Standard Test Library Protocol API - GetResultCount
//
typedef
EFI_STATUS
(EFIAPI *EFI_GET_RESULT_COUNT)(
  IN  EFI_STANDARD_TEST_LIBRARY_PROTOCOL          *This,
  IN  EFI_TEST_ASSERTION                          Type,
  OUT UINT32                                      *Count
  )

/*++

Routine Description:

  Gets the number of test results.

Arguments:

  This          - Standard test library protocol instance.

  Type          - Specifies the kind of test results to be returned.

  Count         - Number of specified test results.

Returns:

  EFI_SUCCESS if everything is correct.

--*/
;

//
// EFI Standard Test Library Protocol
//

struct _EFI_STANDARD_TEST_LIBRARY_PROTOCOL {
  UINT64                  LibraryRevision;
  CHAR16                  *Name;
  CHAR16                  *Description;
  EFI_RECORD_ASSERTION    RecordAssertion;
  EFI_RECORD_MESSAGE      RecordMessage;
  EFI_GET_RESULT_COUNT    GetResultCount;
};

//
// Global ID for EFI Standard Test Library Protocol
//

extern EFI_GUID  gEfiStandardTestLibraryGuid;

//
// Global ID for some standard services
//
#ifndef EFI_NULL_GUID
#define EFI_NULL_GUID                     \
  { 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}
#endif

#define EFI_GENERIC_CLIENT_GUID           \
  { 0x71652D04, 0xBF38, 0x434a, {0xBC, 0xB8, 0x65, 0x47, 0xD7, 0xFD, 0x83, 0x84 }}

#define EFI_BOOT_SERVICES_CLIENT_GUID     \
  { 0xE9EF7553, 0xF833, 0x4e56, {0x96, 0xE8, 0x38, 0xAE, 0x67, 0x95, 0x23, 0xCC }}

#define EFI_RUNTIME_SERVICES_CLIENT_GUID  \
  { 0xAFF115FB, 0x387B, 0x4c18, {0x8C, 0x41, 0x6A, 0xFC, 0x7F, 0x03, 0xBB, 0x90 }}

extern EFI_GUID  gEfiNullGuid;
extern EFI_GUID  gEfiGenericCategoryGuid;
extern EFI_GUID  gEfiBootServicesCategoryGuid;
extern EFI_GUID  gEfiRuntimeServicesCategoryGuid;

#endif
