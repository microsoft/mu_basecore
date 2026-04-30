/** @file
  Header for TLS Library Host-based Unit Tests.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/TlsLib.h>

#define UNIT_TEST_NAME     "TLS Library Host Unit Test"
#define UNIT_TEST_VERSION  "1.0"

typedef struct {
  CHAR8                     *Description;
  CHAR8                     *ClassName;
  UNIT_TEST_FUNCTION        Func;
  UNIT_TEST_PREREQUISITE    PreReq;
  UNIT_TEST_CLEANUP         CleanUp;
  UNIT_TEST_CONTEXT         Context;
} TEST_DESC;

typedef struct {
  CHAR8                       *Title;
  CHAR8                       *Package;
  UNIT_TEST_SUITE_SETUP       Sup;
  UNIT_TEST_SUITE_TEARDOWN    Tdn;
  UINTN                       *TestNum;
  TEST_DESC                   *TestDesc;
} SUITE_DESC;

//
// TLS Function Pointer Tests (Suite 1)
//
extern UINTN      mTlsFunctionPointerTestNum;
extern TEST_DESC  mTlsFunctionPointerTest[];

//
// TLS Context Lifecycle Tests (Suite 2)
//
extern UINTN      mTlsContextTestNum;
extern TEST_DESC  mTlsContextTest[];

//
// TLS Cipher Suite Enumeration Tests (Suite 3)
//
extern UINTN      mTlsCipherTestNum;
extern TEST_DESC  mTlsCipherTest[];

//
// TLS Configuration Tests (Suite 4)
//
extern UINTN      mTlsConfigTestNum;
extern TEST_DESC  mTlsConfigTest[];

//
// TLS Certificate Management Tests (Suite 5)
//
extern UINTN      mTlsCertificateTestNum;
extern TEST_DESC  mTlsCertificateTest[];

//
// TLS Getter / Query Function Tests (Suite 6)
//
extern UINTN      mTlsGetterTestNum;
extern TEST_DESC  mTlsGetterTest[];

/** Creates and populates the unit test framework with all TLS test suites. */
EFI_STATUS
EFIAPI
CreateUnitTest (
  IN     CHAR8                       *UnitTestName,
  IN     CHAR8                       *UnitTestVersion,
  IN OUT UNIT_TEST_FRAMEWORK_HANDLE  *Framework
  );
