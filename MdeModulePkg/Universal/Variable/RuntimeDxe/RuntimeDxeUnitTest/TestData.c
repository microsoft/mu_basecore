/** @file
Test data. May be auto-generated.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariableRuntimeDxeUnitTest.h"

#define     TEST_GUID_1  { 0x9cd0c159, 0x4c9f, 0x49ca, { 0xa1, 0xe9, 0x3c, 0x38, 0xce, 0x57, 0x93, 0x87 } } // {9CD0C159-4C9F-49CA-A1E9-3C38CE579387}

TEST_VARIABLE_HEADER  TestVarA = {
  "TestVarA",
  L"TestVarA",
  TEST_GUID_1,
  VARIABLE_ATTRIBUTE_NV_BS_RT,
  VAR_TYPE_STANDARD,
  "DEADBEEF",
  DATA_ENC_HEX
};

TEST_VARIABLE_HEADER  TestVarB = {
  "TestVarB",
  L"TestVarB",
  TEST_GUID_1,
  VARIABLE_ATTRIBUTE_NV_BS,
  VAR_TYPE_STANDARD,
  "FEEDF00D",
  DATA_ENC_HEX
};

TEST_VARIABLE_HEADER  *mGlobalTestVarDb[] = {
  &TestVarA,
  &TestVarB
};
UINT32                mGlobalTestVarDbCount = ARRAY_SIZE (mGlobalTestVarDb);
