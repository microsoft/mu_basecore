/** @file
  UEFI Shell based unit test for EFI_BOOT_MANAGER_POLICY_PROTOCOL.

  Tests ConnectDevicePath and ConnectDeviceClass with all defined class GUIDs
  (Console, Network, ConnectAll, Storage) and verifies that an unknown GUID
  returns EFI_NOT_FOUND.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UnitTestLib.h>
#include <Protocol/BootManagerPolicy.h>

#define UNIT_TEST_APP_NAME     "BootManagerPolicyProtocol Unit Test"
#define UNIT_TEST_APP_VERSION  "1.0"

///
/// Context structure shared across all test cases.
///
typedef struct {
  EFI_BOOT_MANAGER_POLICY_PROTOCOL    *Protocol;
} BM_POLICY_TEST_CONTEXT;

STATIC BM_POLICY_TEST_CONTEXT  mTestContext;

///
/// A fabricated GUID that should not match any known device class.
///
STATIC EFI_GUID  mUnknownClassGuid = {
  0xDEADBEEF, 0x1234, 0x5678, { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89 }
};

/**
  Helper to determine if a status is a spec-valid return from ConnectDeviceClass
  for a known GUID. Per UEFI spec, valid returns are:
    EFI_SUCCESS, EFI_DEVICE_ERROR, EFI_NOT_FOUND, EFI_UNSUPPORTED
**/
STATIC
BOOLEAN
IsValidConnectDeviceClassStatus (
  IN EFI_STATUS  Status
  )
{
  return (Status == EFI_SUCCESS) ||
         (Status == EFI_DEVICE_ERROR) ||
         (Status == EFI_NOT_FOUND) ||
         (Status == EFI_UNSUPPORTED);
}

/**
  Helper to determine if a status is a spec-valid return from ConnectDevicePath.
  Per UEFI spec, valid returns are:
    EFI_SUCCESS, EFI_NOT_FOUND, EFI_SECURITY_VIOLATION, EFI_UNSUPPORTED
**/
STATIC
BOOLEAN
IsValidConnectDevicePathStatus (
  IN EFI_STATUS  Status
  )
{
  return (Status == EFI_SUCCESS) ||
         (Status == EFI_NOT_FOUND) ||
         (Status == EFI_SECURITY_VIOLATION) ||
         (Status == EFI_UNSUPPORTED);
}

// =============================================================================
// Prerequisite Fixture
// =============================================================================

