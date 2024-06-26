/** @file
Host-based unit test for the VariableRuntimeDxe driver. Will
use mocks for all external interfaces.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariableRuntimeDxeUnitTest.h"
#include <Library/UnitTestLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "../Variable.h"
#include "BlackBoxTest/VariableServicesBBTestMain.h"

#define UNIT_TEST_NAME     "RuntimeVariableDxe Host-Based Unit Test"
#define UNIT_TEST_VERSION  "1.0"

BOOLEAN  mTestAtRuntime = FALSE;
EFI_TPL  mTestTpl       = TPL_APPLICATION;

//
// Mock version of the UEFI Boot Services Table
//
EFI_BOOT_SERVICES  MockBoot = {
  {
    EFI_BOOT_SERVICES_SIGNATURE,              // Signature
    EFI_BOOT_SERVICES_REVISION,               // Revision
    sizeof (EFI_BOOT_SERVICES),               // HeaderSize
    0,                                        // CRC32
    0                                         // Reserved
  },
  NULL,                                       // RaiseTPL
  NULL,                                       // RestoreTPL
  NULL,                                       // AllocatePages
  NULL,                                       // FreePages
  NULL,                                       // GetMemoryMap
  NULL,                                       // AllocatePool
  NULL,                                       // FreePool
  NULL,                                       // CreateEvent
  NULL,                                       // SetTimer
  NULL,                                       // WaitForEvent
  NULL,                                       // SignalEvent
  NULL,                                       // CloseEvent
  NULL,                                       // CheckEvent
  NULL,                                       // InstallProtocolInterface
  NULL,                                       // ReinstallProtocolInterface
  NULL,                                       // UninstallProtocolInterface
  NULL,                                       // HandleProtocol
  (VOID *)NULL,                               // Reserved
  NULL,                                       // RegisterProtocolNotify
  NULL,                                       // LocateHandle
  NULL,                                       // LocateDevicePath
  NULL,                                       // InstallConfigurationTable
  NULL,                                       // LoadImage
  NULL,                                       // StartImage
  NULL,                                       // Exit
  NULL,                                       // UnloadImage
  NULL,                                       // ExitBootServices
  NULL,                                       // GetNextMonotonicCount
  NULL,                                       // Stall
  NULL,                                       // SetWatchdogTimer
  NULL,                                       // ConnectController
  NULL,                                       // DisconnectController
  NULL,                                       // OpenProtocol
  NULL,                                       // CloseProtocol
  NULL,                                       // OpenProtocolInformation
  NULL,                                       // ProtocolsPerHandle
  NULL,                                       // LocateHandleBuffer
  NULL,                                       // LocateProtocol
  NULL,                                       // InstallMultipleProtocolInterfaces
  NULL,                                       // UninstallMultipleProtocolInterfaces
  NULL,                                       // CalculateCrc32
  (EFI_COPY_MEM)CopyMem,                      // CopyMem
  (EFI_SET_MEM)SetMem,                        // SetMem
  NULL                                        // CreateEventEx
};

///
/// Mock version of the UEFI Runtime Services Table
///
EFI_RUNTIME_SERVICES  MockRuntime = {
  {
    EFI_RUNTIME_SERVICES_SIGNATURE,     // Signature
    EFI_RUNTIME_SERVICES_REVISION,      // Revision
    sizeof (EFI_RUNTIME_SERVICES),      // HeaderSize
    0,                                  // CRC32
    0                                   // Reserved
  },
  NULL,               // GetTime
  NULL,               // SetTime
  NULL,               // GetWakeupTime
  NULL,               // SetWakeupTime
  NULL,               // SetVirtualAddressMap
  NULL,               // ConvertPointer
  NULL,               // GetVariable
  NULL,               // GetNextVariableName
  NULL,               // SetVariable
  NULL,               // GetNextHighMonotonicCount
  NULL,               // ResetSystem
  NULL,               // UpdateCapsule
  NULL,               // QueryCapsuleCapabilities
  NULL                // QueryVariableInfo
};

VOID
EFIAPI
RecordSecureBootPolicyVarData (
  VOID
  );

EFI_TPL
EFIAPI
MockRaiseTpl (
  IN EFI_TPL  NewTpl
  )
{
  EFI_TPL  OldTpl;

  OldTpl = mTestTpl;
  if (OldTpl > NewTpl) {
    DEBUG ((DEBUG_ERROR, "FATAL ERROR - RaiseTpl with OldTpl(0x%x) > NewTpl(0x%x)\n", OldTpl, NewTpl));
    ASSERT (FALSE);
  }

  mTestTpl = NewTpl;

  return OldTpl;
}

VOID
EFIAPI
MockRestoreTpl (
  IN EFI_TPL  NewTpl
  )
{
  EFI_TPL  OldTpl;

  OldTpl = mTestTpl;
  if (NewTpl > OldTpl) {
    DEBUG ((DEBUG_ERROR, "FATAL ERROR - RestoreTpl with NewTpl(0x%x) > OldTpl(0x%x)\n", NewTpl, OldTpl));
    ASSERT (FALSE);
  }

  mTestTpl = NewTpl;
}

EFI_STATUS
EFIAPI
MockFreePool (
  VOID  *Pool
  )
{
  FreePool (Pool);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MockLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  )
{
  return EFI_NOT_FOUND;
}

BOOLEAN
AtRuntime (
  VOID
  )
{
  return mTestAtRuntime;
}

/**
  Initializes a basic mutual exclusion lock.

  This function initializes a basic mutual exclusion lock to the released state
  and returns the lock.  Each lock provides mutual exclusion access at its task
  priority level.  Since there is no preemption or multiprocessor support in EFI,
  acquiring the lock only consists of raising to the locks TPL.
  If Lock is NULL, then ASSERT().
  If Priority is not a valid TPL value, then ASSERT().

  @param  Lock       A pointer to the lock data structure to initialize.
  @param  Priority   EFI TPL is associated with the lock.

  @return The lock.

**/
EFI_LOCK *
InitializeLock (
  IN OUT EFI_LOCK  *Lock,
  IN     EFI_TPL   Priority
  )
{
  return EfiInitializeLock (Lock, Priority);
}

