/** @file

  Copyright 2006 - 2012 Unified EFI, Inc.<BR>
  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at 
  http://opensource.org/licenses/bsd-license.php
 
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
**/
/*++

Module Name:
  VariableServicesBbTestStress.c

Abstract:
  Source file for Variable Services Stress Black-Box Test - Stress Test.

--*/

#include "SctLib.h"
#include "VariableServicesBBTestMain.h"

#define MULTIPLE_TEST_TIMES         50

//
// Prototypes (external)
//

EFI_STATUS
MultipleStressTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

EFI_STATUS
OverflowStressTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

//
// Prototypes (internal)
//

EFI_STATUS
MultipleStressTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
MultipleStressTestSub2 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
MultipleStressTestSub3 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
OverflowStressTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_RECOVERY_LIBRARY_PROTOCOL   *RecoveryLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

//
// Functions
//

/**
 *  TDS 5.1 - Entry point for GetVariable() and SetVariable() Stress Test.
 *  @param This             A pointer to the EFI_BB_TEST_PROTOCOL instance.
 *  @param ClientInterface  A pointer to the interface to be tested.
 *  @param TestLevel        Test "thoroughness" control.
 *  @param SupportHandle    A handle containing support protocols.
 *  @return EFI_SUCCESS     Successfully.
 *  @return Other value     Something failed.
 */
EFI_STATUS
MultipleStressTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
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
  if (EFI_ERROR(Status)) {
    return Status;
  }

  RT = (EFI_RUNTIME_SERVICES *)ClientInterface;

  //
  // Stress test for GetVariable
  //
  Status = MultipleStressTestSub1 (RT, StandardLib, LoggingLib);

  //
  // Stress test for GetNextVariableName
  //
  Status = MultipleStressTestSub2 (RT, StandardLib, LoggingLib);

  //
  // Stress test for SetVariable
  //
  Status = MultipleStressTestSub3 (RT, StandardLib, LoggingLib);

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  TDS 5.2 - Entry point for Variable Overflow Stress Test.
 *  @param This             A pointer to the EFI_BB_TEST_PROTOCOL instance.
 *  @param ClientInterface  A pointer to the interface to be tested.
 *  @param TestLevel        Test "thoroughness" control.
 *  @param SupportHandle    A handle containing support protocols.
 *  @return EFI_SUCCESS     Successfully.
 *  @return Other value     Something failed.
 */