/**
  Prerequisite function that locates the Boot Manager Policy Protocol.
  If the protocol is not available, the test case is skipped.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
LocateProtocolPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS              Status;
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  if (Ctx->Protocol != NULL) {
    return UNIT_TEST_PASSED;
  }

  Status = gBS->LocateProtocol (
                  &gEfiBootManagerPolicyProtocolGuid,
                  NULL,
                  (VOID **)&Ctx->Protocol
                  );

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (Ctx->Protocol);

  return UNIT_TEST_PASSED;
}

// =============================================================================
// Suite 1: Protocol Discovery
// =============================================================================

/**
  Test that the Boot Manager Policy Protocol can be located.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestLocateProtocol (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  UT_ASSERT_NOT_NULL (Ctx->Protocol);
  UT_ASSERT_NOT_NULL ((VOID *)Ctx->Protocol->ConnectDevicePath);
  UT_ASSERT_NOT_NULL ((VOID *)Ctx->Protocol->ConnectDeviceClass);

  return UNIT_TEST_PASSED;
}

/**
  Test that the protocol revision matches the expected value.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestProtocolRevision (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  UT_LOG_INFO ("Protocol revision is:  0x%llX\n", Ctx->Protocol->Revision);
  UT_ASSERT_EQUAL (Ctx->Protocol->Revision, EFI_BOOT_MANAGER_POLICY_PROTOCOL_REVISION);

  return UNIT_TEST_PASSED;
}

// =============================================================================
// Suite 2: ConnectDeviceClass with Known GUIDs
// =============================================================================

/**
  Test ConnectDeviceClass with EFI_BOOT_MANAGER_POLICY_CONSOLE_GUID.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestConnectDeviceClassConsole (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS              Status;
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  // logging statement so if system uefi log needs to be evaluated it is clear which operation was requested
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(Console) - Start\n", UNIT_TEST_APP_NAME));

  Status = Ctx->Protocol->ConnectDeviceClass (
                            Ctx->Protocol,
                            &gEfiBootManagerPolicyConsoleGuid
                            );
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(Console) - End\n", UNIT_TEST_APP_NAME)); // log completed operation for clarity in logs

  UT_ASSERT_TRUE (IsValidConnectDeviceClassStatus (Status));
  UT_LOG_INFO ("ConnectDeviceClass(Console) returned %r\n", Status);

  return UNIT_TEST_PASSED;
}

/**
  Test ConnectDeviceClass with EFI_BOOT_MANAGER_POLICY_NETWORK_GUID.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestConnectDeviceClassNetwork (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS              Status;
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  // Logging statement so if system uefi log needs to be evaluated it is clear which operation was requested
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(Network) - Start\n", UNIT_TEST_APP_NAME));

  Status = Ctx->Protocol->ConnectDeviceClass (
                            Ctx->Protocol,
                            &gEfiBootManagerPolicyNetworkGuid
                            );
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(Network) - End\n", UNIT_TEST_APP_NAME)); // log completed operation for clarity in logs

  UT_ASSERT_TRUE (IsValidConnectDeviceClassStatus (Status));
  UT_LOG_INFO ("ConnectDeviceClass(Network) returned %r\n", Status);

  return UNIT_TEST_PASSED;
}

/**
  Test ConnectDeviceClass with EFI_BOOT_MANAGER_POLICY_CONNECT_ALL_GUID.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestConnectDeviceClassConnectAll (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS              Status;
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  // Logging statement so if system uefi log needs to be evaluated it is clear which operation was requested
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(ConnectAll) - Start\n", UNIT_TEST_APP_NAME));

  Status = Ctx->Protocol->ConnectDeviceClass (
                            Ctx->Protocol,
                            &gEfiBootManagerPolicyConnectAllGuid
                            );
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(ConnectAll) - End\n", UNIT_TEST_APP_NAME)); // log completed operation for clarity in logs

  UT_ASSERT_TRUE (IsValidConnectDeviceClassStatus (Status));
  UT_LOG_INFO ("ConnectDeviceClass(ConnectAll) returned %r\n", Status);

  return UNIT_TEST_PASSED;
}

/**
  Test ConnectDeviceClass with EFI_BOOT_MANAGER_POLICY_STORAGE_GUID.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestConnectDeviceClassStorage (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS              Status;
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  // Logging statement so if system uefi log needs to be evaluated it is clear which operation was requested
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(Storage) - Start\n", UNIT_TEST_APP_NAME));

  Status = Ctx->Protocol->ConnectDeviceClass (
                            Ctx->Protocol,
                            &gEfiBootManagerPolicyStorageGuid
                            );
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(Storage) - End\n", UNIT_TEST_APP_NAME)); // log completed operation for clarity in logs

  UT_ASSERT_TRUE (IsValidConnectDeviceClassStatus (Status));
  UT_LOG_INFO ("ConnectDeviceClass(Storage) returned %r\n", Status);

  return UNIT_TEST_PASSED;
}

// =============================================================================
// Suite 3: ConnectDeviceClass with Unknown GUID
// =============================================================================

/**
  Test ConnectDeviceClass with an unknown GUID.
  Per UEFI spec, the expected return is EFI_NOT_FOUND.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestConnectDeviceClassUnknownGuid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS              Status;
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  // Logging statement so if system uefi log needs to be evaluated it is clear which operation was requested
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(Unknown GUID) - Start\n", UNIT_TEST_APP_NAME));

  Status = Ctx->Protocol->ConnectDeviceClass (
                            Ctx->Protocol,
                            &mUnknownClassGuid
                            );
  DEBUG ((DEBUG_INFO, "[%a] - Connect DeviceClass(Unknown GUID) - End\n", UNIT_TEST_APP_NAME)); // log completed operation for clarity in logs

  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  return UNIT_TEST_PASSED;
}

// =============================================================================
// Suite 4: ConnectDevicePath
// =============================================================================

/**
  Test ConnectDevicePath with a NULL DevicePath (non-recursive).
  Per UEFI spec, NULL DevicePath connects all controllers using platform policy.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestConnectDevicePathNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS              Status;
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  // Logging statement so if system uefi log needs to be evaluated it is clear which operation was requested
  DEBUG ((DEBUG_INFO, "[%a] - Connect DevicePath(NULL, FALSE) - Start\n", UNIT_TEST_APP_NAME));

  Status = Ctx->Protocol->ConnectDevicePath (
                            Ctx->Protocol,
                            NULL,
                            FALSE
                            );
  DEBUG ((DEBUG_INFO, "[%a] - Connect DevicePath(NULL, FALSE) - End\n", UNIT_TEST_APP_NAME)); // log completed operation for clarity in logs

  UT_ASSERT_TRUE (IsValidConnectDevicePathStatus (Status));
  UT_LOG_INFO ("ConnectDevicePath(NULL, FALSE) returned %r\n", Status);

  return UNIT_TEST_PASSED;
}

/**
  Test ConnectDevicePath with a NULL DevicePath (recursive).
  Per UEFI spec, Recursive is ignored when DevicePath is NULL.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestConnectDevicePathNullRecursive (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS              Status;
  BM_POLICY_TEST_CONTEXT  *Ctx;

  Ctx = (BM_POLICY_TEST_CONTEXT *)Context;

  // Logging statement so if system uefi log needs to be evaluated it is clear which operation was requested
  DEBUG ((DEBUG_INFO, "[%a] - Connect DevicePath(NULL, TRUE) - Start\n", UNIT_TEST_APP_NAME));

  Status = Ctx->Protocol->ConnectDevicePath (
                            Ctx->Protocol,
                            NULL,
                            TRUE
                            );

  DEBUG ((DEBUG_INFO, "[%a] - Connect DevicePath(NULL, TRUE) - End\n", UNIT_TEST_APP_NAME)); // log completed operation for clarity in logs

  UT_ASSERT_TRUE (IsValidConnectDevicePathStatus (Status));
  UT_LOG_INFO ("ConnectDevicePath(NULL, TRUE) returned %r\n", Status);

  return UNIT_TEST_PASSED;
}

// =============================================================================
// Entry Point
// =============================================================================

/**
  Initialize the unit test framework and register all test cases.
**/
STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw;
  UNIT_TEST_SUITE_HANDLE      DiscoverySuite;
  UNIT_TEST_SUITE_HANDLE      KnownGuidSuite;
  UNIT_TEST_SUITE_HANDLE      UnknownGuidSuite;
  UNIT_TEST_SUITE_HANDLE      DevicePathSuite;

  Fw = NULL;

  ZeroMem (&mTestContext, sizeof (mTestContext));

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  Status = InitUnitTestFramework (
             &Fw,
             UNIT_TEST_APP_NAME,
             gEfiCallerBaseName,
             UNIT_TEST_APP_VERSION
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Suite 1: Protocol Discovery
  //
  Status = CreateUnitTestSuite (
             &DiscoverySuite,
             Fw,
             "Protocol Discovery Tests",
             "BootManagerPolicy.Discovery",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  AddTestCase (
    DiscoverySuite,
    "Locate Boot Manager Policy Protocol",
    "BootManagerPolicy.Discovery.LocateProtocol",
    TestLocateProtocol,
    LocateProtocolPreReq,
    NULL,
    &mTestContext
    );
  AddTestCase (
    DiscoverySuite,
    "Verify protocol revision",
    "BootManagerPolicy.Discovery.Revision",
    TestProtocolRevision,
    LocateProtocolPreReq,
    NULL,
    &mTestContext
    );

  //
  // Suite 2: ConnectDeviceClass with Known GUIDs
  //
  Status = CreateUnitTestSuite (
             &KnownGuidSuite,
             Fw,
             "ConnectDeviceClass Known GUID Tests",
             "BootManagerPolicy.ConnectDeviceClass.Known",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  AddTestCase (
    KnownGuidSuite,
    "ConnectDeviceClass with Console GUID",
    "BootManagerPolicy.ConnectDeviceClass.Console",
    TestConnectDeviceClassConsole,
    LocateProtocolPreReq,
    NULL,
    &mTestContext
    );
  AddTestCase (
    KnownGuidSuite,
    "ConnectDeviceClass with Network GUID",
    "BootManagerPolicy.ConnectDeviceClass.Network",
    TestConnectDeviceClassNetwork,
    LocateProtocolPreReq,
    NULL,
    &mTestContext
    );
  AddTestCase (
    KnownGuidSuite,
    "ConnectDeviceClass with Storage GUID",
    "BootManagerPolicy.ConnectDeviceClass.Storage",
    TestConnectDeviceClassStorage,
    LocateProtocolPreReq,
    NULL,
    &mTestContext
    );

  // Put this test last in teh suite since ConnectAll may have side effects of connecting multiple
  // devices and impacting other test cases if they are not properly isolated.
  AddTestCase (
    KnownGuidSuite,
    "ConnectDeviceClass with ConnectAll GUID",
    "BootManagerPolicy.ConnectDeviceClass.ConnectAll",
    TestConnectDeviceClassConnectAll,
    LocateProtocolPreReq,
    NULL,
    &mTestContext
    );

  //
  // Suite 3: ConnectDeviceClass with Unknown GUID
  //
  Status = CreateUnitTestSuite (
             &UnknownGuidSuite,
             Fw,
             "ConnectDeviceClass Unknown GUID Tests",
             "BootManagerPolicy.ConnectDeviceClass.Unknown",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  AddTestCase (
    UnknownGuidSuite,
    "ConnectDeviceClass with unknown GUID returns EFI_NOT_FOUND",
    "BootManagerPolicy.ConnectDeviceClass.UnknownGuid",
    TestConnectDeviceClassUnknownGuid,
    LocateProtocolPreReq,
    NULL,
    &mTestContext
    );

  //
  // Suite 4: ConnectDevicePath
  //
  Status = CreateUnitTestSuite (
             &DevicePathSuite,
             Fw,
             "ConnectDevicePath Tests",
             "BootManagerPolicy.ConnectDevicePath",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  AddTestCase (
    DevicePathSuite,
    "ConnectDevicePath with NULL path non-recursive",
    "BootManagerPolicy.ConnectDevicePath.NullNonRecursive",
    TestConnectDevicePathNull,
    LocateProtocolPreReq,
    NULL,
    &mTestContext
    );
  AddTestCase (
    DevicePathSuite,
    "ConnectDevicePath with NULL path recursive",
    "BootManagerPolicy.ConnectDevicePath.NullRecursive",
    TestConnectDevicePathNullRecursive,
    LocateProtocolPreReq,
    NULL,
    &mTestContext
    );

  Status = RunAllTestSuites (Fw);

EXIT:
  if (Fw != NULL) {
    FreeUnitTestFramework (Fw);
  }

  return Status;
}

EFI_STATUS
EFIAPI
BootManagerPolicyProtocolTestEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UnitTestingEntry ();
}
