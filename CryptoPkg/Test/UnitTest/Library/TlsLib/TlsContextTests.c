/** @file
  TLS Context Lifecycle Tests.

  Verify TLS context and connection creation/destruction for various
  TLS versions and connection endpoint configurations.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestTlsLib.h"

/**
  Test TLS context creation and free for TLS 1.2.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsCtxNewFreeTls12 (
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
  Test TLS connection creation as client endpoint.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetConnectionEndClient (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

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

  //
  // FALSE = client
  //
  Status = TlsSetConnectionEnd (Tls, FALSE);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test TLS connection creation as server endpoint.

  Note: Server mode may return EFI_UNSUPPORTED depending on the crypto
  provider configuration. This test documents the behavior rather than
  asserting success.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetConnectionEndServer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

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

  //
  // TRUE = server. Firmware is client-only — server mode must not be available.
  //
  Status = TlsSetConnectionEnd (Tls, TRUE);
  DEBUG ((DEBUG_INFO, "  TlsSetConnectionEnd(Server) .. %r\n", Status));

  //
  // Server endpoint is not needed for firmware TLS (client-only).
  // Assert that server mode is explicitly unsupported.
  //
  UT_ASSERT_STATUS_EQUAL (Status, EFI_UNSUPPORTED);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test TlsSetVersion for TLS 1.2.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetVersionTls12 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

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

  Status = TlsSetVersion (Tls, 3, 3);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mTlsContextTest[] = {
  //
  // Description---------------------------------Class--------------------------------------Func------------------------------PreReq--CleanUp--Context
  //
  { "TlsCtxNew/Free TLS 1.2",     "CryptoPkg.TlsLib.Context", TestTlsCtxNewFreeTls12,        NULL, NULL, NULL },
  { "TlsSetConnectionEnd client", "CryptoPkg.TlsLib.Context", TestTlsSetConnectionEndClient, NULL, NULL, NULL },
  { "TlsSetConnectionEnd server", "CryptoPkg.TlsLib.Context", TestTlsSetConnectionEndServer, NULL, NULL, NULL },
  { "TlsSetVersion TLS 1.2",      "CryptoPkg.TlsLib.Context", TestTlsSetVersionTls12,        NULL, NULL, NULL },
};

UINTN  mTlsContextTestNum = ARRAY_SIZE (mTlsContextTest);