/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function that will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to acquire.

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!AtRuntime ()) {
    EfiAcquireLock (Lock);
  }
}

/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiReleaseLock() at boot time and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to release.

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!AtRuntime ()) {
    EfiReleaseLock (Lock);
  }
}

/**
  Retrieve the Fault Tolerent Write protocol interface.

  @param[out] FtwProtocol       The interface of Ftw protocol

  @retval EFI_SUCCESS           The FTW protocol instance was found and returned in FtwProtocol.
  @retval EFI_NOT_FOUND         The FTW protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
GetFtwProtocol (
  OUT VOID  **FtwProtocol
  )
{
  // TODO: Create a mocked version.
  return EFI_UNSUPPORTED;
}

/**
  Retrieve the FVB protocol interface by HANDLE.

  @param[in]  FvBlockHandle     The handle of FVB protocol that provides services for
                                reading, writing, and erasing the target block.
  @param[out] FvBlock           The interface of FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.

**/
EFI_STATUS
GetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
{
  //
  // To get the FVB protocol interface on the handle
  //
  // TODO: Create a mocked version.
  return EFI_UNSUPPORTED;
}

/**
  Function returns an array of handles that support the FVB protocol
  in a buffer allocated from pool.

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to the buffer to return the requested
                                array of  handles that support FVB protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer, and the number of
                                handles in Buffer was returned in NumberHandles.
  @retval EFI_NOT_FOUND         No FVB handle was found.
  @retval EFI_OUT_OF_RESOURCES  There is not enough pool memory to store the matching results.
  @retval EFI_INVALID_PARAMETER NumberHandles is NULL or Buffer is NULL.

**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN       *NumberHandles,
  OUT EFI_HANDLE  **Buffer
  )
{
  //
  // Locate all handles of Fvb protocol
  //
  // TODO: Create a mocked version.
  return EFI_UNSUPPORTED;
}

UNIT_TEST_STATUS
EFIAPI
DummyTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS  TestResult = UNIT_TEST_PASSED;
  T_VAR             *VarA;
  UINTN             DataSize;
  UINT8             Data[10];
  UINT32            Attributes;

  VarA = LoadTestVariable ("TestVarA");
  UT_CLEANUP_ASSERT_NOT_NULL (VarA);

  UT_CLEANUP_ASSERT_NOT_EFI_ERROR (
    VariableServiceSetVariable (
      VarA->Name,
      &VarA->VendorGuid,
      VarA->Attributes,
      VarA->DataSize,
      VarA->Data
      )
    );

  DataSize = sizeof (Data);
  UT_CLEANUP_ASSERT_NOT_EFI_ERROR (
    VariableServiceGetVariable (
      VarA->Name,
      &VarA->VendorGuid,
      &Attributes,
      &DataSize,
      (VOID *)Data
      )
    );

  UT_CLEANUP_ASSERT_EQUAL (VarA->Attributes, Attributes);
  UT_CLEANUP_ASSERT_EQUAL (VarA->DataSize, DataSize);
  UT_CLEANUP_ASSERT_MEM_EQUAL (VarA->Data, Data, VarA->DataSize);

Cleanup:
  if (VarA != NULL) {
    FreeTestVariable (VarA);
  }

  return TestResult;
}

#define SCT_TEST_WRAPPER_FUNCTION(TestName)    \
  UNIT_TEST_STATUS                              \
  EFIAPI                                        \
  TestName##Wrapper (                           \
    IN UNIT_TEST_CONTEXT      Context           \
    )                                           \
  {                                             \
    UNIT_TEST_STATUS              Result;       \
    SCT_HOST_TEST_PRIVATE_DATA    TestData;     \
                                                \
    Result = UNIT_TEST_PASSED;                  \
                                                \
    UT_ASSERT_NOT_EFI_ERROR( InitSctPrivateData( &Result, &TestData ) );  \
    TestName (NULL,                             \
              (VOID*)gRT,                       \
              EFI_TEST_LEVEL_DEFAULT,           \
              (EFI_HANDLE)&TestData);           \
                                                \
    UT_ASSERT_EQUAL(Result, UNIT_TEST_PASSED);  \
    return Result;                              \
  }

SCT_TEST_WRAPPER_FUNCTION (GetVariableConfTest)
SCT_TEST_WRAPPER_FUNCTION (GetNextVariableNameConfTest)
SCT_TEST_WRAPPER_FUNCTION (SetVariableConfTest)
SCT_TEST_WRAPPER_FUNCTION (QueryVariableInfoConfTest)
SCT_TEST_WRAPPER_FUNCTION (GetVariableFuncTest)
SCT_TEST_WRAPPER_FUNCTION (GetNextVariableNameFuncTest)
SCT_TEST_WRAPPER_FUNCTION (SetVariableFuncTest)
SCT_TEST_WRAPPER_FUNCTION (QueryVariableInfoFuncTest)
SCT_TEST_WRAPPER_FUNCTION (HardwareErrorRecordConfTest)
SCT_TEST_WRAPPER_FUNCTION (HardwareErrorRecordFuncTest)
SCT_TEST_WRAPPER_FUNCTION (MultipleStressTest)
SCT_TEST_WRAPPER_FUNCTION (OverflowStressTest)

EFI_STATUS
EFIAPI
VarCheckPolicyLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

STATIC
VOID
InitVariableDriver (
  VOID
  )
{
  // NOTE: This initialization should be performed per-suite, probably.
  //       But to do that optimally, I think we'd need to be able to deinit. Dunno.
  //       We'll play around with it.
  MockRuntime.GetVariable         = VariableServiceGetVariable;
  MockRuntime.GetNextVariableName = VariableServiceGetNextVariableName;
  MockRuntime.SetVariable         = VariableServiceSetVariable;
  MockRuntime.QueryVariableInfo   = VariableServiceQueryVariableInfo;

  MockBoot.RaiseTPL       = MockRaiseTpl;
  MockBoot.RestoreTPL     = MockRestoreTpl;
  MockBoot.FreePool       = MockFreePool;
  MockBoot.LocateProtocol = MockLocateProtocol;

  ASSERT_EFI_ERROR (VarCheckPolicyLibConstructor (NULL, NULL));

  ASSERT_EFI_ERROR (VariableCommonInitialize ());
  ASSERT_EFI_ERROR (VariableWriteServiceInitialize ());
  RecordSecureBootPolicyVarData ();

  InitSctShim (&MockBoot, &MockRuntime);

  // Signal EndOfDxe so that the driver code assumes setup is complete.
  MorLockInitAtEndOfDxe ();
  mEndOfDxe = TRUE;
  VarCheckLibInitializeAtEndOfDxe (NULL);
  InitializeVariableQuota ();
}

/**
  Main fuction sets up the unit test environment
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      GenericTests;
  UNIT_TEST_SUITE_HANDLE      SctConformanceTests;
  UNIT_TEST_SUITE_HANDLE      SctFunctionalTests;
  UNIT_TEST_SUITE_HANDLE      SctHwErrTests;
  UNIT_TEST_SUITE_HANDLE      SctStressTests;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the Example Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&GenericTests, Framework, "Generic Tests", "Generic", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for GenericTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (GenericTests, "Dummy Test", "Dummy", DummyTest, NULL, NULL, NULL);

  //
  // Populate the SCT Conformance TDS 3.1-3.4 Unit Test Suite
  //
  Status = CreateUnitTestSuite (&SctConformanceTests, Framework, "SCT Conformance Tests Suite", "SctConformance", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SctConformanceTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (SctConformanceTests, "GetVariableConf Test", "GetVariableConf", GetVariableConfTestWrapper, NULL, NULL, NULL);
  AddTestCase (SctConformanceTests, "GetNextVariableNameConf Test", "GetNextVariableNameConf", GetNextVariableNameConfTestWrapper, NULL, NULL, NULL);
  AddTestCase (SctConformanceTests, "SetVariableConf Test", "SetVariableConf", SetVariableConfTestWrapper, NULL, NULL, NULL);
  AddTestCase (SctConformanceTests, "QueryVariableInfoConf Test", "QueryVariableInfoConf", QueryVariableInfoConfTestWrapper, NULL, NULL, NULL);

  //
  // Populate the SCT Functional TDS 4.1-4.4 Unit Test Suite
  //
  Status = CreateUnitTestSuite (&SctFunctionalTests, Framework, "SCT Functional Tests Suite", "SctFunctional", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SctFunctionalTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (SctFunctionalTests, "GetVariableFunc Test", "GetVariableFunc", GetVariableFuncTestWrapper, NULL, NULL, NULL);
  AddTestCase (SctFunctionalTests, "GetNextVariableNameFunc Test", "GetNextVariableNameFunc", GetNextVariableNameFuncTestWrapper, NULL, NULL, NULL);
  AddTestCase (SctFunctionalTests, "SetVariableFunc Test", "SetVariableFunc", SetVariableFuncTestWrapper, NULL, NULL, NULL);
  AddTestCase (SctFunctionalTests, "QueryVariableInfoFunc Test", "QueryVariableInfoFunc", QueryVariableInfoFuncTestWrapper, NULL, NULL, NULL);

  //
  // Populate the SCT HwErrRecord Unit Test Suite
  //
  Status = CreateUnitTestSuite (&SctHwErrTests, Framework, "SCT HwErrRecord Tests Suite", "SctHwErr", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SctHwErrTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (SctHwErrTests, "HardwareErrorRecordConf Test", "HardwareErrorRecordConf", HardwareErrorRecordConfTestWrapper, NULL, NULL, NULL);
  AddTestCase (SctHwErrTests, "HardwareErrorRecordFunc Test", "HardwareErrorRecordFunc", HardwareErrorRecordFuncTestWrapper, NULL, NULL, NULL);

  //
  // Populate the SCT Stress TDS 5.1-5.2 Test Suite
  //
  Status = CreateUnitTestSuite (&SctStressTests, Framework, "SCT Stress Tests Suite", "SctStress", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SctStressTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (SctStressTests, "MultipleStress Test", "MultipleStress", MultipleStressTestWrapper, NULL, NULL, NULL);
  AddTestCase (SctStressTests, "OverflowStress Test", "OverflowStress", OverflowStressTestWrapper, NULL, NULL, NULL);

  InitVariableDriver ();

  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UefiTestMain ();
}
