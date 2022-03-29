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
  VariableServicesBbTestConformance.c

Abstract:
  Source file for Variable Services Black-Box Test - Conformance Test.

--*/

#include "VariableServicesBBTestMain.h"

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
extern EFI_GUID  gGlobalVariableGuid;
extern EFI_GUID  gHwErrRecGuid;
#endif

//
// Prototypes (external)
//

EFI_STATUS
GetVariableConfTest (
  IN EFI_BB_TEST_PROTOCOL  *This,
  IN VOID                  *ClientInterface,
  IN EFI_TEST_LEVEL        TestLevel,
  IN EFI_HANDLE            SupportHandle
  );

EFI_STATUS
GetNextVariableNameConfTest (
  IN EFI_BB_TEST_PROTOCOL  *This,
  IN VOID                  *ClientInterface,
  IN EFI_TEST_LEVEL        TestLevel,
  IN EFI_HANDLE            SupportHandle
  );

EFI_STATUS
SetVariableConfTest (
  IN EFI_BB_TEST_PROTOCOL  *This,
  IN VOID                  *ClientInterface,
  IN EFI_TEST_LEVEL        TestLevel,
  IN EFI_HANDLE            SupportHandle
  );

//
// Prototypes (internal)
//

EFI_STATUS
GetVariableConfTestSub1 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetVariableConfTestSub2 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetVariableConfTestSub3 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetVariableConfTestSub4 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetVariableConfTestSub5 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetVariableConfTestSub6 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetVariableConfTestSub7 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetNextVariableNameConfTestSub1 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetNextVariableNameConfTestSub2 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetNextVariableNameConfTestSub3 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetNextVariableNameConfTestSub4 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetNextVariableNameConfTestSub5 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetNextVariableNameConfTestSub6 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
GetNextVariableNameConfTestSub7 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
SetVariableConfTestSub1 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
SetVariableConfTestSub2 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
SetVariableConfTestSub3 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

EFI_STATUS
SetVariableConfTestSub4 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  );

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
EFI_STATUS
QueryVariableInfoConfTest (
  IN EFI_BB_TEST_PROTOCOL  *This,
  IN VOID                  *ClientInterface,
  IN EFI_TEST_LEVEL        TestLevel,
  IN EFI_HANDLE            SupportHandle
  );

//
// QueryVariableInfo with MaximumVariableStorageSize being NULL
//
EFI_STATUS
QueryVariableInfoConfTestSub1 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  );

//
// QueryVariableInfo with RemainingVariableStorageSize being NULL
//
EFI_STATUS
QueryVariableInfoConfTestSub2 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  );

//
// QueryVariableInfo with MaximumVariableSize being NULL
//
EFI_STATUS
QueryVariableInfoConfTestSub3 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  );

//
// QueryVariableInfo with Attributes being 0
//
EFI_STATUS
QueryVariableInfoConfTestSub4 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  );

//
// QueryVariableInfo with Attributes being an invalid combination
//
EFI_STATUS
QueryVariableInfoConfTestSub5 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  );

#endif

//
// Functions
//

/**
 *  TDS 3.1 - Entry point for RT->GetVariable() Conformance Test.
 *  @param This             A pointer to the EFI_BB_TEST_PROTOCOL instance.
 *  @param ClientInterface  A pointer to the interface to be tested.
 *  @param TestLevel        Test "thoroughness" control.
 *  @param SupportHandle    A handle containing support protocols.
 *  @return EFI_SUCCESS     Successfully.
 *  @return Other value     Something failed.
 */
