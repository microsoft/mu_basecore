/** @file
  Integration tests for DxeImageVerificationHandler.

  Test cases in this file dispatch through RunVerificationScenario
  declared in ScenarioHarness.h. The harness implementation, including
  every mock and input builder, lives in ScenarioHarness.cpp so this
  file stays limited to test bodies and the gtest entry point.

  Copyright (c) 2025, Yandex. All rights reserved.
  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
