/** @file
  TLS Function Pointer Validation Tests.

  Verify that all TLS function pointers in BaseCryptLib/TlsLib are
  accessible (non-NULL) when linked, confirming TLS support is compiled in.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestTlsLib.h"

/**
  Test that TlsInitialize succeeds, indicating TLS support is present.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsInitialize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Result;

  Result = TlsInitialize ();
  UT_ASSERT_TRUE (Result);

  return UNIT_TEST_PASSED;
}

/**
  Test that TlsCtxNew returns a non-NULL context for TLS 1.2.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsCtxNewNotNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID  *TlsCtx;

  TlsInitialize ();

  //
  // TLS 1.2 = Major 3, Minor 3
  //
  TlsCtx = TlsCtxNew (3, 3);
  UT_ASSERT_NOT_NULL (TlsCtx);

  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test that TlsNew returns a non-NULL connection from a valid context.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsNewNotNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID  *TlsCtx;
  VOID  *Tls;

  TlsInitialize ();

  TlsCtx = TlsCtxNew (3, 3);
  if (TlsCtx == NULL) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  Tls = TlsNew (TlsCtx);
  if (Tls == NULL) {
    TlsCtxFree (TlsCtx);
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  UT_ASSERT_NOT_NULL (Tls);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mTlsFunctionPointerTest[] = {
  //
  // Description---------------------------Class-------------------------------------------------Func------------------------PreReq--CleanUp--Context
  //
  { "TlsInitialize succeeds",               "CryptoPkg.TlsLib.FunctionPointer", TestTlsInitialize,    NULL, NULL, NULL },
  { "TlsCtxNew returns non-NULL (TLS 1.2)", "CryptoPkg.TlsLib.FunctionPointer", TestTlsCtxNewNotNull, NULL, NULL, NULL },
  { "TlsNew returns non-NULL",              "CryptoPkg.TlsLib.FunctionPointer", TestTlsNewNotNull,    NULL, NULL, NULL },
};

UINTN  mTlsFunctionPointerTestNum = ARRAY_SIZE (mTlsFunctionPointerTest);