EFI_STATUS
OverflowStressTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  )
{
  EFI_STATUS                          Status;
  EFI_RUNTIME_SERVICES                *RT;
  EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *StandardLib;
  EFI_TEST_RECOVERY_LIBRARY_PROTOCOL  *RecoveryLib;
  EFI_TEST_LOGGING_LIBRARY_PROTOCOL   *LoggingLib;
  UINTN                               RecoveryDataSize;
  UINT8                               *RecoveryData;

  //
  // Get test support library interfaces
  //
  Status = GetTestSupportLibrary (
             SupportHandle,
             &StandardLib,
             &RecoveryLib,
             &LoggingLib
             );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  RT = (EFI_RUNTIME_SERVICES *)ClientInterface;

  //
  // Allocate memory for recovery data
  //
  Status = gtBS->AllocatePool (
                   EfiLoaderData,
                   1024,
                   (VOID **)&RecoveryData
                   );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Read reset record
  //
  RecoveryDataSize = 1024;
  Status = RecoveryLib->ReadResetRecord (
                          RecoveryLib,
                          &RecoveryDataSize,
                          RecoveryData
                          );
  if (!EFI_ERROR(Status) && (RecoveryDataSize > 0)) {
    switch (RecoveryData[0]) {
    case 2:
      goto step2;
    default:
      goto step3;
    }
  }

  //
  // Reclaim test with system reset
  //
step2:
  Status = OverflowStressTestSub1 (RT, StandardLib, RecoveryLib, LoggingLib);


  //
  // Free resources
  //
step3:
  gtBS->FreePool (RecoveryData);

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  Multiple test for GetVariable.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
MultipleStressTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 DataIndex;
  UINTN                 DataSize;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINT32                Attributes;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetSetStressTestSub1",
                  L"TDS 5.1.2.1"
                  );
  }

  //
  // Insert a variable
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
  if (EFI_ERROR(Status)) {
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
                    L"GetSetStressTestSub1",
                    L"TDS 5.1.2.1 - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // Get the variable multiple times
  //
  for (Index = 0; Index < MULTIPLE_TEST_TIMES; Index++) {
    DataSize = MAX_BUFFER_SIZE;
    Status = RT->GetVariable (
                   L"TestVariable",             // VariableName
                   &gTestVendor1Guid,           // VendorGuid
                   &Attributes,                 // Attributes
                   &DataSize,                   // DataSize
                   Data                         // Data
                   );
    if (Status == EFI_SUCCESS) {
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
                   gVariableServicesBbTestStressAssertionGuid001,
                   L"RT.GetVariable - GetVariable multiple times",
                   L"%a:%d:Index - %d, Status - %r, Expected - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Index,
                   Status,      EFI_SUCCESS
                   );
  }

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
                  L"GetSetStressTestSub1",
                  L"TDS 5.1.2.1"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  Multiple test for GetNextVariableName.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
MultipleStressTestSub2 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 DataIndex;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINTN                 VariableNameSize;
  CHAR16                VariableName[MAX_BUFFER_SIZE];
  EFI_GUID              VendorGuid;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetSetStressTestSub2",
                  L"TDS 5.1.2.2"
                  );
  }

  //
  // Insert a variable (at least one variable exist)
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
  if (EFI_ERROR(Status)) {
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
                    L"GetSetStressTestSub2",
                    L"TDS 5.1.2.2 - Cannot insert a variable"
                    );
    }

    return Status;
  }

  //
  // Get the next variable name multiple times
  //
  for (Index = 0; Index < MULTIPLE_TEST_TIMES; Index++) {

    VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
    VariableName[0] = L'\0';
    Status = RT->GetNextVariableName (
                   &VariableNameSize,
                   VariableName,
                   &VendorGuid
                   );
    if (Status == EFI_SUCCESS) {
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
                   gVariableServicesBbTestStressAssertionGuid002,
                   L"RT.GetNextVariableName - GetNextVariableName multiple times",
                   L"%a:%d:Index - %d, Status - %r, Expected - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Index,
                   Status,      EFI_SUCCESS
                   );
  }

  //
  // Delete a variable (restore environment)
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
                  L"GetSetStressTestSub2",
                  L"TDS 5.1.2.2"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  Multiple test for SetVariable.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
MultipleStressTestSub3 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 DataIndex;
  UINT8                 Data[MAX_BUFFER_SIZE];

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"GetSetStressTestSub3",
                  L"TDS 5.1.2.3"
                  );
  }

  //
  // Insert a variable multiple times with the same data
  //
  for (Index = 0; Index < MULTIPLE_TEST_TIMES; Index++) {
    //
    // Insert a variable
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
    if (Status == EFI_SUCCESS) {
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
                   gVariableServicesBbTestStressAssertionGuid003,
                   L"RT.SetVariable - SetVariable with same data multiple times",
                   L"%a:%d:Index - %d, Status - %r, Expected - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Index,
                   Status,      EFI_SUCCESS
                   );
  }

  //
  // Insert a variable multiple times with the different data
  //
  for (Index = 0; Index < MULTIPLE_TEST_TIMES; Index++) {
    //
    // Insert a variable
    //
    for (DataIndex = 0; DataIndex < 10; DataIndex++) {
      Data[DataIndex] = (UINT8)(Index + DataIndex);
    }

    Status = RT->SetVariable (
                   L"TestVariable",             // VariableName
                   &gTestVendor1Guid,           // VendorGuid
                   EFI_VARIABLE_BOOTSERVICE_ACCESS,
                   10,                          // DataSize
                   Data                         // Data
                   );
    if (Status == EFI_SUCCESS) {
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
                   gVariableServicesBbTestStressAssertionGuid004,
                   L"RT.SetVariable - SetVariable with different data multiple times",
                   L"%a:%d:Index - %d, Status - %r, Expected - %r",
                   __FILE__,
                   (UINTN)__LINE__,
                   Index,
                   Status,      EFI_SUCCESS
                   );
  }

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
                  L"GetSetStressTestSub3",
                  L"TDS 5.1.2.3"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  Reclaim test with system reset.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param RecoveryLib    A pointer to EFI_TEST_RECOVERY_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
OverflowStressTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_RECOVERY_LIBRARY_PROTOCOL   *RecoveryLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  CHAR16                *VariableName;
  UINTN                 DataIndex;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINTN                 RecoveryDataSize;
  UINT8                 *RecoveryData;

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"OverflowStressTestSub2",
                  L"TDS 5.2.2.2"
                  );
  }

  //
  // Allocate memory for recovery data
  //
  Status = gtBS->AllocatePool (
                   EfiLoaderData,
                   1024,
                   (VOID **)&RecoveryData
                   );
  if (EFI_ERROR(Status)) {
    if (LoggingLib != NULL) {
      LoggingLib->ExitFunction (
                    LoggingLib,
                    L"OverflowStressTestSub2",
                    L"TDS 5.2.2.2 - Cannot allocate memory for recovery data\n"
                    );
    }

    return Status;
  }

  //
  // Read reset record
  //
  RecoveryDataSize = 1024;
  Status = RecoveryLib->ReadResetRecord (
                          RecoveryLib,
                          &RecoveryDataSize,
                          RecoveryData
                          );
  if (!EFI_ERROR(Status) && (RecoveryDataSize > 1)) {
    switch (RecoveryData[1]) {
    case 2:
      goto step2;
    default:
      goto step3;
    }
  }

  //
  // Insert variables until no enough storage
  //
  for (Index = 0; Index < 10000; Index++) {
    //
    // Insert a variable
    //
    VariableName = SctPoolPrint (L"TestVariable_%d", Index);

    for (DataIndex = 0; DataIndex < 10; DataIndex++) {
      Data[DataIndex] = (UINT8)DataIndex;
    }

    Status = RT->SetVariable (
                   VariableName,                // VariableName
                   &gTestVendor1Guid,           // VendorGuid
                   EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                   10,                          // DataSize
                   Data                         // Data
                   );

    gtBS->FreePool (VariableName);

    if (EFI_ERROR(Status)) {
      break;
    }
  }

  //
  // Write reset record
  //
  RecoveryData[0] = 2;
  RecoveryData[1] = 2;
  RecoveryLib->WriteResetRecord (RecoveryLib, 2, RecoveryData);

  //
  // Print out some information to avoid the user thought it is an error
  //
  SctPrint (L"System will cold reset after 1 second...");
  gtBS->Stall (1000000);

  gtRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

  //
  // After system reset
  //
step2:
  RecoveryLib->WriteResetRecord (RecoveryLib, 0, NULL);

  //
  // Delete variables until all added have been deleted
  //
  for (Index = 0; Index < 10000; Index++) {
    //
    // Delete a variable
    //
    VariableName = SctPoolPrint (L"TestVariable_%d", Index);

    Status = RT->SetVariable (
                   VariableName,                // VariableName
                   &gTestVendor1Guid,           // VendorGuid
                   EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                   0,                           // DataSize
                   Data                         // Data
                   );

    gtBS->FreePool (VariableName);

    if (EFI_ERROR(Status)) {
      break;
    }
  }

  //
  // Record assertion (If system can restart, test passed)
  //
  StandardLib->RecordAssertion (
                 StandardLib,
                 EFI_TEST_ASSERTION_PASSED,
                 gVariableServicesBbTestStressAssertionGuid006,
                 L"RT.SetVariable - Reclaim after system reset",
                 L"%a:%d:Number - %d",
                 __FILE__,
                 (UINTN)__LINE__,
                 Index
                 );

  //
  // Free resources
  //
step3:
  gtBS->FreePool (RecoveryData);

  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"OverflowStressTestSub2",
                  L"TDS 5.2.2.2"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}
