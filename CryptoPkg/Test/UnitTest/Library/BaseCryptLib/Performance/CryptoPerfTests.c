/** @file
  Performance test suite registration for BaseCryptLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptoPerfLib.h"

PERF_SUITE_DESC  mPerfSuiteDesc[] = {
  //
  // Title--------------------------------Package------------------------Sup--Tdn--TestNum-----------------TestDesc
  //
  { "Hash performance tests", "CryptoPkg.BaseCryptLib.Perf", NULL, NULL, &mHashPerfTestNum, mHashPerfTest },
  { "AES performance tests",  "CryptoPkg.BaseCryptLib.Perf", NULL, NULL, &mAesPerfTestNum,  mAesPerfTest  },
  { "HMAC performance tests", "CryptoPkg.BaseCryptLib.Perf", NULL, NULL, &mHmacPerfTestNum, mHmacPerfTest },
  { "RSA performance tests",  "CryptoPkg.BaseCryptLib.Perf", NULL, NULL, &mRsaPerfTestNum,  mRsaPerfTest  },
  { "EC performance tests",   "CryptoPkg.BaseCryptLib.Perf", NULL, NULL, &mEcPerfTestNum,   mEcPerfTest   },
};

EFI_STATUS
EFIAPI
CreatePerfTest (
  IN     CHAR8                       *UnitTestName,
  IN     CHAR8                       *UnitTestVersion,
  IN OUT UNIT_TEST_FRAMEWORK_HANDLE  *Framework
  )
{
  EFI_STATUS  Status;
  UINTN       SuiteIndex;
  UINTN       TestIndex;

  if ((Framework == NULL) || (UnitTestVersion == NULL) || (UnitTestName == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = InitUnitTestFramework (Framework, UnitTestName, gEfiCallerBaseName, UnitTestVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    return Status;
  }

  for (SuiteIndex = 0; SuiteIndex < ARRAY_SIZE (mPerfSuiteDesc); SuiteIndex++) {
    UNIT_TEST_SUITE_HANDLE  Suite = NULL;
    Status = CreateUnitTestSuite (
               &Suite,
               *Framework,
               mPerfSuiteDesc[SuiteIndex].Title,
               mPerfSuiteDesc[SuiteIndex].Package,
               mPerfSuiteDesc[SuiteIndex].Sup,
               mPerfSuiteDesc[SuiteIndex].Tdn
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    for (TestIndex = 0; TestIndex < *mPerfSuiteDesc[SuiteIndex].TestNum; TestIndex++) {
      AddTestCase (
        Suite,
        mPerfSuiteDesc[SuiteIndex].TestDesc[TestIndex].Description,
        mPerfSuiteDesc[SuiteIndex].TestDesc[TestIndex].ClassName,
        mPerfSuiteDesc[SuiteIndex].TestDesc[TestIndex].Func,
        mPerfSuiteDesc[SuiteIndex].TestDesc[TestIndex].PreReq,
        mPerfSuiteDesc[SuiteIndex].TestDesc[TestIndex].CleanUp,
        mPerfSuiteDesc[SuiteIndex].TestDesc[TestIndex].Context
        );
    }
  }

  return EFI_SUCCESS;
}
