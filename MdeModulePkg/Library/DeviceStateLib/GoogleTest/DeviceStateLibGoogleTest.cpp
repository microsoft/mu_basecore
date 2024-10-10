/** @file DeviceStateLibGoogleTest.cpp

  DeviceStateLib Google Test

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockPcdLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <Library/DeviceStateLib.h>
}

using namespace testing;

/**
  Test DeviceStateBitmaskIsSet function
**/
class DeviceStateBitmaskIsSetTest : public Test {
protected:
  StrictMock<MockPcdLib> PcdLibMock;
  DEVICE_STATE CurrentDeviceStateMock;
  DEVICE_STATE InsecureDeviceStateSettingMock;
  CHAR8 Buffer[MAX_INSECURE_DEVICE_STATE_STRING_SIZE];
  UINTN StringSize;
  UINTN *StrSizePtr = &StringSize;
  EFI_STATUS Status;

  void
  SetUp (
    ) override
  {
    CurrentDeviceStateMock         = 0x000000000;
    InsecureDeviceStateSettingMock = 0x000000000;
    Buffer[0]                      = '\0';
  }
};

/**
  Test GetInsecureDeviceStateString function failures

  The tested function calls GetInsecureDeviceStateSetting to get the FixedAtBuild PCD
  defined in MdeModulePkg.dec
**/
TEST_F (DeviceStateBitmaskIsSetTest, GetInsecureDeviceStateStringInvalidParameter) {
  CurrentDeviceStateMock |= DEVICE_STATE_SOURCE_DEBUG_ENABLED;

  EXPECT_CALL (
    PcdLibMock,
    LibPcdGet32
    )
    .Times (4)
    .WillRepeatedly (
       Return (CurrentDeviceStateMock)
       );

  Status = GetInsecureDeviceStateString (NULL, MAX_INSECURE_DEVICE_STATE_STRING_SIZE, StrSizePtr);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);

  Status = GetInsecureDeviceStateString (Buffer, 0, StrSizePtr);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);

  Status = GetInsecureDeviceStateString (Buffer, MAX_INSECURE_DEVICE_STATE_STRING_SIZE, NULL);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);

  Status = GetInsecureDeviceStateString (Buffer, 5, StrSizePtr);
  EXPECT_EQ (Status, EFI_BUFFER_TOO_SMALL);
}

/**
  Test GetInsecureDeviceStateString function with one insecure state

  The tested function calls GetInsecureDeviceStateSetting to get the FixedAtBuild PCD
  defined in MdeModulePkg.dec
**/
TEST_F (DeviceStateBitmaskIsSetTest, GetInsecureDeviceStateStringSuccessEmpty) {
  CHAR8  ExpectedString1[MAX_INSECURE_DEVICE_STATE_STRING_SIZE] = "";

  EXPECT_CALL (
    PcdLibMock,
    LibPcdGet32
    )
    .WillOnce (
       Return (CurrentDeviceStateMock)
       );

  Status = GetInsecureDeviceStateString (Buffer, MAX_INSECURE_DEVICE_STATE_STRING_SIZE, StrSizePtr);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (*StrSizePtr, AsciiStrnSizeS (ExpectedString1, MAX_INSECURE_DEVICE_STATE_STRING_SIZE));
}

/**
  Test GetInsecureDeviceStateString function with one insecure state

  The tested function calls GetInsecureDeviceStateSetting to get the FixedAtBuild PCD
  defined in MdeModulePkg.dec
**/
TEST_F (DeviceStateBitmaskIsSetTest, GetInsecureDeviceStateStringSuccess1) {
  CHAR8  ExpectedString1[MAX_INSECURE_DEVICE_STATE_STRING_SIZE] = DEVICE_STATE_SOURCE_DEBUG_ENABLED_STRING;

  CurrentDeviceStateMock |= DEVICE_STATE_SOURCE_DEBUG_ENABLED;

  EXPECT_CALL (
    PcdLibMock,
    LibPcdGet32
    )
    .WillOnce (
       Return (CurrentDeviceStateMock)
       );

  Status = GetInsecureDeviceStateString (Buffer, MAX_INSECURE_DEVICE_STATE_STRING_SIZE, StrSizePtr);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (*StrSizePtr, AsciiStrnSizeS (ExpectedString1, MAX_INSECURE_DEVICE_STATE_STRING_SIZE));
}

/**
  Test GetInsecureDeviceStateString function with two insecure states

  The tested function calls GetInsecureDeviceStateSetting to get the FixedAtBuild PCD
  defined in MdeModulePkg.dec
**/
TEST_F (DeviceStateBitmaskIsSetTest, GetInsecureDeviceStateStringSuccess2) {
  CHAR8  ExpectedString2[MAX_INSECURE_DEVICE_STATE_STRING_SIZE] = "SOURCE_DEBUG_ENABLED UNIT_TEST_MODE ";

  CurrentDeviceStateMock |= DEVICE_STATE_SOURCE_DEBUG_ENABLED;
  CurrentDeviceStateMock |= DEVICE_STATE_UNIT_TEST_MODE;

  EXPECT_CALL (
    PcdLibMock,
    LibPcdGet32
    )
    .WillOnce (
       Return (CurrentDeviceStateMock)
       );

  Status = GetInsecureDeviceStateString (Buffer, MAX_INSECURE_DEVICE_STATE_STRING_SIZE, StrSizePtr);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (*StrSizePtr, AsciiStrnSizeS (ExpectedString2, MAX_INSECURE_DEVICE_STATE_STRING_SIZE));
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
