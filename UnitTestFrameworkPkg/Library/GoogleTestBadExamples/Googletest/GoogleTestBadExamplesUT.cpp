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

  // Reason:
  //   The reason this passed is due to the fact that the LocateProtocol did not modify the pointer on its way out.
  //   Thus the call of policy protocol will fail. However, since this is a death test, the output will not be dumped
  //   visibly.

  // Solution:
  //   - One should NEVER EVER use "" for failed string matching!!
  //     Instead inspect what the real failure is and put that in the matcher field:
  //     `EXPECT_DEATH (GetPolicy (), HasSubstr("GoogleTestBadExamplesSrc.c, line 39"));`
  //   - The MdePkg/Include/Library/DebugLib.h should hook up to the system assert pipeline
  //     so that the gtest can read error from stderr:
  //     ```
  //       [BuildOptions]
  //         *_*_*_CC_FLAGS = -D GOOGLE_TESTING
  //     ```
}

/**
  Unit test for GetPolicy, expected death will have "1 leaked mock object".

  Expected: PASSED
  Actual:   FAILED, due to an error regarding "1 leaked mock object".
**/
TEST_F (GoogleTestBadExample, GetPolicyExpectAssert) {
  EXPECT_CALL (UefiBootServicesTableLib, gBS_LocateProtocol (&gEfiBootManagerPolicyProtocolGuid,_,_))
      .WillOnce(Return (EFI_NOT_FOUND));

  EXPECT_DEATH (GetPolicy (), "");

  // Reason:
  //   The reason this failed is due to the GetPolicy () runs in child process, and the access to gBS_LocateProtocol
  //   from the child process will not have the parent process notified, thus the parent process will percieve that
  //   the gBS_LocateProtocol was never invoked.

  // Solution:
  //   - The MdePkg/Include/Library/DebugLib.h should hook up to the system assert pipeline
  //     so that the gtest can read error from stderr:
  //     ```
  //       [BuildOptions]
  //         *_*_*_CC_FLAGS = -D GOOGLE_TESTING
  //     ```
  //   - Move the expect call into the child process as well, so that it can be detected:
  //     ```
  //       EXPECT_DEATH ({
  //       EXPECT_CALL (UefiBootServicesTableLib, gBS_LocateProtocol (&gEfiBootManagerPolicyProtocolGuid,_,_))
  //         .WillOnce(Return (EFI_NOT_FOUND));
  //       GetPolicy ();
  //       }, HasSubstr("GoogleTestBadExamplesSrc.c, line 39"));
  //     ```
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
