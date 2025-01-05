/** @file
  Application for Diffie-Hellman Primitives Validation.

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"
#include "TlsLib.h"

UNIT_TEST_STATUS
EFIAPI
TestVerifyTlsPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  if (!PcdGetBool (PcdCryptoServiceTlsInitialize) || !PcdGetBool (PcdCryptoServiceTlsCtxNew) || !PcdGetBool (PcdCryptoServiceTlsCtxFree)) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyTlsCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{ 
  // TODO: Inbal: Fill in free of needed buffers
}

/* Tests for init protocol */

UNIT_TEST_STATUS
EFIAPI
TestTlsInitialize (
  VOID
  )
{
  BOOLEAN Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTlsCreation31CtxNewFree (
  VOID
  )
{
  BOOLEAN Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);
  
  auto SslCtxObj = TlsCtxNew(3,1);
  UT_ASSERT_NOT_NULL(SslCtxObj);
  
  TlsFree(SslCtxObj);

  return UNIT_TEST_PASSED;
}




UNIT_TEST_STATUS
EFIAPI
TestVerifyTlsGenerateKey (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status = TRUE;

  return Status;
}

TEST_DESC  mTlsTest[] = {
  //
  // -----Description--------------------------------Class---------------------Function----------------Pre-----------------Post------------Context
  //
  { "TestVerifyTlsGenerateKey()", "CryptoPkg.BaseCryptLib.Tls", TestTlsInitialize, TestVerifyTlsPreReq, NULL, NULL},
  { "TestVerifyTlsGenerateKey()", "CryptoPkg.BaseCryptLib.Tls", TestTlsCreation31CtxNewFree, TestVerifyTlsPreReq, NULL, NULL},
};

UINTN  mTlsTestNum = ARRAY_SIZE (mTlsTest);
