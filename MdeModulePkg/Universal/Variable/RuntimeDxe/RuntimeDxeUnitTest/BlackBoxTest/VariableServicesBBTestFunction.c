/** @file

  Copyright 2006 - 2012 Unified EFI, Inc.<BR>
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
  VariableServicesBbTestFunction.c

Abstract:
  Source file for Variable Services Function Black-Box Test - Function Test.

--*/

#include "VariableServicesBBTestMain.h"

#include <Guid/GlobalVariable.h>

//
// Prototypes (external)
//

EFI_STATUS
GetVariableFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

EFI_STATUS
GetNextVariableNameFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

EFI_STATUS
SetVariableFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

//
// Prototypes (internal)
//

EFI_STATUS
GetVariableFuncTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
GetNextVariableNameFuncTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
SetVariableFuncTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
SetVariableFuncTestSub2 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
SetVariableFuncTestSub3 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
SetVariableFuncTestSub4 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
SetVariableFuncTestSub5 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
SetVariableFuncTestSub6 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_RECOVERY_LIBRARY_PROTOCOL   *RecoveryLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

EFI_STATUS
SetVariableFuncTestSub7 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_RECOVERY_LIBRARY_PROTOCOL   *RecoveryLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  );

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
EFI_STATUS
QueryVariableInfoFuncTest (
  IN EFI_BB_TEST_PROTOCOL       *This,
  IN VOID                       *ClientInterface,
  IN EFI_TEST_LEVEL             TestLevel,
  IN EFI_HANDLE                 SupportHandle
  );

//
// QueryVariableInfo must succeed with a valid Attributes and 
// return the proper value when it is in Boot time
//
EFI_STATUS
QueryVariableInfoFuncTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib,
  IN EFI_HANDLE                           SupportHandle
  );

#endif

// #if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
// EFI_GUID gGlobalVariableGuid = EFI_GLOBAL_VARIABLE;

// EFI_GUID gHwErrRecGuid = EFI_HARDWARE_ERROR_VARIABLE;
// #endif
//
// Functions
//

/**
 *  TDS 4.1 - Entry point for RT->GetVariable() Function Test.
 *  @param This             A pointer to the EFI_BB_TEST_PROTOCOL instance.
 *  @param ClientInterface  A pointer to the interface to be tested.
 *  @param TestLevel        Test "thoroughness" control.
 *  @param SupportHandle    A handle containing support protocols.
 *  @return EFI_SUCCESS     Successfully.
 *  @return Other value     Something failed.
 */
EFI_STATUS
GetVariableFuncTest (
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
  // GetVariable when the variable exists
  //
  Status = GetVariableFuncTestSub1 (RT, StandardLib, LoggingLib);

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  TDS 4.2 - Entry point for RT->GetNextVariableName() Function Test.
 *  @param This             A pointer to the EFI_BB_TEST_PROTOCOL instance.
 *  @param ClientInterface  A pointer to the interface to be tested.
 *  @param TestLevel        Test "thoroughness" control.
 *  @param SupportHandle    A handle containing support protocols.
 *  @return EFI_SUCCESS     Successfully.
 *  @return Other value     Something failed.
 */
EFI_STATUS
GetNextVariableNameFuncTest (
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
  // GetNextVariableName when the next variable exists
  //
  Status = GetNextVariableNameFuncTestSub1 (RT, StandardLib, LoggingLib);

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  TDS 4.3 - Entry point for RT->SetVariable() Function Test.
 *  @param This             A pointer to the EFI_BB_TEST_PROTOCOL instance.
 *  @param ClientInterface  A pointer to the interface to be tested.
 *  @param TestLevel        Test "thoroughness" control.
 *  @param SupportHandle    A handle containing support protocols.
 *  @return EFI_SUCCESS     Successfully.
 *  @return Other value     Something failed.
 */
EFI_STATUS
SetVariableFuncTest (
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
  // UINTN                               RecoveryDataSize;
  // UINT8                               *RecoveryData;

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
  // Status = gtBS->AllocatePool (
  //                  EfiLoaderData,
  //                  1024,
  //                  (VOID **)&RecoveryData
  //                  );
  // if (EFI_ERROR(Status)) {
  //   return Status;
  // }

  //
  // Read reset record
  //
  // RecoveryDataSize = 1024;
  // Status = RecoveryLib->ReadResetRecord (
  //                         RecoveryLib,
  //                         &RecoveryDataSize,
  //                         RecoveryData
  //                         );
  // if (!EFI_ERROR(Status) && (RecoveryDataSize > 0)) {
  //   switch (RecoveryData[0]) {
  //   case 6:
  //     goto step6;
  //   case 7:
  //     goto step7;
  //   default:
  //     goto step8;
  //   }
  // }

  //
  // SetVariable when the variable does not exist
  //
  Status = SetVariableFuncTestSub1 (RT, StandardLib, LoggingLib);

  //
  // SetVariable when the variable exists
  //
  Status = SetVariableFuncTestSub2 (RT, StandardLib, LoggingLib);

  //
  // SetVariable when the similar variable exists
  //
  Status = SetVariableFuncTestSub3 (RT, StandardLib, LoggingLib);

  //
  // SetVariable when DataSize is 0
  //
  Status = SetVariableFuncTestSub4 (RT, StandardLib, LoggingLib);

  //
  // SetVariable when the Attributes are 0
  //
  Status = SetVariableFuncTestSub5 (RT, StandardLib, LoggingLib);

  //
  // Non-volatile variable exists after system reset
  //
  // step6:
  //   Status = SetVariableFuncTestSub6 (RT, StandardLib, RecoveryLib, LoggingLib);

  //
  // Volatile variable does not exist after system reset
  //
  // step7:
  //   Status = SetVariableFuncTestSub7 (RT, StandardLib, RecoveryLib, LoggingLib);

  //
  // Free resources
  //
  // step8:
  //   gtBS->FreePool (RecoveryData);

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  GetVariable when the variable exists.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetVariableFuncTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 SubIndex;
  UINTN                 DataIndex;
  UINTN                 DataSize;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINT32                Attributes;
  EFI_TPL               OldTpl;
  EFI_TPL               TplArray[] = {TPL_APPLICATION, TPL_CALLBACK};
  UINT32                AttributesArray[] = {
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
                  L"GetVariableFuncTestSub1",
                  L"TDS 4.1.2.1"
                  );
  }

  //
  // For each TPL less than or equal to TPL_CALLBACK do
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

        continue;
      }

      //
      // Get the variable without attributes
      //
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     NULL,                        // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );

      gtBS->RestoreTPL (OldTpl);

      //
      // Check results
      //
      if ((Status   == EFI_SUCCESS) &&
          (DataSize == 10         )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 10; DataIndex++) {
          if (Data[DataIndex] != (UINT8)DataIndex) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid001 :
                     gVariableServicesBbTestFunctionAssertionGuid002,
                     L"RT.GetVariable - Existent variable without attributes",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     Status,      EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     DataSize,    10,
                     Data[0],     0
                     );

      //
      // Get the variable with attributes
      //
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );

      gtBS->RestoreTPL (OldTpl);

      //
      // Check results
      //
      if ((Status     == EFI_SUCCESS              ) &&
          (Attributes == AttributesArray[SubIndex]) &&
          (DataSize   == 10                       )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 10; DataIndex++) {
          if (Data[DataIndex] != (UINT8)DataIndex) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid003 :
                     gVariableServicesBbTestFunctionAssertionGuid004,
                     L"RT.GetVariable - Existent variable with attributes",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     Status,      EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"Attributes=0x%x, Expected=0x%x\n"
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     Attributes,  AttributesArray[SubIndex],
                     DataSize,    10,
                     Data[0],     0
                     );

      //
      // Delete the variable (restore environment)
      //
      Status = RT->SetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetVariableFuncTestSub1",
                  L"TDS 4.1.2.1"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  GetNextVariableName when the next variable exists.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
GetNextVariableNameFuncTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 SubIndex;
  UINTN                 DataIndex;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINTN                 VariableNameSize;
  CHAR16                VariableName[MAX_BUFFER_SIZE];
  EFI_GUID              VendorGuid;
  EFI_TPL               OldTpl;
  EFI_TPL               TplArray[] = {TPL_APPLICATION, TPL_CALLBACK};
  UINT32                AttributesArray[] = {
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
                  L"GetNextVariableNameFuncTestSub1",
                  L"TDS 4.2.2.1"
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
      if (EFI_ERROR(Status)) {
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

        continue;
      }

      //
      // Walk through the entire variable list
      //
      VariableName[0] = L'\0';
      Result = EFI_TEST_ASSERTION_FAILED;

      while (TRUE) {
        OldTpl = gtBS->RaiseTPL (TplArray[Index]);

        VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
        Status = RT->GetNextVariableName (
                       &VariableNameSize,         // VariableNameSize
                       VariableName,              // VariableName
                       &VendorGuid                // VendorGuid
                       );

        gtBS->RestoreTPL (OldTpl);

        if (EFI_ERROR(Status)) {
          break;
        }

        if ((SctStrCmp (VariableName, L"TestVariable")       == 0) &&
            (SctCompareGuid (&VendorGuid, &gTestVendor1Guid) == 0)) {
          Result = EFI_TEST_ASSERTION_PASSED;
          break;
        }
      }

      //
      // Record assertion
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid005 :
                     gVariableServicesBbTestFunctionAssertionGuid006,
                     L"RT.GetNextVariableName - Existent variable",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     Status,
                     EFI_SUCCESS
                     );

      //
      // Delete the variable (restore environment)
      //
      Status = RT->SetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"GetNextVariableNameFuncTestSub1",
                  L"TDS 4.2.2.1"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  SetVariable when the variable does not exist.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
SetVariableFuncTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            ReturnedStatus;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 SubIndex;
  UINTN                 DataIndex;
  UINTN                 DataSize;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINTN                 DataSize2;
  UINT8                 Data2[MAX_BUFFER_SIZE];
  UINT32                Attributes;
  EFI_TPL               OldTpl;
  EFI_TPL               TplArray[] = {TPL_APPLICATION, TPL_CALLBACK};
  UINT32                AttributesArray[] = {
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
                  L"SetVariableFuncTestSub1",
                  L"TDS 4.3.2.1"
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
      // Insert a test variable with GUID2
      //
      for (DataIndex = 0; DataIndex < 20; DataIndex++) {
        Data2[DataIndex] = (UINT8)(DataIndex + 1);
      }

      Status = RT->SetVariable (
                     L"TestVariable",           // VariableName
                     &gTestVendor2Guid,         // VendorGuid
                     AttributesArray[SubIndex], // Attributes
                     20,                        // DataSize
                     Data2                      // Data
                     );
      if (EFI_ERROR(Status)) {
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
      // Insert a variable
      //
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = (UINT8)DataIndex;
      }

      ReturnedStatus = RT->SetVariable (
                             L"TestVariable",             // VariableName
                             &gTestVendor1Guid,           // VendorGuid
                             AttributesArray[SubIndex],   // Attributes
                             10,                          // DataSize
                             Data                         // Data
                             );

      gtBS->RestoreTPL (OldTpl);

      //
      // Get the variable to check results
      //
      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR(Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot get the variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      //
      // Get the variable with GUID2 to check results
      //
      DataSize2 = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor2Guid,           // VendorGuid
                     NULL,                        // Attributes
                     &DataSize2,                  // DataSize
                     Data2                        // Data
                     );
      if (EFI_ERROR(Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot get the variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      //
      // Check the results
      //
      if ((ReturnedStatus == EFI_SUCCESS              ) &&
          (Attributes     == AttributesArray[SubIndex]) &&
          (DataSize       == 10                       ) &&
          (DataSize2      == 20                       )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 10; DataIndex++) {
          if (Data[DataIndex] != (UINT8)DataIndex) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }

        for (DataIndex = 0; DataIndex < 20; DataIndex++) {
          if (Data2[DataIndex] != (UINT8)(DataIndex + 1)) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid007 :
                     gVariableServicesBbTestFunctionAssertionGuid008,
                     L"RT.SetVariable - Nonexistent variable",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     ReturnedStatus,    EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"Attributes=0x%x, Expected=0x%x\n"
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     Attributes,        AttributesArray[SubIndex],
                     DataSize,          10,
                     Data[0],           0
                     );

      //
      // Delete the variable (restore environment)
      //
      Status = RT->SetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );
                     
      Status = RT->SetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor2Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );
                     
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableFuncTestSub1",
                  L"TDS 4.3.2.1"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  SetVariable when the variable exists.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
SetVariableFuncTestSub2 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            ReturnedStatus;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 SubIndex;
  UINTN                 DataIndex;
  UINTN                 DataSize;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINT32                Attributes;
  EFI_TPL               OldTpl;
  EFI_TPL               TplArray[] = {TPL_APPLICATION, TPL_CALLBACK};
  UINT32                AttributesArray[] = {
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
                  L"SetVariableFuncTestSub2",
                  L"TDS 4.3.2.2"
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
      // Insert a variable firstly
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
      if (EFI_ERROR(Status)) {
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
      // Insert a variable with the same data
      //
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = (UINT8)DataIndex;
      }

      ReturnedStatus = RT->SetVariable (
                             L"TestVariable",             // VariableName
                             &gTestVendor1Guid,           // VendorGuid
                             AttributesArray[SubIndex],   // Attributes
                             10,                          // DataSize
                             Data                         // Data
                             );

      gtBS->RestoreTPL (OldTpl);

      //
      // Get the variable to check results
      //
      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR(Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot get the variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      //
      // Check the results
      //
      if ((ReturnedStatus == EFI_SUCCESS              ) &&
          (Attributes     == AttributesArray[SubIndex]) &&
          (DataSize       == 10                       )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 10; DataIndex++) {
          if (Data[DataIndex] != (UINT8)DataIndex) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid009 :
                     gVariableServicesBbTestFunctionAssertionGuid010,
                     L"RT.SetVariable - Existent variable and same data",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     ReturnedStatus,    EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"Attributes=0x%x, Expected=0x%x\n"
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     Attributes,        AttributesArray[SubIndex],
                     DataSize,          10,
                     Data[0],           0
                     );

      //
      // Insert the variable with different data
      //
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      for (DataIndex = 0; DataIndex < 20; DataIndex++) {
        Data[DataIndex] = (UINT8)(DataIndex + 1);
      }

      ReturnedStatus = RT->SetVariable (
                             L"TestVariable",             // VariableName
                             &gTestVendor1Guid,           // VendorGuid
                             AttributesArray[SubIndex],   // Attributes
                             20,                          // DataSize
                             Data                         // Data
                             );

      gtBS->RestoreTPL (OldTpl);

      //
      // Get the variable to check results
      //
      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR(Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot get the variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      //
      // Check the results
      //
      if ((ReturnedStatus == EFI_SUCCESS              ) &&
          (Attributes     == AttributesArray[SubIndex]) &&
          (DataSize       == 20                       )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 20; DataIndex++) {
          if (Data[DataIndex] != (UINT8)(DataIndex + 1)) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid011 :
                     gVariableServicesBbTestFunctionAssertionGuid012,
                     L"RT.SetVariable - Existent variable and different data",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     ReturnedStatus,    EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"Attributes=0x%x, Expected=0x%x\n"
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     Attributes,        AttributesArray[SubIndex],
                     DataSize,          20,
                     Data[0],           1
                     );

      //
      // Insert the variable with different data
      //
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      for (DataIndex = 0; DataIndex < 5; DataIndex++) {
        Data[DataIndex] = (UINT8)DataIndex;
      }

      ReturnedStatus = RT->SetVariable (
                             L"TestVariable",             // VariableName
                             &gTestVendor1Guid,           // VendorGuid
                             AttributesArray[SubIndex],   // Attributes
                             5,                           // DataSize
                             Data                         // Data
                             );

      gtBS->RestoreTPL (OldTpl);

      //
      // Get the variable to check results
      //
      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR(Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot get the variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      //
      // Check the results
      //
      if ((ReturnedStatus == EFI_SUCCESS              ) &&
          (Attributes     == AttributesArray[SubIndex]) &&
          (DataSize       == 5                        )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 5; DataIndex++) {
          if (Data[DataIndex] != (UINT8)DataIndex) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid013 :
                     gVariableServicesBbTestFunctionAssertionGuid014,
                     L"RT.SetVariable - Existent variable and different data",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     ReturnedStatus,    EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"Attributes=0x%x, Expected=0x%x\n"
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     Attributes,        AttributesArray[SubIndex],
                     DataSize,          5,
                     Data[0],           0
                     );

      //
      // Delete the variable (restore environment)
      //
      Status = RT->SetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableFuncTestSub2",
                  L"TDS 4.3.2.2"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  SetVariable when the similar variable exists.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
SetVariableFuncTestSub3 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            ReturnedStatus;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 SubIndex;
  UINTN                 DataIndex;
  UINTN                 DataSize;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINT32                Attributes;
  EFI_TPL               OldTpl;
  EFI_TPL               TplArray[] = {TPL_APPLICATION, TPL_CALLBACK};
  UINT32                AttributesArray[] = {
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
                  L"SetVariableFuncTestSub3",
                  L"TDS 4.3.2.3"
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
      // Insert similar variable firstly ("TestVariable" + A)
      //
      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = (UINT8)(DataIndex + 1);
      }

      Status = RT->SetVariable (
                     L"TestVariableA",            // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     10,                          // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR(Status)) {
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
      // Insert similar variable firstly ("TestVariable" - e)
      //
      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = (UINT8)(DataIndex + 2);
      }

      Status = RT->SetVariable (
                     L"TestVariabl",              // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     10,                          // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR(Status)) {
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
      // Insert the test variable
      //
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = (UINT8)DataIndex;
      }

      ReturnedStatus = RT->SetVariable (
                             L"TestVariable",             // VariableName
                             &gTestVendor1Guid,           // VendorGuid
                             AttributesArray[SubIndex],   // Attributes
                             10,                          // DataSize
                             Data                         // Data
                             );

      gtBS->RestoreTPL (OldTpl);

      //
      // Get the variable to check results
      //
      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR(Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot get the variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      //
      // Check results
      //
      if ((ReturnedStatus == EFI_SUCCESS              ) &&
          (Attributes     == AttributesArray[SubIndex]) &&
          (DataSize       == 10                       )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 10; DataIndex++) {
          if (Data[DataIndex] != (UINT8)DataIndex) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid015 :
                     gVariableServicesBbTestFunctionAssertionGuid016,
                     L"RT.SetVariable - Existent similar variable",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     ReturnedStatus,    EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"Attributes=0x%x, Expected=0x%x\n"
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     Attributes,        AttributesArray[SubIndex],
                     DataSize,          10,
                     Data[0],           0
                     );

      //
      // Get the similar variable to check results ("TestVariable" + A)
      //
      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariableA",            // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR(Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot get the variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      //
      // Check results
      //
      if ((Attributes == AttributesArray[SubIndex]) &&
          (DataSize   == 10                       )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 10; DataIndex++) {
          if (Data[DataIndex] != (UINT8)(DataIndex + 1)) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid017 :
                     gVariableServicesBbTestFunctionAssertionGuid018,
                     L"RT.SetVariable - Existent similar variable (+A)",
                     L"%a:%d",
                     __FILE__,
                     (UINTN)__LINE__
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"Attributes=0x%x, Expected=0x%x\n"
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     Attributes,  AttributesArray[SubIndex],
                     DataSize,    10,
                     Data[0],     1
                     );

      //
      // Get the similar variable to check results ("TestVariable" - e)
      //
      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariabl",              // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );
      if (EFI_ERROR(Status)) {
        StandardLib->RecordAssertion (
                       StandardLib,
                       EFI_TEST_ASSERTION_WARNING,
                       gTestGenericFailureGuid,
                       L"RT.SetVariable - Cannot get the variable",
                       L"%a:%d:Status - %r",
                       __FILE__,
                       (UINTN)__LINE__,
                       Status
                       );

        continue;
      }

      //
      // Check results
      //
      if ((Attributes == AttributesArray[SubIndex]) &&
          (DataSize   == 10                       )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 10; DataIndex++) {
          if (Data[DataIndex] != (UINT8)(DataIndex + 2)) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid019 :
                     gVariableServicesBbTestFunctionAssertionGuid020,
                     L"RT.SetVariable - Existent similar variable (-A)",
                     L"%a:%d:",
                     __FILE__,
                     (UINTN)__LINE__
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"Attributes=0x%x, Expected=0x%x\n"
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     Attributes,  AttributesArray[SubIndex],
                     DataSize,    10,
                     Data[0],     2
                     );

      //
      // Delete similar and test variables (restore environment)
      //
      Status = RT->SetVariable (
                     L"TestVariableA",            // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );

      Status = RT->SetVariable (
                     L"TestVariabl",              // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );

      Status = RT->SetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableFuncTestSub3",
                  L"TDS 4.3.2.3"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  SetVariable when the DataSize is 0.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
SetVariableFuncTestSub4 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            ReturnedStatus;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 SubIndex;
  UINTN                 DataIndex;
  UINTN                 DataSize;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINT32                Attributes;
  EFI_TPL               OldTpl;
  EFI_TPL               TplArray[] = {TPL_APPLICATION, TPL_CALLBACK};
  UINT32                AttributesArray[] = {
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
                  L"SetVariableFuncTestSub4",
                  L"TDS 4.3.2.4"
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
      if (EFI_ERROR(Status)) {
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
      // Get the variable to check results
      //
      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );

      //
      // Check results
      //
      if ((ReturnedStatus == EFI_SUCCESS  ) &&
          (Status         == EFI_NOT_FOUND)) {
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
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid021 :
                     gVariableServicesBbTestFunctionAssertionGuid022,
                     L"RT.SetVariable - With DataSize is 0",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     ReturnedStatus,    EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"GetVariable: Status - %r, Expected - %r\n",
                     Status,            EFI_NOT_FOUND
                     );
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableFuncTestSub4",
                  L"TDS 4.3.2.4"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  SetVariable when the Attributes is 0.
 *  @param StandardLib    A pointer to EFI_STANDARD_TEST_LIBRARY_PROTOCOL
 *                        instance.
 *  @param LoggingLib     A pointer to EFI_TEST_LOGGING_LIBRARY_PROTOCOL
 *                        instance.
 *  @return EFI_SUCCESS   Successfully.
 *  @return Other value   Something failed.
 */
EFI_STATUS
SetVariableFuncTestSub5 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            ReturnedStatus;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 SubIndex;
  UINTN                 DataIndex;
  UINTN                 DataSize;
  UINT8                 Data[MAX_BUFFER_SIZE];
  UINT32                Attributes;
  EFI_TPL               OldTpl;
  EFI_TPL               TplArray[] = {TPL_APPLICATION, TPL_CALLBACK};
  UINT32                AttributesArray[] = {
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
                  L"SetVariableFuncTestSub5",
                  L"TDS 4.3.2.5"
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
      if (EFI_ERROR(Status)) {
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
      // Delete the variable with Attributes is 0
      //
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      ReturnedStatus = RT->SetVariable (
                             L"TestVariable",             // VariableName
                             &gTestVendor1Guid,           // VendorGuid
                             0,                           // Attributes
                             10,                          // DataSize
                             Data                         // Data
                             );

      gtBS->RestoreTPL (OldTpl);


      //
      // Get the variable to check results
      //
      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     L"TestVariable",             // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );

      //
      // Check results
      //
      if ((ReturnedStatus == EFI_SUCCESS  ) &&
          (Status         == EFI_NOT_FOUND)) {
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
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid023 :
                     gVariableServicesBbTestFunctionAssertionGuid024,
                     L"RT.SetVariable - With Attributes is 0",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     ReturnedStatus,    EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"GetVariable: Status - %r, Expected - %r\n",
                     Status,            EFI_NOT_FOUND
                     );
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableFuncTestSub5",
                  L"TDS 4.3.2.5"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  Non-volatile variable exists after system reset.
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
SetVariableFuncTestSub6 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_RECOVERY_LIBRARY_PROTOCOL   *RecoveryLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 SubIndex;
  UINTN                 DataIndex;
  UINTN                 DataSize;
  UINT8                 Data[MAX_BUFFER_SIZE];
  CHAR16                *VariableName;
  UINT32                Attributes;
  // UINTN                 RecoveryDataSize;
  // UINT8                 *RecoveryData;
  EFI_TPL               OldTpl;
  EFI_TPL               TplArray[] = {TPL_APPLICATION, TPL_CALLBACK};
  UINT32                AttributesArray[] = {
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
  };

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"SetVariableFuncTestSub6",
                  L"TDS 4.3.2.6"
                  );
  }

  //
  // Allocate memory for recovery data
  //
  // Status = gtBS->AllocatePool (
  //                  EfiLoaderData,
  //                  1024,
  //                  (VOID **)&RecoveryData
  //                  );
  // if (EFI_ERROR(Status)) {
  //   if (LoggingLib != NULL) {
  //     LoggingLib->ExitFunction (
  //                   LoggingLib,
  //                   L"SetVariableFuncTestSub6",
  //                   L"TDS 4.3.2.6 - Cannot allocate memory for recovery data"
  //                   );
  //   }

  //   return Status;
  // }

  //
  // Read reset record
  //
  // RecoveryDataSize = 1024;
  // Status = RecoveryLib->ReadResetRecord (
  //                         RecoveryLib,
  //                         &RecoveryDataSize,
  //                         RecoveryData
  //                         );
  // if (!EFI_ERROR(Status) && (RecoveryDataSize > 1)) {
  //   if (RecoveryData[0] == 6) {
  //     switch (RecoveryData[1]) {
  //     case 2:
  //       goto step2;
  //     default:
  //       goto step3;
  //     }
  //   }
  // }

  //
  // for each TPL less than or equal to TPL_CALLBACK do
  //
  for (Index = 0; Index < 2; Index++) {
    //
    // For each Attributes of NV|BA and NV|BA|RA do
    //
    for (SubIndex = 0; SubIndex < 2; SubIndex++) {
      //
      // Insert a variable
      //
      VariableName = SctPoolPrint (L"TestVariable_%d_%d", Index, SubIndex);
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = (UINT8)DataIndex;
      }

      Status = RT->SetVariable (
                     VariableName,                // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     10,                          // DataSize
                     Data                         // Data
                     );

      gtBS->RestoreTPL (OldTpl);
      gtBS->FreePool (VariableName);

      //
      // Check results
      //
      if (EFI_ERROR(Status)) {
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
      }
    }
  }

  //
  // Write reset record
  //
  // RecoveryData[0] = 6;
  // RecoveryData[1] = 2;
  // RecoveryLib->WriteResetRecord (RecoveryLib, 2, RecoveryData);

  //
  // Print out some information to avoid the user thought it is an error
  //
  // SctPrint (L"System will cold reset after 1 second...");
  // gtBS->Stall (1000000);

  // gtRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

  //
  // After system reset
  //
  // step2:
  //   RecoveryLib->WriteResetRecord (RecoveryLib, 0, NULL);

  //
  // for each TPL less than or equal to TPL_CALLBACK do
  //
  for (Index = 0; Index < 2; Index++) {
    //
    // For each Attributes of NV|BA and NV|BA|RA do
    //
    for (SubIndex = 0; SubIndex < 2; SubIndex++) {
      //
      // Get the variable
      //
      VariableName = SctPoolPrint (L"TestVariable_%d_%d", Index, SubIndex);

      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     VariableName,                // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );

      //
      // Check results
      //
      if ((Status     == EFI_SUCCESS              ) &&
          (Attributes == AttributesArray[SubIndex]) &&
          (DataSize   == 10                       )) {
        Result = EFI_TEST_ASSERTION_PASSED;

        for (DataIndex = 0; DataIndex < 10; DataIndex++) {
          if (Data[DataIndex] != (UINT8)DataIndex) {
            Result = EFI_TEST_ASSERTION_FAILED;
            break;
          }
        }
      } else {
        Result = EFI_TEST_ASSERTION_FAILED;
      }

      //
      // Record assertion & message
      //
      StandardLib->RecordAssertion (
                     StandardLib,
                     Result,
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid025 :
                     gVariableServicesBbTestFunctionAssertionGuid026,
                     L"RT.SetVariable - Non-volatile variable after system reset",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     Status,      EFI_SUCCESS
                     );

      StandardLib->RecordMessage (
                     StandardLib,
                     EFI_VERBOSE_LEVEL_DEFAULT,
                     L"Attributes=0x%x, Expected=0x%x\n"
                     L"DataSize=%d, Expected=%d\n"
                     L"Data[0]=0x%x, Expected=0x%x\n",
                     Attributes,  AttributesArray[SubIndex],
                     DataSize,    10,
                     Data[0],     0
                     );

      //
      // Delete the variable (restore environment)
      //
      Status = RT->SetVariable (
                     VariableName,                // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     0,                           // DataSize
                     Data                         // Data
                     );

      gtBS->FreePool (VariableName);
    }
  }

  //
  // Free resources
  //
  // step3:
  //   gtBS->FreePool (RecoveryData);

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableFuncTestSub6",
                  L"TDS 4.3.2.6"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
 *  Volatile variable does not exist after system reset.
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
SetVariableFuncTestSub7 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_RECOVERY_LIBRARY_PROTOCOL   *RecoveryLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib
  )
{
  EFI_STATUS            Status;
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINTN                 SubIndex;
  UINTN                 DataIndex;
  UINTN                 DataSize;
  UINT8                 Data[MAX_BUFFER_SIZE];
  CHAR16                *VariableName;
  UINT32                Attributes;
  // UINTN                 RecoveryDataSize;
  // UINT8                 *RecoveryData;
  EFI_TPL               OldTpl;
  EFI_TPL               TplArray[] = {TPL_APPLICATION, TPL_CALLBACK};
  UINT32                AttributesArray[] = {
    EFI_VARIABLE_BOOTSERVICE_ACCESS,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
  };

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"SetVariableFuncTestSub7",
                  L"TDS 4.3.2.7"
                  );
  }

  //
  // Allocate memory for recovery data
  //
  // Status = gtBS->AllocatePool (
  //                  EfiLoaderData,
  //                  1024,
  //                  (VOID **)&RecoveryData
  //                  );
  // if (EFI_ERROR(Status)) {
  //   if (LoggingLib != NULL) {
  //     LoggingLib->ExitFunction (
  //                   LoggingLib,
  //                   L"SetVariableFuncTestSub7",
  //                   L"TDS 4.3.2.7 - Cannot allocate memory for recovery data"
  //                   );
  //   }

  //   return Status;
  // }

  //
  // Read reset record
  //
  // RecoveryDataSize = 1024;
  // Status = RecoveryLib->ReadResetRecord (
  //                         RecoveryLib,
  //                         &RecoveryDataSize,
  //                         RecoveryData
  //                         );
  // if (!EFI_ERROR(Status) && (RecoveryDataSize > 1)) {
  //   if (RecoveryData[0] == 7) {
  //     switch (RecoveryData[1]) {
  //     case 2:
  //       goto step2;
  //     default:
  //       goto step3;
  //     }
  //   }
  // }

  //
  // for each TPL less than or equal to TPL_CALLBACK do
  //
  for (Index = 0; Index < 2; Index++) {
    //
    // For each Attributes of NV|BA and NV|BA|RA do
    //
    for (SubIndex = 0; SubIndex < 2; SubIndex++) {
      //
      // Insert a variable
      //
      VariableName = SctPoolPrint (L"TestVariable_%d_%d", Index, SubIndex);
      OldTpl = gtBS->RaiseTPL (TplArray[Index]);

      for (DataIndex = 0; DataIndex < 10; DataIndex++) {
        Data[DataIndex] = (UINT8)DataIndex;
      }

      Status = RT->SetVariable (
                     VariableName,                // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     AttributesArray[SubIndex],   // Attributes
                     10,                          // DataSize
                     Data                         // Data
                     );

      gtBS->RestoreTPL (OldTpl);
      gtBS->FreePool (VariableName);

      //
      // Check results
      //
      if (EFI_ERROR(Status)) {
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
      }
    }
  }

  //
  // Write reset record
  //
  // RecoveryData[0] = 7;
  // RecoveryData[1] = 2;
  // RecoveryLib->WriteResetRecord (RecoveryLib, 2, RecoveryData);

  //
  // Print out some information to avoid the user thought it is an error
  //
  // SctPrint (L"System will cold reset after 1 second...");
  // gtBS->Stall (1000000);

  // gtRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

  //
  // After system reset
  //
  // step2:
  //
  // for each TPL less than or equal to TPL_CALLBACK do
  //
  for (Index = 0; Index < 2; Index++) {
    //
    // For each Attributes of NV|BA and NV|BA|RA do
    //
    for (SubIndex = 0; SubIndex < 2; SubIndex++) {
      //
      // Get the variable
      //
      VariableName = SctPoolPrint (L"TestVariable_%d_%d", Index, SubIndex);

      DataSize = MAX_BUFFER_SIZE;
      Status = RT->GetVariable (
                     VariableName,                // VariableName
                     &gTestVendor1Guid,           // VendorGuid
                     &Attributes,                 // Attributes
                     &DataSize,                   // DataSize
                     Data                         // Data
                     );

      //
      // Check results
      //
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
                     (Index == 0) ? gVariableServicesBbTestFunctionAssertionGuid027 :
                     gVariableServicesBbTestFunctionAssertionGuid028,
                     L"RT.SetVariable - Volatile variable after system reset",
                     L"%a:%d:Status - %r, Expected - %r",
                     __FILE__,
                     (UINTN)__LINE__,
                     Status,      EFI_NOT_FOUND
                     );

      gtBS->FreePool (VariableName);
    }
  }

  //
  // Free resources
  //
  // step3:
  //   gtBS->FreePool (RecoveryData);

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"SetVariableFuncTestSub7",
                  L"TDS 4.3.2.7"
                  );
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
//
// QueryVariableInfoFuncTest
//
UINT32
UnicodeStrLength (
  IN CHAR16 *String
  )
{
  UINT32  Count;

  if (NULL == String) {
    return 0;
  }

  Count = 0;

  while (0 != String[Count]) {
    Count++;
  }

  return (Count * 2) + 2;
}

EFI_STATUS
QueryVariableInfoFuncTest (
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

  // if (FALSE == CheckBBTestCanRunAndRecordAssertion(
  //                 StandardLib, 
  //                 L"RT.QueryVariableInfo_Func - QueryVariableInfo_Func it's not Supported in EFI",
  //                 __FILE__,
  //                 (UINTN)__LINE__
  //                 )) {
  //   return EFI_SUCCESS;
  // }

  RT = (EFI_RUNTIME_SERVICES *)ClientInterface;

  Status = QueryVariableInfoFuncTestSub1 (RT, StandardLib, LoggingLib, SupportHandle);

  return EFI_SUCCESS;
}

//
// QueryVariableInfo must succeed with a valid Attributes and 
// return the proper value when it is in Boot time
//
EFI_STATUS
QueryVariableInfoFuncTestSub1 (
  IN EFI_RUNTIME_SERVICES                 *RT,
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL   *StandardLib,
  IN EFI_TEST_LOGGING_LIBRARY_PROTOCOL    *LoggingLib,
  IN EFI_HANDLE                           SupportHandle
  )
{
  EFI_STATUS            QueryVarStatus[3];
  // EFI_STATUS          SetVarStatus;
  UINT32                ValidAttributes[] = {
                          EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS,
                          EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
                          EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
                          EFI_VARIABLE_BOOTSERVICE_ACCESS,
                          0
                        };
  UINT64                MaximumVariableStorageSize[3];
  UINT64                RemainingVariableStorageSize[3];
  UINT64                MaximumVariableSize[3];
  EFI_TEST_ASSERTION    Result;
  UINTN                 Index;
  UINT8                 DataBuf[MAX_DATA_SIZE];
  UINT32                DataIndex;
  UINT32                SubIndex;
  EFI_GUID              TestVendorGuid = QUERYVAR_GUID1;
  UINT32                ErrorCode;
  
  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"QueryVariableInfoFuncTestSub1",
                  L"TDS 4.4.2.1"
                  );
  }
   
  for (Index = 0; ValidAttributes[Index] != 0; Index ++) {
    //
    // QueryVariableInfo 1
    //
    QueryVarStatus[0] = RT->QueryVariableInfo (
                              ValidAttributes[Index],
                              &MaximumVariableStorageSize[0],
                              &RemainingVariableStorageSize[0],
                              &MaximumVariableSize[0]
                              );

    //
    // Call SetVariable to insert a variable
    //
    for (DataIndex = 0; DataIndex < MAX_DATA_SIZE; DataIndex++) {
        DataBuf[DataIndex] = (UINT8)(DataIndex + 1);
    }

    RT->SetVariable (
          L"TestVariable",           // VariableName
          &TestVendorGuid,           // VendorGuid
          ValidAttributes[Index],    // Attributes
          MAX_DATA_SIZE,             // DataSize
          DataBuf                    // Data
          );

    //
    // QueryVariableInfo 2
    //
    QueryVarStatus[1] = RT->QueryVariableInfo (
                              ValidAttributes[Index],
                              &MaximumVariableStorageSize[1],
                              &RemainingVariableStorageSize[1],
                              &MaximumVariableSize[1]
                              );
               
    //
    // Call SetVariable service to delete the variable inserted just now
    //
    RT->SetVariable (
          L"TestVariable",            // VariableName
          &TestVendorGuid,            // VendorGuid
          ValidAttributes[Index],
          0,                    // DataSize
          DataBuf               // Data
          );

    //
    // QueryVariableInfo 3
    // 
    QueryVarStatus[2] = RT->QueryVariableInfo (
                              ValidAttributes[Index],
                              &MaximumVariableStorageSize[2],
                              &RemainingVariableStorageSize[2],
                              &MaximumVariableSize[2]
                              );

    // QueryVarStatus[0] QueryVarStatus[1] QueryVarStatus[2] being EFI_SUCCESS or EFI_UNSUPPORTED
    Result = EFI_TEST_ASSERTION_PASSED;
    ErrorCode = 0;
    for (SubIndex = 0; SubIndex < 3; SubIndex++) {
      if (QueryVarStatus[SubIndex] == EFI_UNSUPPORTED) {
        Result = EFI_TEST_ASSERTION_WARNING;
        ErrorCode = 0;
        goto LoopEnd;
      }
      if(QueryVarStatus[SubIndex] != EFI_SUCCESS) {
        Result = EFI_TEST_ASSERTION_FAILED;
        ErrorCode = 1;
        goto LoopEnd;
      } 
    }
 
    for (SubIndex = 0; SubIndex < 3; SubIndex++) {
      if(MaximumVariableStorageSize[SubIndex] != MaximumVariableStorageSize[0]) {
        Result = EFI_TEST_ASSERTION_FAILED;
        ErrorCode = 2;
        goto LoopEnd;
      } 
    }
/*
    for (SubIndex = 0; SubIndex < 3; SubIndex++) {
      if (MaximumVariableSize[SubIndex] != MaximumVariableSize[0]) {
        Result = EFI_TEST_ASSERTION_FAILED;
        ErrorCode = 3;
        goto LoopEnd;
      }
    }
*/
LoopEnd:
    //
    // Record assertion
    //
    StandardLib->RecordAssertion (
                   StandardLib,
                   Result,
                   gVariableServicesBbTestQueryVarInfoAssertionGuid007,
                   L"RT.QueryVariableInfo - Boottime function test",
                   L"%a:%d: Error Code %d Attributes %d [%d]",
                   __FILE__,
                   (UINTN)__LINE__,
                   ErrorCode,
                   ValidAttributes[Index],
                   Index
                   );

    //
    // Exit the loop if there is an Error
    //
    if (ErrorCode != 0) {
      break;
    }
  }

  //
  // Trace ...
  //
  if (LoggingLib != NULL) {
    LoggingLib->ExitFunction (
                  LoggingLib,
                  L"QueryVariableInfoFuncTestSub1",
                  L"TDS 4.4.2.1"
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
HardwareErrorRecordFuncTest (
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
  
  // UINTN                 RecoveryDataSize;
  // UINT8                 *RecoveryData;
  
  CHAR16                *HwErrRecName = L"HwErrRecSupport";
  UINT16                HwErrRecSupportVariable;
  UINTN                 DataSize;
  
  UINT32                Attributes = EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS
                                     |EFI_VARIABLE_RUNTIME_ACCESS|EFI_VARIABLE_HARDWARE_ERROR_RECORD;
  UINT64                MaximumVariableStorageSize;
  UINT64                RemainingVariableStorageSize;
  UINT64                MaximumVariableSize;
  
  CHAR16                HwErrRecVariableName[HW_ERR_REC_VARIABLE_NAME_LEN];
  CHAR16                HwErrRecVariable[] = L"This is a HwErrRec variable!";
  
  CHAR16                GetVariableName[MAX_BUFFER_SIZE];
  UINTN                 VariableNameSize;
  EFI_GUID              VendorGuid;
  
  UINTN                 Num;
  UINTN                 MaxNum = 0;
  CHAR16                ErrorNum[HW_ERR_REC_VARIABLE_NAME_INDEX_LEN + 1];
  
  CHAR16                HwErrRecGetVariable[255];
  
  EFI_TEST_ASSERTION    AssertionType;
  
  
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

  if (LoggingLib != NULL) {
    LoggingLib->EnterFunction (
                  LoggingLib,
                  L"HardwareErrorRecord Func Test",
                  L"TDS"
                  );
  }
  
  //
  // Allocate memory for recovery data
  //
  // RecoveryDataSize = 1024;
  // Status = gtBS->AllocatePool (
  //                  EfiLoaderData,
  //                  RecoveryDataSize,
  //                  (VOID **)&RecoveryData
  //                  );
  // if ( EFI_ERROR(Status) ) {
  //   return Status;
  // }

  //
  // Try to read reset record from the RecoveryData,
  // and the magic num is saved in the RecoveryData[0]. 
  // When the status is EFI_SUCCESS and magic num is 2,
  // it means useful data has been saved before the reset
  // and the date should be retrived goto particular process
  //
  // Status = RecoveryLib->ReadResetRecord (
  //                         RecoveryLib,
  //                         &RecoveryDataSize,
  //                         RecoveryData
  //                         );
  // if ( !EFI_ERROR(Status) && (RecoveryDataSize > 0) ) {
  //   switch (RecoveryData[0]) {
  //   case 2:
  //     goto step2;
  //   default:
  //     goto step3;
  //   }
  // }
  
  //
  // Check if the platform implements support for Hardware Error Record Persistence
  //
  DataSize = sizeof( HwErrRecSupportVariable );
  HwErrRecSupportVariable = 0xFFFF;
  Status = RT->GetVariable (
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

  
  if ( EFI_SUCCESS != Status || !HwErrRecSupportVariable ) {
    // gtBS->FreePool (RecoveryData);
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
                         L"\r\n Query the variable info : MaximumVariableStorageSize - %d, RemainingVariableStorageSize - %d",
                         MaximumVariableStorageSize,
                         RemainingVariableStorageSize
                         );
  
  if ( Status != EFI_SUCCESS || RemainingVariableStorageSize <= 0 ) {
    // gtBS->FreePool (RecoveryData);
    return Status;
  }
  
  //
  // Get a useable variable name
  //
  GetVariableName[0] = L'\0';
  ErrorNum[HW_ERR_REC_VARIABLE_NAME_INDEX_LEN] = L'\0';
  

  while (TRUE) {
    VariableNameSize = MAX_BUFFER_SIZE * sizeof (CHAR16);
    Status = RT->GetNextVariableName (
                       &VariableNameSize,         // VariableNameSize
                       GetVariableName,           // VariableName
                       &VendorGuid                // VendorGuid
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

    if ( (SctStrnCmp (GetVariableName, L"HwErrRec", HW_ERR_REC_VARIABLE_NAME_PREFIX_LEN) == 0) &&
         (SctCompareGuid (&VendorGuid, &gHwErrRecGuid) == 0) ) {
      SctStrnCpy (ErrorNum, &GetVariableName[HW_ERR_REC_VARIABLE_NAME_PREFIX_LEN], HW_ERR_REC_VARIABLE_NAME_INDEX_LEN);
      Num = SctXtoi (ErrorNum);
      if (MaxNum < Num)
        MaxNum = Num;
    }
  }

  MaxNum++;
    
  HwErrRecVariableName[0] = L'\0';
  SctStrCat ( HwErrRecVariableName, L"HwErrRec" );
  Myitox( MaxNum, HwErrRecVariableName + HW_ERR_REC_VARIABLE_NAME_PREFIX_LEN );
  HwErrRecVariableName[HW_ERR_REC_VARIABLE_NAME_LEN - 1] = L'\0';
  
  //
  // Set the new HwErrRec variable to the global variable
  //
  DataSize = sizeof( HwErrRecVariable );
  Status = RT->SetVariable (
                        HwErrRecVariableName,
                        &gHwErrRecGuid,
                        Attributes,
                        DataSize,
                        HwErrRecVariable
                        );
  if ( EFI_ERROR(Status) ) {
    // gtBS->FreePool (RecoveryData);
    return Status;
  }
  
  //
  // Before the reset, test writes magic num 2 in RecoveryData[0]
  // and writes the useful data - HwErrRecVariableName - to RecoveryData[2]
  //
  // RecoveryData[0] = 2;
  // SctStrnCpy ( (CHAR16*)(&RecoveryData[2]), HwErrRecVariableName, HW_ERR_REC_VARIABLE_NAME_LEN - 1 );
  // RecoveryLib->WriteResetRecord( RecoveryLib, HW_ERR_REC_VARIABLE_NAME_LEN * sizeof(CHAR16) + 2, RecoveryData );
  
  //
  // Prompt the user about the cold reset and reset the system
  //
  // SctPrint (L"\r\nSystem will cold reset after 1 second...");
  // gtBS->Stall (1000000);

  // gtRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  
  //
  // The particular process after the reset
  // retrive the useful data - HwErrRecVariableName - from RecoveryData[2]
  //
  // step2:
  DataSize = 255;
  // HwErrRecVariableName[HW_ERR_REC_VARIABLE_NAME_LEN - 1] = L'\0';
  // SctStrnCpy ( HwErrRecVariableName, (CHAR16*)(RecoveryData+2), HW_ERR_REC_VARIABLE_NAME_LEN - 1 );
  Status = RT->GetVariable (
                        HwErrRecVariableName,
                        &gHwErrRecGuid,
                        NULL,
                        &DataSize,
                        HwErrRecGetVariable
                        );
  if ( EFI_ERROR(Status) ) {
    // gtBS->FreePool (RecoveryData);
    return Status;
  }
  
  if ( SctStrCmp ( HwErrRecGetVariable, HwErrRecVariable ) ) {
    AssertionType = EFI_TEST_ASSERTION_FAILED;
  } else {
    AssertionType = EFI_TEST_ASSERTION_PASSED;
  }
  
  //
  // Compare the result
  //
  StandardLib->RecordAssertion (
                     StandardLib,
                     AssertionType,
                     gHwErrRecBbTestAssertionGuid001,
                     L"HwErrRec varuable test.",
                     L"%a:%d",
                     __FILE__,
                     (UINTN)__LINE__
                     );
  
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
  // Free resources
  //
  // step3:
  //   gtBS->FreePool (RecoveryData);

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
