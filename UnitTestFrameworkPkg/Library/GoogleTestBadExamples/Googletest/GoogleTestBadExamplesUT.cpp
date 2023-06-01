/** @file
  Unit tests for the PlatformBootConfigurationLib using google tests

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
#include <Uefi.h>
#include <Protocol/BootManagerPolicy.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
GetPolicy (
  VOID
  );
}

// *----------------------------------------------------------------------------------*
// * Test Contexts                                                                    *
// *----------------------------------------------------------------------------------*

using namespace testing;
using ::testing::HasSubstr;

//
// Declarations to handle usage of the UefiBootServiceTableLib by creating mock
//
struct MockUefiBootServicesTableLib {
  MOCK_INTERFACE_DECLARATION (MockUefiBootServicesTableLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_LocateProtocol,
    (IN  EFI_GUID  *Protocol,
     IN  VOID      *Registration  OPTIONAL,
     OUT VOID      **Interface)
    );
};

MOCK_INTERFACE_DEFINITION (MockUefiBootServicesTableLib);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LocateProtocol, 3, EFIAPI);

static EFI_BOOT_SERVICES  LocalBs = {
  // This does not build on MSVC due to std:c++ usage
  .LocateProtocol = gBS_LocateProtocol

  // Solution: add below to test inf
  // [BuildOptions]
  //   MSFT:*_*_*_CC_FLAGS = /std:c++latest

};

EFI_BOOT_SERVICES  *gBS = &LocalBs;

/// ================================================================================================
/// ================================================================================================
///
/// TEST CASES
///
/// ================================================================================================
/// ================================================================================================

//
// Declarations for unit tests
//
class GoogleTestBadExample : public  Test {
protected:
EFI_STATUS Status;
MockUefiBootServicesTableLib UefiBootServicesTableLib;
};

UNIT_TEST_STATUS
EFIAPI
UTShimTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Use UT_EXPECT_ASSERT* to do death test.
  UT_EXPECT_ASSERT_FAILURE (GetPolicy (), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for GetPolicy, expected death by putting the expect_call
  in the subprocess as well.

  Status: PASSED
**/
TEST_F (GoogleTestBadExample, GetPolicyExpectedAssertNotHappen) {
  EXPECT_DEATH ({
    EXPECT_CALL (UefiBootServicesTableLib, gBS_LocateProtocol (&gEfiBootManagerPolicyProtocolGuid,_,_))
      .WillOnce(Return (EFI_NOT_FOUND));
    GetPolicy ();
    }, HasSubstr("GoogleTestBadExamplesSrc.c, line 39"));
}

/**
  Unit test for GetPolicy, expected death using UT_ASSERT*.

  Status: PASSED
**/
TEST_F (GoogleTestBadExample, GetPolicyExpectAssert) {
  UNIT_TEST_STATUS TestStatus;

  EXPECT_CALL (UefiBootServicesTableLib, gBS_LocateProtocol (&gEfiBootManagerPolicyProtocolGuid,_,_))
      .WillOnce(Return (EFI_NOT_FOUND));

  TestStatus = UTShimTest (NULL);
  EXPECT_EQ (TestStatus, (UNIT_TEST_STATUS)UNIT_TEST_PASSED);
}


int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