EFI_STATUS
GetVariableConfTest (
  IN EFI_BB_TEST_PROTOCOL  *This,
  IN VOID                  *ClientInterface,
  IN EFI_TEST_LEVEL        TestLevel,
  IN EFI_HANDLE            SupportHandle
  )
{
  EFI_STATUS                          Status;
  EFI_RUNTIME_SERVICES                *RT;
  EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib;
  EFI_TEST_RECOVERY_LIBRARY_PROTOCOL  *RecoveryLib;
  EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib;

  //
  // Get test support library interfaces
  //
  Status = GetTestSupportLibrary (
             SupportHandle,
             &StandardLib,
             &RecoveryLib,
             &LoggingLib
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RT = (EFI_RUNTIME_SERVICES *)ClientInterface;

  //
  // GetVariable when VariableName is NULL
  //
  Status = GetVariableConfTestSub1 (RT, StandardLib, LoggingLib);

  //
  // GetVariable when VendorGuid is NULL
  //
  Status = GetVariableConfTestSub2 (RT, StandardLib, LoggingLib);

  //
  // GetVariable when DataSize is NULL
  //
  Status = GetVariableConfTestSub3 (RT, StandardLib, LoggingLib);

  //
  // GetVariable when Data is NULL
  //
  Status = GetVariableConfTestSub4 (RT, StandardLib, LoggingLib);

  //
  // GetVariable with nonexistent VariableName
  //
  Status = GetVariableConfTestSub5 (RT, StandardLib, LoggingLib);

  //
  // GetVariable with nonexistent VendorGuid
  //
  Status = GetVariableConfTestSub6 (RT, StandardLib, LoggingLib);

  //
  // GetVariable without enough DataSize
  //
  Status = GetVariableConfTestSub7 (RT, StandardLib, LoggingLib);

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  TDS 3.2 - Entry point for RT->GetNextVariableName() Conformance Test.
 *  @param This             A pointer to the EFI_BB_TEST_PROTOCOL instance.
 *  @param ClientInterface  A pointer to the interface to be tested.
 *  @param TestLevel        Test "thoroughness" control.
 *  @param SupportHandle    A handle containing support protocols.
 *  @return EFI_SUCCESS     Execute successfully.
 *  @return Other value     Something failed.
 */
EFI_STATUS
GetNextVariableNameConfTest (
  IN EFI_BB_TEST_PROTOCOL  *This,
  IN VOID                  *ClientInterface,
  IN EFI_TEST_LEVEL        TestLevel,
  IN EFI_HANDLE            SupportHandle
  )
{
  EFI_STATUS                          Status;
  EFI_RUNTIME_SERVICES                *RT;
  EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib;
  EFI_TEST_RECOVERY_LIBRARY_PROTOCOL  *RecoveryLib;
  EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib;

  //
  // Get test support library interfaces
  //
  Status = GetTestSupportLibrary (
             SupportHandle,
             &StandardLib,
             &RecoveryLib,
             &LoggingLib
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RT = (EFI_RUNTIME_SERVICES *)ClientInterface;

  //
  // GetNextVariableName when VariableNameSize is NULL
  //
  Status = GetNextVariableNameConfTestSub1 (RT, StandardLib, LoggingLib);

  //
  // GetNextVariableName when VariableName is NULL
  //
  Status = GetNextVariableNameConfTestSub2 (RT, StandardLib, LoggingLib);

  //
  // GetNextVariableName when VendorGuid is NULL
  //
  Status = GetNextVariableNameConfTestSub3 (RT, StandardLib, LoggingLib);

  //
  // GetNextVariableName without enough VariableNameSize
  //
  Status = GetNextVariableNameConfTestSub4 (RT, StandardLib, LoggingLib);

  //
  // GetNextVariableName after the entire variable list has been returned
  //
  Status = GetNextVariableNameConfTestSub5 (RT, StandardLib, LoggingLib);

  //
  // GetNextVariableName when a VariableName buffer on input is not a Null-terminated string
  //
  Status = GetNextVariableNameConfTestSub6 (RT, StandardLib, LoggingLib);

  //
  // GetNextVariableName when input values of VariableName and VendorGuid are not a name and GUID of an existing variable
  //
  Status = GetNextVariableNameConfTestSub7 (RT, StandardLib, LoggingLib);
  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  TDS 3.3 - Entry point for RT->SetVariable() Conformance Test.
 *  @param This             A pointer to the EFI_BB_TEST_PROTOCOL instance.
 *  @param ClientInterface  A pointer to the interface to be tested.
 *  @param TestLevel        Test "thoroughness" control.
 *  @param SupportHandle    A handle containing support protocols.
 *  @return EFI_SUCCESS     Execute successfully.
 *  @return Other value     Something failed.
 */
EFI_STATUS
SetVariableConfTest (
  IN EFI_BB_TEST_PROTOCOL  *This,
  IN VOID                  *ClientInterface,
  IN EFI_TEST_LEVEL        TestLevel,
  IN EFI_HANDLE            SupportHandle
  )
{
  EFI_STATUS                          Status;
  EFI_RUNTIME_SERVICES                *RT;
  EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib;
  EFI_TEST_RECOVERY_LIBRARY_PROTOCOL  *RecoveryLib;
  EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib;

  //
  // Get test support library interfaces
  //
  Status = GetTestSupportLibrary (
             SupportHandle,
             &StandardLib,
             &RecoveryLib,
             &LoggingLib
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RT = (EFI_RUNTIME_SERVICES *)ClientInterface;

  //
  // SetVariable when VariableName is an empty string
  //
  Status = SetVariableConfTestSub1 (RT, StandardLib, LoggingLib);

  //
  // SetVariable with invalid combination of attribute bits
  //
  Status = SetVariableConfTestSub2 (RT, StandardLib, LoggingLib);

  //
  // SCR #2863
  // SetVariable with a variable whose size is > 1024 bytes
  //
  // Status = SetVariableConfTestSub3 (RT, StandardLib, LoggingLib);

  //
  // SetVariable(delete) a variable while it isn't existed
  //
  Status = SetVariableConfTestSub4 (RT, StandardLib, LoggingLib);

  //
  // Done
  //
  return EFI_SUCCESS;
}

// ********************************************************
// Internal Functions
// ********************************************************

/**
 *  GetVariable when VariableName is NULL.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetVariableConfTestSub1 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataSize;
  UINT8               Data[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub1",
                  L"TDS 3.1.2.1"
                  );
  }

  //
  // GetVariable should not succeed when VariableName is NULL
  //
  DataSize = MAX_BUFFER_SIZE;
  Status   = RT->GetVariable (
                   NULL,                  // VariableName
                   &gTestVendor1Guid,     // VendorGuid
                   NULL,                  // Attributes
                   &DataSize,             // DataSize
                   Data                   // Data
                   );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid001,
                 L"RT.GetVariable - When VariableName is NULL",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub1",
                  L"TDS 3.1.2.1"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetVariable when VendorGuid is NULL.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetVariableConfTestSub2 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataSize;
  UINT8               Data[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub2",
                  L"TDS 3.1.2.2"
                  );
  }

  //
  // GetVariable should not succeed when VendorGuid is NULL
  //
  DataSize = MAX_BUFFER_SIZE;
  Status   = RT->GetVariable (
                   L"TestVariable",       // VariableName
                   NULL,                  // VendorGuid
                   NULL,                  // Attributes
                   &DataSize,             // DataSize
                   Data                   // Data
                   );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid002,
                 L"RT.GetVariable - When VendorGuid is NULL",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub2",
                  L"TDS 3.1.2.2"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetVariable when DataSize is NULL.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetVariableConfTestSub3 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINT8               Data[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub3",
                  L"TDS 3.1.2.3"
                  );
  }

  //
  // Set a test variable
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",             // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                          // DataSize
                 Data                         // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetVariable - Cannot insert a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetVariableConfTestSub3",
                    L"TDS 3.1.2.3 - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // GetVariable should not succeed when DataSize is NULL
  //
  Status = RT->GetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 NULL,                    // Attributes
                 NULL,                    // DataSize
                 Data                     // Data
                 );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid003,
                 L"RT.GetVariable - With DataSize is NULL",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Delete the variable (restore environment)
  //
  Status = RT->SetVariable (
                 L"TestVariable",             // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                           // DataSize
                 Data                         // Data
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub3",
                  L"TDS 3.1.2.3"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetVariable when Data is NULL.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetVariableConfTestSub4 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINTN               DataSize;
  UINT8               Data[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub4",
                  L"TDS 3.1.2.4"
                  );
  }

  //
  // Set a test variable
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",             // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                          // DataSize
                 Data                         // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetVariable - Cannot insert a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetVariableConfTestSub3",
                    L"TDS 3.1.2.3 - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // GetVariable should not succeed when Data is NULL
  //
  DataSize = MAX_BUFFER_SIZE;
  Status   = RT->GetVariable (
                   L"TestVariable",       // VariableName
                   &gTestVendor1Guid,     // VendorGuid
                   NULL,                  // Attributes
                   &DataSize,             // DataSize
                   NULL                   // Data
                   );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid004,
                 L"RT.GetVariable - With Data is NULL",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Delete the variable (restore environment)
  //
  Status = RT->SetVariable (
                 L"TestVariable",             // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                           // DataSize
                 Data                         // Data
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub4",
                  L"TDS 3.1.2.4"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetVariable with nonexistent VariableName.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetVariableConfTestSub5 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINTN               DataSize;
  UINT8               Data[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub5",
                  L"TDS 3.1.2.5"
                  );
  }

  //
  // Insert a test variable
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",             // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                          // DataSize
                 Data                         // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetVariable - Cannot insert a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetVariableConfTestSub5",
                    L"TDS 3.1.2.5 - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // Delete the test variable
  //
  Status = RT->SetVariable (
                 L"TestVariable",             // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                           // DataSize
                 Data                         // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetVariable - Cannot delete a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetVariableConfTestSub5",
                    L"TDS 3.1.2.5 - Cannot delete a variable"
                    );
    }

    return Status;
  }

  //
  // GetVariable should not succeed with a nonexistent VariableName
  //
  DataSize = MAX_BUFFER_SIZE;
  Status   = RT->GetVariable (
                   L"TestVariable",       // VariableName
                   &gTestVendor1Guid,     // VendorGuid
                   NULL,                  // Attributes
                   &DataSize,             // DataSize
                   Data                   // Data
                   );
  if (Status == EFI_NOT_FOUND) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid005,
                 L"RT.GetVariable - With nonexistent VariableName",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_NOT_FOUND
                 );

  //
  // Insert two similar variables
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariableA",            // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                          // DataSize
                 Data                         // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetVariable - Cannot insert similar variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetVariableConfTestSub5",
                    L"TDS 3.1.2.5 - Cannot insert similar variable"
                    );
    }

    return Status;
  }

  Status = RT->SetVariable (
                 L"TestVariabl",              // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                          // DataSize
                 Data                         // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetVariable - Cannot insert similar variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetVariableConfTestSub5",
                    L"TDS 3.1.2.5 - Cannot insert similar variable"
                    );
    }

    return Status;
  }

  //
  // GetVariable should not succeed with a nonexistent VariableName
  //
  DataSize = MAX_BUFFER_SIZE;
  Status   = RT->GetVariable (
                   L"TestVariable",       // VariableName
                   &gTestVendor1Guid,     // VendorGuid
                   NULL,                  // Attributes
                   &DataSize,             // DataSize
                   Data                   // Data
                   );
  if (Status == EFI_NOT_FOUND) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid005,
                 L"RT.GetVariable - With nonexistent VariableName",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_NOT_FOUND
                 );

  //
  // Delete two similar variables (restore environment)
  //
  Status = RT->SetVariable (
                 L"TestVariableA",            // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                           // DataSize
                 Data                         // Data
                 );

  Status = RT->SetVariable (
                 L"TestVariabl",              // VariableName
                 &gTestVendor1Guid,           // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                           // DataSize
                 Data                         // Data
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub5",
                  L"TDS 3.1.2.5"
                  );
  }

  //
  // Done
  //
  return Status;
}

/**
 *  GetVariable with nonexistent VendorGuid.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetVariableConfTestSub6 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINTN               DataSize;
  UINT8               Data[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub6",
                  L"TDS 3.1.2.6"
                  );
  }

  //
  // Insert a test variables with the vendor GUID2
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor2Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                      // DataSize
                 Data                     // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetVariable - Cannot insert a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetVariableConfTestSub6",
                    L"TDS 3.1.2.6 - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // GetVariable should not succeed with a nonexistent VendorGuid
  //
  DataSize = MAX_BUFFER_SIZE;
  Status   = RT->GetVariable (
                   L"TestVariable",       // VariableName
                   &gTestVendor1Guid,     // VendorGuid
                   NULL,                  // Attributes
                   &DataSize,             // DataSize
                   Data                   // Data
                   );
  if (Status == EFI_NOT_FOUND) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid006,
                 L"RT.GetVariable - With nonexistent VendorGuid",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_NOT_FOUND
                 );

  //
  // Delete the variable with the vendor GUID2 (restore environment)
  //
  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor2Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                       // DataSize
                 Data                     // Data
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub6",
                  L"TDS 3.1.2.6"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetVariable without enough DataSize.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetVariableConfTestSub7 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINTN               DataSize;
  UINT8               Data[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub7",
                  L"TDS 3.1.2.7"
                  );
  }

  //
  // Insert a variables with a specified DataSize
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                      // DataSize
                 Data                     // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetVariable - Cannot insert a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetVariableConfTestSub7",
                    L"TDS 3.1.2.7 - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // GetVariable should not succeed when DataSize is 0
  //
  DataSize = 0;
  Status   = RT->GetVariable (
                   L"TestVariable",       // VariableName
                   &gTestVendor1Guid,     // VendorGuid
                   NULL,                  // Attributes
                   &DataSize,             // DataSize
                   Data                   // Data
                   );
  if ((Status   == EFI_BUFFER_TOO_SMALL) &&
      (DataSize == 10))
  {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion & message
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid007,
                 L"RT.GetVariable - With DataSize is 0",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_BUFFER_TOO_SMALL
                 );

  StandardLib->RecordMessage (
                 StandardLib,
                 EFI_VERBOSE_LEVEL_DEFAULT,
                 L"DataSize=%d, Expected=%d\n",
                 DataSize,
                 10
                 );

  DataSize = 0;
  Status   = RT->GetVariable (
                   L"TestVariable",       // VariableName
                   &gTestVendor1Guid,     // VendorGuid
                   NULL,                  // Attributes
                   &DataSize,             // DataSize
                   NULL                   // Data
                   );
  if ((Status   == EFI_BUFFER_TOO_SMALL) &&
      (DataSize == 10))
  {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion & message
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid022,
                 L"RT.GetVariable - With DataSize is 0",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_BUFFER_TOO_SMALL
                 );

  StandardLib->RecordMessage (
                 StandardLib,
                 EFI_VERBOSE_LEVEL_DEFAULT,
                 L"DataSize=%d, Expected=%d\n",
                 DataSize,
                 10
                 );

  //
  // GetVariable should not succeed when DataSize is required - 1
  //
  DataSize = 9;
  Status   = RT->GetVariable (
                   L"TestVariable",       // VariableName
                   &gTestVendor1Guid,     // VendorGuid
                   NULL,                  // Attributes
                   &DataSize,             // DataSize
                   Data                   // Data
                   );
  if ((Status   == EFI_BUFFER_TOO_SMALL) &&
      (DataSize == 10))
  {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion & message
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid008,
                 L"RT.GetVariable - With DataSize is required-1",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_BUFFER_TOO_SMALL
                 );

  StandardLib->RecordMessage (
                 StandardLib,
                 EFI_VERBOSE_LEVEL_DEFAULT,
                 L"DataSize=%d, Expected=%d\n",
                 DataSize,
                 10
                 );

  //
  // Delete the variable (restore environment)
  //
  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                       // DataSize
                 Data                     // Data
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetVariableConfTestSub7",
                  L"TDS 3.1.2.7"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetNextVariableName when VariableNameSize is NULL.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetNextVariableNameConfTestSub1 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  CHAR16              VariableName[MAX_BUFFER_SIZE];
  EFI_GUID            VendorGuid;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub1",
                  L"TDS 3.2.2.1"
                  );
  }

  //
  // GetNextVariableName should not succeed when VariableNameSize is NULL
  //
  VariableName[0] = L'\0';
  Status          = RT->GetNextVariableName (
                          NULL,           // VariableNameSize
                          VariableName,   // VariableName
                          &VendorGuid     // VendorGuid
                          );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid009,
                 L"RT.GetNextVariableName - With VariableNameSize is NULL",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub1",
                  L"TDS 3.2.2.1"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetNextVariableName when VariableName is NULL.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetNextVariableNameConfTestSub2 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               VariableNameSize;
  EFI_GUID            VendorGuid;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub2",
                  L"TDS 3.2.2.2"
                  );
  }

  //
  // GetNextVariableName should not succeed when VariableNameSize is NULL
  //
  VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
  Status           = RT->GetNextVariableName (
                           &VariableNameSize, // VariableNameSize
                           NULL,              // VariableName
                           &VendorGuid        // VendorGuid
                           );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid010,
                 L"RT.GetNextVariableName - With VariableName is NULL",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub2",
                  L"TDS 3.2.2.2"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetNextVariableName when VendorGuid is NULL.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetNextVariableNameConfTestSub3 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               VariableNameSize;
  CHAR16              VariableName[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub3",
                  L"TDS 3.2.2.3"
                  );
  }

  //
  // GetNextVariableName should not succeed when VariableNameSize is NULL
  //
  VariableName[0]  = L'\0';
  VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
  Status           = RT->GetNextVariableName (
                           &VariableNameSize, // VariableNameSize
                           VariableName,      // VariableName
                           NULL               // VendorGuid
                           );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid011,
                 L"RT.GetNextVariableName - With VendorGuid is NULL",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub3",
                  L"TDS 3.2.2.3"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetNextVariableName without enough VariableNameSize.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetNextVariableNameConfTestSub4 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINT8               Data[MAX_BUFFER_SIZE];
  UINTN               VariableNameSize;
  CHAR16              VariableName[MAX_BUFFER_SIZE];
  EFI_GUID            VendorGuid;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub4",
                  L"TDS 3.2.2.4"
                  );
  }

  //
  // Insert a variable (at least one variable)
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                      // DataSize
                 Data                     // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetNextVariableName - Cannot insert a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetNextVariableNameConfTestSub4",
                    L"TDS 3.2.2.4 - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // GetNextVariableName should not succeed when VariableNameSize is 2
  // (The size of each variable should larger than 2.)
  //
  VariableName[0]  = L'\0';
  VariableNameSize = 2;
  Status           = RT->GetNextVariableName (
                           &VariableNameSize, // VariableNameSize
                           VariableName,      // VariableName
                           &VendorGuid        // VendorGuid
                           );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid012,
                 L"RT.GetNextVariableName - With VariableNameSize is 2",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_BUFFER_TOO_SMALL
                 );

  //
  // Delete a variable (restore environment)
  //
  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                       // DataSize
                 Data                     // Data
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub4",
                  L"TDS 3.2.2.4"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetNextVariableName after the entire variable list has been returned.
 *    NOTES: Also check GetNextVariableName will not return deleted variable.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetNextVariableNameConfTestSub5 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINT8               Data[MAX_BUFFER_SIZE];
  UINTN               VariableNameSize;
  CHAR16              VariableName[MAX_BUFFER_SIZE];
  EFI_GUID            VendorGuid;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub5",
                  L"TDS 3.2.2.5"
                  );
  }

  //
  // Insert a variable
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                      // DataSize
                 Data                     // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetNextVariableName - Cannot insert a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetNextVariableNameConfTestSub5",
                    L"TDS 3.2.2.5 - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // Delete the variable
  //
  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                       // DataSize
                 Data                     // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetNextVariableName - Cannot delete a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetNextVariableNameConfTestSub5",
                    L"TDS 3.2.2.5 - Cannot delete a variable"
                    );
    }

    return Status;
  }

  //
  // Walk through all variables
  //
  VariableName[0] = L'\0';
  Result          = EFI_TEST_ASSERTION_PASSED;

  while (TRUE) {
    VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
    Status           = RT->GetNextVariableName (
                             &VariableNameSize, // VariableNameSize
                             VariableName,      // VariableName
                             &VendorGuid        // VendorGuid
                             );
    if (EFI_ERROR (Status)) {
      if (Status != EFI_NOT_FOUND) {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      break;
    }

    if ((SctStrCmp (VariableName, L"TestVariable")       == 0) &&
        (SctCompareGuid (&VendorGuid, &gTestVendor1Guid) == 0))
    {
      Result = EFI_TEST_ASSERTION_FAILED;
      break;
    }
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid013,
                 L"RT.GetNextVariableName - After entire variable list returned",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_NOT_FOUND
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub5",
                  L"TDS 3.2.2.5"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetNextVariableName when a VariableName buffer on input is not a Null-terminated string.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetNextVariableNameConfTestSub6 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINT8               Data[MAX_BUFFER_SIZE];
  UINTN               VariableNameSize;
  CHAR16              VariableName[MAX_BUFFER_SIZE];
  EFI_GUID            VendorGuid;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub6",
                  L"TDS"
                  );
  }

  //
  // Insert a variable
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                      // DataSize
                 Data                     // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetNextVariableName - Cannot insert a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetNextVariableNameConfTestSub6",
                    L"TDS - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // Walk through all variables
  //
  VariableName[0]  = L'\0';
  VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
  Result           = EFI_TEST_ASSERTION_PASSED;

  while (TRUE) {
    Status = RT->GetNextVariableName (
                   &VariableNameSize,       // VariableNameSize
                   VariableName,            // VariableName
                   &VendorGuid              // VendorGuid
                   );
    if (EFI_ERROR (Status)) {
      if (Status != EFI_INVALID_PARAMETER) {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      break;
    }

    if ((SctStrCmp (VariableName, L"TestVariable")       == 0) &&
        (SctCompareGuid (&VendorGuid, &gTestVendor1Guid) == 0))
    {
      VariableNameSize = 8;
    } else {
      VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
    }
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid019,
                 L"RT.GetNextVariableName - when a VariableName buffer on input is not a Null-terminated string",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                       // DataSize
                 Data                     // Data
                 );

  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetNextVariableName - Cannot delete a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetNextVariableNameConfTestSub6",
                    L"TDS - Cannot delete a variable"
                    );
    }

    return Status;
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub6",
                  L"TDS"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  GetNextVariableName when input values of VariableName and VendorGuid are not a name and GUID of an existing variable.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetNextVariableNameConfTestSub7 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINT8               Data[MAX_BUFFER_SIZE];
  UINTN               VariableNameSize;
  CHAR16              VariableName[MAX_BUFFER_SIZE];
  EFI_GUID            VendorGuid;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub7",
                  L"TDS"
                  );
  }

  //
  // Insert a variable
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                      // DataSize
                 Data                     // Data
                 );
  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetNextVariableName - Cannot insert a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetNextVariableNameConfTestSub7",
                    L"TDS - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // Walk through all variables
  //
  VariableName[0] = L'\0';

  Result = EFI_TEST_ASSERTION_PASSED;

  while (TRUE) {
    VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
    Status           = RT->GetNextVariableName (
                             &VariableNameSize, // VariableNameSize
                             VariableName,      // VariableName
                             &VendorGuid        // VendorGuid
                             );
    if (EFI_ERROR (Status)) {
      if (Status != EFI_INVALID_PARAMETER) {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      break;
    }

    if ((SctStrCmp (VariableName, L"TestVariable")       == 0) &&
        (SctCompareGuid (&VendorGuid, &gTestVendor1Guid) == 0))
    {
      VariableName[8] = L'e';
    }
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid020,
                 L"RT.GetNextVariableName - when input values of VariableName and VendorGuid are not a name and GUID of an existing variable",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Walk through all variables
  //
  VariableName[0] = L'\0';

  Result = EFI_TEST_ASSERTION_PASSED;

  while (TRUE) {
    VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
    Status           = RT->GetNextVariableName (
                             &VariableNameSize, // VariableNameSize
                             VariableName,      // VariableName
                             &VendorGuid        // VendorGuid
                             );
    if (EFI_ERROR (Status)) {
      if (Status != EFI_INVALID_PARAMETER) {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      break;
    }

    if ((SctStrCmp (VariableName, L"TestVariable")       == 0) &&
        (SctCompareGuid (&VendorGuid, &gTestVendor1Guid) == 0))
    {
      VendorGuid = gTestVendor2Guid;
    }
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid020,
                 L"RT.GetNextVariableName - when input values of VariableName and VendorGuid are not a name and GUID of an existing variable",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,                       // DataSize
                 Data                     // Data
                 );

  if (EFI_ERROR (Status)) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   EFI_TEST_ASSERTION_WARNING,
                   gTestGenericFailureGuid,
                   L"RT.GetNextVariableName - Cannot delete a variable",
                   L"%a:%d:Status - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status
                   );

    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"GetNextVariableNameConfTestSub6",
                    L"TDS - Cannot delete a variable"
                    );
    }

    return Status;
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetNextVariableNameConfTestSub6",
                  L"TDS"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  SetVariable when VariableName is an empty string.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
SetVariableConfTestSub1 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINT8               Data[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"SetVariableConfTestSub1",
                  L"TDS 3.3.2.1"
                  );
  }

  //
  // SetVariable should not succeed when VariableName is an empty string
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"",                     // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 10,                      // DataSize
                 Data                     // Data
                 );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid014,
                 L"RT.SetVariable - With VariableName is empty string",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableConfTestSub1",
                  L"TDS 3.3.2.1"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  SetVariable with invalid combination of attribute bits.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
SetVariableConfTestSub2 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINT8               Data[MAX_BUFFER_SIZE];

  UINTN       Index;
  UINTN       SubIndex;
  UINTN       SubIndex1;
  EFI_TPL     OldTpl;
  EFI_STATUS  ReturnedStatus;
  UINTN       DataSize;
  EFI_TPL     TplArray[] = { TPL_APPLICATION, TPL_CALLBACK };
  UINT32      Attributes;
  UINT32      AttributesArray[] = {
    EFI_VARIABLE_BOOTSERVICE_ACCESS,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
  };

  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"SetVariableConfTestSub2",
                  L"TDS 3.3.2.2"
                  );
  }

  //
  // SetVariable should not succeed when Attributes is RA
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_RUNTIME_ACCESS,
                 10,                      // DataSize
                 Data                     // Data
                 );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid015,
                 L"RT.SetVariable - With Attributes is RA",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // SetVariable should not succeed when Attributes is NV | RA
  //
  for (DataIndex = 0; DataIndex < 10; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS,
                 10,                      // DataSize
                 Data                     // Data
                 );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid016,
                 L"RT.SetVariable - With Attributes is NV|RA",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // for each TPL less than or equal to TPL_CALLBACK do
  //
  for (Index = 0; Index < 2; Index++) {
    //
    // For each Attributes of BA, NV|BA, BA|RA and NV|BA|RA do
    //
    for (SubIndex = 0; SubIndex < 4; SubIndex++) {
      //
      // Insert a variable firstly
      //
      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = (UINT8)DataIndex;
      }

      Attributes = AttributesArray[SubIndex];
      Status     = RT->SetVariable (
                         L"TestVariable",         // VariableName
                         &gTestVendor1Guid,       // VendorGuid
                         Attributes,              // Attributes
                         10,                      // DataSize
                         Data                     // Data
                         );
      if (EFI_ERROR (Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot insert a variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = 10 - (UINT8)DataIndex;
      }

      for (SubIndex1 = 0; SubIndex1 < 4; SubIndex1++) {
        if (Attributes != AttributesArray[SubIndex1]) {
          OldTpl = gtBS->RaiseTPL (TplArray[Index]);

          ReturnedStatus = RT->SetVariable (
                                 L"TestVariable",             // VariableName
                                 &gTestVendor1Guid,           // VendorGuid
                                 AttributesArray[SubIndex1],  // Attributes
                                 10,                          // DataSize
                                 Data                         // Data
                                 );

          gtBS->RestoreTPL (OldTpl);

          if (ReturnedStatus == EFI_INVALID_PARAMETER) {
            Result = EFI_TEST_ASSERTION_PASSED;
          } else {
            Result = EFI_TEST_ASSERTION_FAILED;
          }

          DataSize = MAX_BUFFER_SIZE;
          RT->GetVariable (
                L"TestVariable",             // VariableName
                &gTestVendor1Guid,           // VendorGuid
                NULL,                        // Attributes
                &DataSize,                   // DataSize
                Data                         // Data
                );

          if (DataSize != 10) {
            Result = EFI_TEST_ASSERTION_FAILED;
          } else {
            for (DataIndex = 0; DataIndex < 10; DataIndex++) {
              if (Data[DataIndex] != (UINT8)DataIndex) {
                Result = EFI_TEST_ASSERTION_FAILED;
                break;
              }
            }
          }

          //
          // Record assertion
          //
          StandardLib->RecordAssertion (
                         StandardLib,
                         Result,
                         gVariableServicesBbTestConformanceAssertionGuid021,
                         L"RT.SetVariable - With different Attributes",
                         L"%a:%d:Status - %r, Expected - %r",
                         __FILE__,
                         (UINTN)__LINE__,
                         ReturnedStatus,
                         EFI_INVALID_PARAMETER
                         );
        }
      }

      if (Status == EFI_SUCCESS) {
        RT->SetVariable (
              L"TestVariable",             // VariableName
              &gTestVendor1Guid,           // VendorGuid
              AttributesArray[SubIndex],   // Attributes
              0,                           // DataSize
              Data                         // Data
              );
      }
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableConfTestSub2",
                  L"TDS 3.3.2.2"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  SetVariable with too large variable.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
SetVariableConfTestSub3 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_TEST_ASSERTION  Result;
  UINTN               DataIndex;
  UINT8               Data[MAX_BUFFER_SIZE];

  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"SetVariableConfTestSub3",
                  L"TDS 3.3.2.3"
                  );
  }

  //
  // Only initialize the Data buffer from 0 to MAX_BUFFER_SIZE
  //
  for (DataIndex = 0; DataIndex < MAX_BUFFER_SIZE; DataIndex++) {
    Data[DataIndex] = (UINT8)DataIndex;
  }

  Status = RT->SetVariable (
                 L"TestVariable",         // VariableName
                 &gTestVendor1Guid,       // VendorGuid
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 (UINTN)-1,               // DataSize
                 Data                     // Data
                 );
  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestConformanceAssertionGuid017,
                 L"RT.SetVariable - With too large variable",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableConfTestSub3",
                  L"TDS 3.3.2.3"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
 *  SetVariable(delete) a variable while it isn't existed.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
SetVariableConfTestSub4 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib
  )
{
  EFI_STATUS          Status;
  EFI_STATUS          ReturnedStatus;
  EFI_TEST_ASSERTION  Result;
  UINTN               Index;
  UINTN               SubIndex;
  UINTN               DataIndex;
  UINT8               Data[MAX_BUFFER_SIZE];
  EFI_TPL             OldTpl;
  EFI_TPL             TplArray[]        = { TPL_APPLICATION, TPL_CALLBACK };
  UINT32              AttributesArray[] = {
    EFI_VARIABLE_BOOTSERVICE_ACCESS,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
  };

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"SetVariableConfTestSub4",
                  L"Add in UEFI2.1"
                  );
  }

  //
  // for each TPL less than or equal to TPL_CALLBACK do
  //
  for (Index = 0; Index < 2; Index++) {
    //
    // For each Attributes of BA, NV|BA, BA|RA and NV|BA|RA do
    //
    for (SubIndex = 0; SubIndex < 4; SubIndex++) {
      //
      // Insert a variable
      //
      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = (UINT8)DataIndex;
      }

      Status = RT->SetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     10,                          // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR (Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot insert a variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      //
      // Delete the variable with DataSize is 0
      //
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      ReturnedStatus = RT->SetVariable (
                             L"TestVariable",             // VariableName
                             &gTestVendor1Guid,           // VendorGuid
                             AttributesArray[SubIndex],   // Attributes
                             0,                           // DataSize
                             Data                         // Data
                             );

      gtBS->RestoreTPL (OldTpl);

      //
      // Delete the variable again to check results
      //

      Status = RT->SetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );

      //
      // Check results
      //
      if ((ReturnedStatus == EFI_SUCCESS) &&
          (Status         == EFI_NOT_FOUND))
      {
        Result = EFI_TEST_ASSERTION_PASSED;
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     gVariableServicesBbTestConformanceAssertionGuid018,
                     L"RT.SetVariable - With DataSize is 0",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     ReturnedStatus,
                     EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"SetVariable: Status - %r, Expected - %r\n",
                     Status,
                     EFI_NOT_FOUND
                     );
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableConfTestSub4",
                  L"Add in UEFI2.1"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
//
//  QueryVariableInfo test case
//
EFI_STATUS
QueryVariableInfoConfTest (
  IN EFI_BB_TEST_PROTOCOL  *This,
  IN VOID                  *ClientInterface,
  IN EFI_TEST_LEVEL        TestLevel,
  IN EFI_HANDLE            SupportHandle
  )
{
  EFI_STATUS                          Status;
  EFI_RUNTIME_SERVICES                *RT;
  EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib;
  EFI_TEST_RECOVERY_LIBRARY_PROTOCOL  *RecoveryLib;
  EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib;

  //
  // Get test support library interfaces
  //
  Status = GetTestSupportLibrary (
             SupportHandle,
             &StandardLib,
             &RecoveryLib,
             &LoggingLib
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // if (FALSE == CheckBBTestCanRunAndRecordAssertion(
  //                 StandardLib,
  //                 L"RT.QueryVariableInfo_Conf - QueryVariableInfo_Conf it's not Supported in EFI",
  //                 __FILE__,
  //                 (UINTN)__LINE__
  //                 )) {
  //   return EFI_SUCCESS;
  // }

  RT = (EFI_RUNTIME_SERVICES *)ClientInterface;
 #if 0
  //
  // QueryVariableInfo with MaximumVariableStorageSize being NULL
  //
  Status = QueryVariableInfoConfTestSub1 (RT, StandardLib, LoggingLib, SupportHandle);

  //
  // QueryVariableInfo with RemainingVariableStorageSize being NULL
  //
  Status = QueryVariableInfoConfTestSub2 (RT, StandardLib, LoggingLib, SupportHandle);

  //
  // QueryVariableInfo with MaximumVariableSize being NULL
  //
  Status = QueryVariableInfoConfTestSub3 (RT, StandardLib, LoggingLib, SupportHandle);
 #endif
  //
  // QueryVariableInfo with Attributes being 0
  //
  Status = QueryVariableInfoConfTestSub4 (RT, StandardLib, LoggingLib, SupportHandle);

  //
  // QueryVariableInfo with Attributes being an invalid combination
  //
  Status = QueryVariableInfoConfTestSub5 (RT, StandardLib, LoggingLib, SupportHandle);

  return EFI_SUCCESS;
}

//
// QueryVariableInfo with MaximumVariableStorageSize being NULL
//
EFI_STATUS
QueryVariableInfoConfTestSub1 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  )
{
  EFI_STATUS  Status;
  UINT32      ValidAttributes[] = {
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS,
    EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_BOOTSERVICE_ACCESS,
    0
  };
  // UINT64                MaximumVariableStorageSize;
  UINT64              RemainingVariableStorageSize;
  UINT64              MaximumVariableSize;
  EFI_TEST_ASSERTION  Result;
  UINTN               Index;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub1",
                  L"TDS 3.4.2.1"
                  );
  }

  //  if(1) return EFI_SUCCESS;
  for (Index = 0; ValidAttributes[Index] != 0; Index++) {
    Status = RT->QueryVariableInfo (
                   ValidAttributes[Index],
                   NULL,
                   &RemainingVariableStorageSize,
                   &MaximumVariableSize
                   );

    if (Status == EFI_UNSUPPORTED) {
      Result = EFI_TEST_ASSERTION_PASSED;
    } else {
      Result = EFI_TEST_ASSERTION_FAILED;
    }

    //
    // Record assertion
    //
    StandardLib->RecordAssertion (
                   StandardLib,
                   Result,
                   gVariableServicesBbTestQueryVarInfoAssertionGuid001,
                   L"RT.QueryVariableInfo - With MaximumVariableStorageSize being NULL",
                   L"%a:%d:Status - %r, Expected - %r (Variable Attribute %d)",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status,
                   EFI_UNSUPPORTED,
                   ValidAttributes[Index]
                   );
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub1",
                  L"TDS 3.4.2.1"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

//
// QueryVariableInfo with RemainingVariableStorageSize being NULL
//
EFI_STATUS
QueryVariableInfoConfTestSub2 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  )
{
  EFI_STATUS  Status;
  UINT32      ValidAttributes[] = {
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS,
    EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_BOOTSERVICE_ACCESS,
    0
  };
  UINT64      MaximumVariableStorageSize;
  // UINT64                RemainingVariableStorageSize;
  UINT64              MaximumVariableSize;
  EFI_TEST_ASSERTION  Result;
  UINTN               Index;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub2",
                  L"TDS 3.4.2.2"
                  );
  }

  for (Index = 0; ValidAttributes[Index] != 0; Index++) {
    Status = RT->QueryVariableInfo (
                   ValidAttributes[Index],
                   &MaximumVariableStorageSize,
                   NULL,
                   &MaximumVariableSize
                   );

    if (Status == EFI_UNSUPPORTED) {
      Result = EFI_TEST_ASSERTION_PASSED;
    } else {
      Result = EFI_TEST_ASSERTION_FAILED;
    }

    //
    // Record assertion
    //
    StandardLib->RecordAssertion (
                   StandardLib,
                   Result,
                   gVariableServicesBbTestQueryVarInfoAssertionGuid002,
                   L"RT.QueryVariableInfo - With RemainingVariableStorageSize being NULL",
                   L"%a:%d:Status - %r, Expected - %r (Variable Attribute %d)",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status,
                   EFI_UNSUPPORTED,
                   ValidAttributes[Index]
                   );
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub2",
                  L"TDS 3.4.2.2"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

//
// QueryVariableInfo with MaximumVariableSize being NULL
//
EFI_STATUS
QueryVariableInfoConfTestSub3 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  )
{
  EFI_STATUS  Status;
  UINT32      ValidAttributes[] = {
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS,
    EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_BOOTSERVICE_ACCESS,
    0
  };
  UINT64      MaximumVariableStorageSize;
  UINT64      RemainingVariableStorageSize;
  // UINT64                MaximumVariableSize;
  EFI_TEST_ASSERTION  Result;
  UINTN               Index;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub3",
                  L"TDS 3.4.2.3"
                  );
  }

  for (Index = 0; ValidAttributes[Index] != 0; Index++) {
    Status = RT->QueryVariableInfo (
                   ValidAttributes[Index],
                   &MaximumVariableStorageSize,
                   &RemainingVariableStorageSize,
                   NULL
                   );

    if (Status == EFI_UNSUPPORTED) {
      Result = EFI_TEST_ASSERTION_PASSED;
    } else {
      Result = EFI_TEST_ASSERTION_FAILED;
    }

    //
    // Record assertion
    //
    StandardLib->RecordAssertion (
                   StandardLib,
                   Result,
                   gVariableServicesBbTestQueryVarInfoAssertionGuid003,
                   L"RT.QueryVariableInfo - With MaximumVariableSize being NULL",
                   L"%a:%d:Status - %r, Expected - %r (Variable Attribute %d)",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status,
                   EFI_UNSUPPORTED,
                   ValidAttributes[Index]
                   );
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub3",
                  L"TDS 3.4.2.3"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

//
// QueryVariableInfo with Attributes being 0
//
EFI_STATUS
QueryVariableInfoConfTestSub4 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  )
{
  EFI_STATUS          Status;
  UINT64              MaximumVariableStorageSize;
  UINT64              RemainingVariableStorageSize;
  UINT64              MaximumVariableSize;
  EFI_TEST_ASSERTION  Result;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub4",
                  L"TDS 3.4.2.4"
                  );
  }

  Status = RT->QueryVariableInfo (
                 0,
                 &MaximumVariableStorageSize,
                 &RemainingVariableStorageSize,
                 &MaximumVariableSize
                 );

  if (Status == EFI_INVALID_PARAMETER) {
    Result = EFI_TEST_ASSERTION_PASSED;
  } else {
    Result = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 Result,
                 gVariableServicesBbTestQueryVarInfoAssertionGuid004,
                 L"RT.QueryVariableInfo - With Attributes being 0",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status,
                 EFI_INVALID_PARAMETER
                 );

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub4",
                  L"TDS 3.4.2.4"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

//
// QueryVariableInfo with Attributes being an invalid combination
//
EFI_STATUS
QueryVariableInfoConfTestSub5 (
  IN EFI_RUNTIME_SERVICES                *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib,
  IN EFI_HANDLE                          SupportHandle
  )
{
  EFI_STATUS          Status;
  UINT32              InvalidAttributes[] = {
    EFI_VARIABLE_NON_VOLATILE,
    EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_RUNTIME_ACCESS,
    0
  };
  UINT64              MaximumVariableStorageSize;
  UINT64              RemainingVariableStorageSize;
  UINT64              MaximumVariableSize;
  EFI_TEST_ASSERTION  Result;
  UINTN               Index;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub5",
                  L"TDS 3.4.2.5"
                  );
  }

  for (Index = 0; InvalidAttributes[Index] != 0; Index++) {
    Status = RT->QueryVariableInfo (
                   InvalidAttributes[Index],
                   &MaximumVariableStorageSize,
                   &RemainingVariableStorageSize,
                   &MaximumVariableSize
                   );

    if (Status == EFI_INVALID_PARAMETER) {
      Result = EFI_TEST_ASSERTION_PASSED;
    } else {
      Result = EFI_TEST_ASSERTION_FAILED;
    }

    //
    // Record assertion
    //
    StandardLib->RecordAssertion (
                   StandardLib,
                   Result,
                   gVariableServicesBbTestQueryVarInfoAssertionGuid005,
                   L"RT.QueryVariableInfo - With being an invalid combination",
                   L"%a:%d:Status - %r, Expected - %r (Variable Attribute 0x%lx)",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status,
                   EFI_INVALID_PARAMETER,
                   InvalidAttributes[Index]
                   );
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"QueryVariableInfoConfTestSub5",
                  L"TDS 3.4.2.5"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

#endif

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)

EFI_STATUS
HardwareErrorRecordConfTest (
  IN EFI_BB_TEST_PROTOCOL  *This,
  IN VOID                  *ClientInterface,
  IN EFI_TEST_LEVEL        TestLevel,
  IN EFI_HANDLE            SupportHandle
  )
{
  EFI_STATUS                          Status, Status2;
  EFI_RUNTIME_SERVICES                *RT;
  EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib;
  EFI_TEST_RECOVERY_LIBRARY_PROTOCOL  *RecoveryLib;
  EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib;

  CHAR16  *HwErrRecName = L"HwErrRecSupport";
  UINT16  HwErrRecSupportVariable;
  UINTN   DataSize;

  UINT32  Attributes = EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS
                       |EFI_VARIABLE_RUNTIME_ACCESS|EFI_VARIABLE_HARDWARE_ERROR_RECORD;
  UINT64  MaximumVariableStorageSize;
  UINT64  RemainingVariableStorageSize;
  UINT64  MaximumVariableSize;

  CHAR16  HwErrRecVariableName[13];
  CHAR16  HwErrRecVariable[] = L"This is a HwErrRec variable!";

  CHAR16    GetVariableName[MAX_BUFFER_SIZE];
  UINTN     VariableNameSize;
  EFI_GUID  VendorGuid;
  CHAR16    *P = NULL;

  UINTN   Num;
  UINTN   MaxNum = 0;
  CHAR16  ErrorNum[5];

  UINT32  InvalidAttributes[] = {
    EFI_VARIABLE_HARDWARE_ERROR_RECORD | EFI_VARIABLE_RUNTIME_ACCESS,
    EFI_VARIABLE_HARDWARE_ERROR_RECORD | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
    0
  };

  UINTN  Index;

  EFI_TEST_ASSERTION  AssertionType;

  //
  // Get test support library interfaces
  //
  Status = GetTestSupportLibrary (
             SupportHandle,
             &StandardLib,
             &RecoveryLib,
             &LoggingLib
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RT = (EFI_RUNTIME_SERVICES *)ClientInterface;

  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"HardwareErrorRecord Func Test",
                  L"TDS"
                  );
  }

  //
  // Check if the platform implements support for Hardware Error Record Persistence
  //
  DataSize                = sizeof (HwErrRecSupportVariable);
  HwErrRecSupportVariable = 0xFFFF;
  Status                  = RT->GetVariable (
                                  HwErrRecName,
                                  &gGlobalVariableGuid,
                                  NULL,
                                  &DataSize,
                                  &HwErrRecSupportVariable
                                  );

  StandardLib->RecordMessage (
                 StandardLib,
                 EFI_VERBOSE_LEVEL_DEFAULT,
                 L"\r\nGetVariable() to get the HwErrRecSupport : %r %d",
                 Status,
                 HwErrRecSupportVariable
                 );

  if ((EFI_SUCCESS != Status) || !HwErrRecSupportVariable ) {
    return Status;
  }

  //
  // Query the variable info
  //
  Status = RT->QueryVariableInfo (
                 Attributes,
                 &MaximumVariableStorageSize,
                 &RemainingVariableStorageSize,
                 &MaximumVariableSize
                 );

  StandardLib->RecordMessage (
                 StandardLib,
                 EFI_VERBOSE_LEVEL_DEFAULT,
                 L"\r\n Query the variable info : MaximumVariableStorageSize - %d, RemainingVariableStorageSize - %d, MaximumVariableSize - %d",
                 MaximumVariableStorageSize,
                 RemainingVariableStorageSize,
                 MaximumVariableSize
                 );

  if ((Status != EFI_SUCCESS) || (RemainingVariableStorageSize <= 0)) {
    return Status;
  }

  //
  // Get a useable variable name
  //
  GetVariableName[0] = L'\0';
  ErrorNum[4]        = L'\0';
  AssertionType      = EFI_TEST_ASSERTION_PASSED;

  while (TRUE) {
    VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
    Status           = RT->GetNextVariableName (
                             &VariableNameSize,   // VariableNameSize
                             GetVariableName,     // VariableName
                             &VendorGuid          // VendorGuid
                             );

    StandardLib->RecordMessage (
                   StandardLib,
                   EFI_VERBOSE_LEVEL_DEFAULT,
                   L"\r\nGetNextVariableName() to get the variable name : %s - %r",
                   GetVariableName,
                   Status
                   );

    if ( EFI_SUCCESS != Status ) {
      break;
    }

    if ((SctStrnCmp (GetVariableName, L"HwErrRec", 8) == 0) &&
        (SctCompareGuid (&VendorGuid, &gHwErrRecGuid) == 0))
    {
      P = GetVariableName;
      if (SctStrLen (P) == 12) {
        P = GetVariableName;
        if ((((P[8] >= '0') && (P[8] <= '9')) || ((P[8] >= 'A') && (P[8] <= 'F')) || ((P[8] >= 'a') && (P[8] <= 'f'))) &&
            (((P[9] >= '0') && (P[9] <= '9')) || ((P[9] >= 'A') && (P[9] <= 'F')) || ((P[9] >= 'a') && (P[9] <= 'f'))) &&
            (((P[10] >= '0') && (P[10] <= '9')) || ((P[10] >= 'A') && (P[10] <= 'F')) || ((P[10] >= 'a') && (P[10] <= 'f'))) &&
            (((P[11] >= '0') && (P[11] <= '9')) || ((P[11] >= 'A') && (P[11] <= 'F')) || ((P[11] >= 'a') && (P[11] <= 'f'))))
        {
          AssertionType = EFI_TEST_ASSERTION_PASSED;
        } else {
          AssertionType = EFI_TEST_ASSERTION_FAILED;
          break;
        }
      } else {
        AssertionType = EFI_TEST_ASSERTION_FAILED;
        break;
      }

      SctStrnCpy (ErrorNum, &GetVariableName[8], 4);
      Num = SctXtoi (ErrorNum);
      if (MaxNum < Num) {
        MaxNum = Num;
      }
    }
  }

  if (P != NULL) {
    StandardLib->RecordAssertion (
                   StandardLib,
                   AssertionType,
                   gHwErrRecBbTestAssertionGuid004,
                   L"RT.SetVariable - Retrive the Hardware Error Record variables, check the name of them",
                   L"%a:%d:Status - %r, Expected - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status,
                   EFI_SUCCESS
                   );
  }

  if (AssertionType == EFI_TEST_ASSERTION_FAILED) {
    return EFI_SUCCESS;
  }

  MaxNum++;

  HwErrRecVariableName[0] = L'\0';
  SctStrCat (HwErrRecVariableName, L"HwErrRec");
  Myitox (MaxNum, HwErrRecVariableName+8);

  //
  // Call SetVariable with invalid attributes
  //
  for ( Index = 0; InvalidAttributes[Index]; Index++ ) {
    Status = RT->SetVariable (
                   HwErrRecVariableName,
                   &gHwErrRecGuid,
                   InvalidAttributes[Index],
                   DataSize,
                   HwErrRecVariable
                   );
    if ((Status == EFI_INVALID_PARAMETER)) {
      AssertionType = EFI_TEST_ASSERTION_PASSED;
    } else {
      AssertionType = EFI_TEST_ASSERTION_FAILED;
    }

    StandardLib->RecordAssertion (
                   StandardLib,
                   AssertionType,
                   gHwErrRecBbTestAssertionGuid002,
                   L"RT.SetVariable - With invalid attributes and attributes has EFI_VARIABLE_HARDWARE_ERROR_RECORD",
                   L"%a:%d:Status - %r, Expected - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Status,
                   EFI_INVALID_PARAMETER
                   );
  }

  //
  // Set the new HwErrRec variable to the global variable
  //
  DataSize = sizeof (HwErrRecVariable);
  Status   = RT->SetVariable (
                   HwErrRecVariableName,
                   &gHwErrRecGuid,
                   Attributes,
                   DataSize,
                   HwErrRecVariable
                   );
  if ( EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Delete the HwErrRecVariable
  //
  Status = RT->SetVariable (
                 HwErrRecVariableName,
                 &gHwErrRecGuid,
                 Attributes,
                 0,
                 HwErrRecVariable
                 );

  //
  // Delete the HwErrRecVariable again
  //
  Status2 = RT->SetVariable (
                  HwErrRecVariableName,
                  &gHwErrRecGuid,
                  Attributes,
                  0,
                  HwErrRecVariable
                  );

  if ((Status == EFI_SUCCESS) && (Status2 == EFI_NOT_FOUND)) {
    AssertionType = EFI_TEST_ASSERTION_PASSED;
  } else {
    AssertionType = EFI_TEST_ASSERTION_FAILED;
  }

  //
  // Record assertion & message
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 AssertionType,
                 gHwErrRecBbTestAssertionGuid003,
                 L"RT.SetVariable - With DataSize is 0 and attributes has EFI_VARIABLE_HARDWARE_ERROR_RECORD",
                 L"%a:%d:Status - %r, Expected - %r",
                 __FILE__,
                 (UINTN)__LINE__,
                 Status2,
                 EFI_NOT_FOUND
                 );

  //
  // Return
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"HardwareErrorRecord Func Test",
                  L"TDS"
                  );
  }

  return EFI_SUCCESS;
}

#endif
