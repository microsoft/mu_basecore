/** @file
  TLS Library Unit Test suite definitions.

  Shared between Host and UEFI Shell entry points.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestTlsLib.h"

SUITE_DESC  mSuiteDesc[] = {
  //
  // Title---------------------------------Package-----------------------Sup--Tdn----TestNum---------------------------TestDesc
  //
  { "TLS function pointer tests",         "CryptoPkg.TlsLib", NULL, NULL, &mTlsFunctionPointerTestNum, mTlsFunctionPointerTest },
  { "TLS context lifecycle tests",        "CryptoPkg.TlsLib", NULL, NULL, &mTlsContextTestNum,         mTlsContextTest         },
  { "TLS cipher suite enumeration tests", "CryptoPkg.TlsLib", NULL, NULL, &mTlsCipherTestNum,          mTlsCipherTest          },
  { "TLS configuration tests",            "CryptoPkg.TlsLib", NULL, NULL, &mTlsConfigTestNum,          mTlsConfigTest          },
  { "TLS certificate management tests",   "CryptoPkg.TlsLib", NULL, NULL, &mTlsCertificateTestNum,     mTlsCertificateTest     },
  { "TLS getter / query function tests",  "CryptoPkg.TlsLib", NULL, NULL, &mTlsGetterTestNum,          mTlsGetterTest          },
};

EFI_STATUS
EFIAPI
CreateUnitTest (
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
    goto Done;
  }

  for (SuiteIndex = 0; SuiteIndex < ARRAY_SIZE (mSuiteDesc); SuiteIndex++) {
    UNIT_TEST_SUITE_HANDLE  Suite = NULL;
    Status = CreateUnitTestSuite (
               &Suite,
               *Framework,
               mSuiteDesc[SuiteIndex].Title,
               mSuiteDesc[SuiteIndex].Package,
               mSuiteDesc[SuiteIndex].Sup,
               mSuiteDesc[SuiteIndex].Tdn
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    for (TestIndex = 0; TestIndex < *mSuiteDesc[SuiteIndex].TestNum; TestIndex++) {
      AddTestCase (
        Suite,
        (mSuiteDesc[SuiteIndex].TestDesc + TestIndex)->Description,
        (mSuiteDesc[SuiteIndex].TestDesc + TestIndex)->ClassName,
        (mSuiteDesc[SuiteIndex].TestDesc + TestIndex)->Func,
        (mSuiteDesc[SuiteIndex].TestDesc + TestIndex)->PreReq,
        (mSuiteDesc[SuiteIndex].TestDesc + TestIndex)->CleanUp,
        (mSuiteDesc[SuiteIndex].TestDesc + TestIndex)->Context
        );
    }
  }

Done:
  return Status;
}
