/** @file
  Unit tests for the PlatformBootConfigurationLib using google tests

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
#include <Uefi.h>
#include <Protocol/Policy.h>
#include <Library/BaseLib.h>
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
  .LocateProtocol = gBS_LocateProtocol
};

EFI_BOOT_SERVICES  *gBS = &LocalBs;

// TODO: Populate mocked policy here.

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

/**
  Unit test for GetPolicy, expected death will pass with just a warning if no mocked values supplied.

  Expected: FAILED
  Actual:   PASSED, with a warning
**/
TEST_F (GoogleTestBadExample, GetPolicyExpectedAssertNotHappen) {
  EXPECT_DEATH (GetPolicy (), "");

  // TODO: Solution here.
}

/**
  Unit test for GetPolicy, expected death will have "1 leaked mock object".

  Expected: PASSED
  Actual:   FAILED, due to an error regarding "1 leaked mock object".
**/
TEST_F (GoogleTestBadExample, GetPolicyExpectAssert) {
  EXPECT_CALL (UefiBootServicesTableLib, gBS_LocateProtocol (&gPolicyProtocolGuid,_,_))
      .WillOnce(Return (EFI_NOT_FOUND));

  EXPECT_DEATH (GetPolicy (), "");

  // TODO: Solution here.
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
