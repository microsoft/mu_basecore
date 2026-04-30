/** @file
  TLS Configuration API Tests.

  Verify TLS Set* configuration functions for verification mode, hostname
  verification, Server Name Indication, session ID, signature algorithms,
  compression, and security level.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestTlsLib.h"

/**
  Helper: Create a TLS 1.2 client context + connection.

  @param[out] TlsCtx  Receives the created TLS context.
  @param[out] Tls     Receives the created TLS connection.

  @retval EFI_SUCCESS  Context and connection created successfully.
**/
STATIC
EFI_STATUS
CreateTls12Client (
  OUT VOID  **TlsCtx,
  OUT VOID  **Tls
  )
{
  EFI_STATUS  Status;

  *TlsCtx = TlsCtxNew (3, 3);
  if (*TlsCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Tls = TlsNew (*TlsCtx);
  if (*Tls == NULL) {
    TlsCtxFree (*TlsCtx);
    *TlsCtx = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  Status = TlsSetConnectionEnd (*Tls, FALSE);
  if (EFI_ERROR (Status)) {
    TlsFree (*Tls);
    TlsCtxFree (*TlsCtx);
    *Tls    = NULL;
    *TlsCtx = NULL;
    return Status;
  }

  return EFI_SUCCESS;
}

//
// ============== TlsSetVerify / TlsGetVerify Tests ==============
//

/**
  Test that TlsSetVerify can set peer verification mode and TlsGetVerify
  returns the value that was set.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetVerifyPeer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID    *TlsCtx;
  VOID    *Tls;
  UINT32  VerifyMode;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  //
  // EFI_TLS_VERIFY_PEER = 0x1
  //
  TlsSetVerify (Tls, 0x1);
  VerifyMode = TlsGetVerify (Tls);
  UT_ASSERT_EQUAL (VerifyMode, 0x1);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test that TlsSetVerify can set verification to NONE.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetVerifyNone (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID    *TlsCtx;
  VOID    *Tls;
  UINT32  VerifyMode;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  //
  // EFI_TLS_VERIFY_NONE = 0x0
  //
  TlsSetVerify (Tls, 0x0);
  VerifyMode = TlsGetVerify (Tls);
  UT_ASSERT_EQUAL (VerifyMode, 0x0);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsSetVerifyHost Tests ==============
//

/**
  Test TlsSetVerifyHost with a valid hostname.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetVerifyHostValid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  Status = TlsSetVerifyHost (Tls, 0, "www.example.com");
  UT_ASSERT_NOT_EFI_ERROR (Status);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test TlsSetVerifyHost with a NULL hostname returns error.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetVerifyHostNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  Status = TlsSetVerifyHost (Tls, 0, NULL);
  UT_ASSERT_TRUE (EFI_ERROR (Status));

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsSetServerName Tests ==============
//

/**
  Test TlsSetServerName with a valid hostname for SNI.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetServerNameValid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  Status = TlsSetServerName (Tls, TlsCtx, "www.example.com");
  UT_ASSERT_NOT_EFI_ERROR (Status);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsSetSessionId Tests ==============
//

/**
  Test TlsSetSessionId with a valid session ID.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetSessionIdValid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINT8       SessionId[32];

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  //
  // Fill with a pattern for a 32-byte session ID.
  //
  SetMem (SessionId, sizeof (SessionId), 0xAB);
  Status = TlsSetSessionId (Tls, SessionId, (UINT16)sizeof (SessionId));

  DEBUG ((DEBUG_INFO, "  TlsSetSessionId(32 bytes) .. %r\n", Status));

  //
  // Session ID setting may return EFI_UNSUPPORTED if no session is available
  // to resume. This test documents the behavior.
  //
  UT_ASSERT_TRUE (!EFI_ERROR (Status) || (Status == EFI_UNSUPPORTED));

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsSetSignatureAlgoList Tests ==============
//

/**
  Test TlsSetSignatureAlgoList with RSA+SHA256 signature algorithm.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetSignatureAlgoListValid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

  //
  // TlsSetSignatureAlgoList expects: Data[0] = length of remaining bytes,
  // followed by pairs of (hash_algo, sig_algo) per RFC 5246.
  // SHA-256 (0x04) + RSA (0x01)
  //
  UINT8  SigAlgos[] = { 0x02, 0x04, 0x01 };

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  Status = TlsSetSignatureAlgoList (Tls, SigAlgos, sizeof (SigAlgos));
  UT_ASSERT_NOT_EFI_ERROR (Status);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test TlsSetSignatureAlgoList with multiple signature algorithm pairs.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetSignatureAlgoListMultiple (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

  //
  // TlsSetSignatureAlgoList expects: Data[0] = length of remaining bytes,
  // followed by pairs of (hash_algo, sig_algo) per RFC 5246.
  // Multiple pairs: SHA-256+RSA, SHA-384+RSA, SHA-256+ECDSA
  //
  UINT8  SigAlgos[] = {
    0x06,        // Length of remaining data (3 pairs * 2 bytes)
    0x04, 0x01,  // SHA-256 + RSA
    0x05, 0x01,  // SHA-384 + RSA
    0x04, 0x03,  // SHA-256 + ECDSA
  };

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  Status = TlsSetSignatureAlgoList (Tls, SigAlgos, sizeof (SigAlgos));
  UT_ASSERT_NOT_EFI_ERROR (Status);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsSetCompressionMethod Tests ==============
//

/**
  Test TlsSetCompressionMethod returns EFI_UNSUPPORTED.

  TLS compression is disabled in firmware builds for security reasons
  (CRIME attack). This test validates that the API correctly rejects
  compression requests.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetCompressionMethodUnsupported (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  TlsInitialize ();

  //
  // CompMethod 1 = DEFLATE. Should be unsupported.
  //
  Status = TlsSetCompressionMethod (1);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_UNSUPPORTED);

  return UNIT_TEST_PASSED;
}

/**
  Test TlsSetCompressionMethod with NULL (no compression).
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetCompressionMethodNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  TlsInitialize ();

  //
  // CompMethod 0 = NULL (no compression). This may or may not succeed
  // depending on the implementation. Document the behavior.
  //
  Status = TlsSetCompressionMethod (0);
  DEBUG ((DEBUG_INFO, "  TlsSetCompressionMethod(NULL) .. %r\n", Status));

  //
  // Both SUCCESS and UNSUPPORTED are valid outcomes for no-compression.
  //
  UT_ASSERT_TRUE (!EFI_ERROR (Status) || (Status == EFI_UNSUPPORTED));

  return UNIT_TEST_PASSED;
}

//
// ============== TlsSetSecurityLevel Tests ==============
//

/**
  Test TlsSetSecurityLevel with valid security levels.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetSecurityLevelValid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

  TlsInitialize ();

  DEBUG ((DEBUG_INFO, "\n[TLS Security Level Report]\n"));

  for (UINT8 Level = 0; Level <= 5; Level++) {
    UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

    Status = TlsSetSecurityLevel (Tls, Level);
    DEBUG (
      (
       DEBUG_INFO,
       "  Security Level %u .. %a\n",
       Level,
       EFI_ERROR (Status) ? "UNSUPPORTED" : "OK"
      )
      );

    TlsFree (Tls);
    TlsCtxFree (TlsCtx);
  }

  DEBUG ((DEBUG_INFO, "\n"));

  //
  // Level 2 (the OpenSSL 3.x default) must be accepted.
  //
  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));
  Status = TlsSetSecurityLevel (Tls, 2);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsShutdown Tests ==============
//

/**
  Test TlsShutdown on a fresh connection (no handshake performed).
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsShutdownFreshConnection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  Status = TlsShutdown (Tls);
  DEBUG ((DEBUG_INFO, "  TlsShutdown(fresh) .. %r\n", Status));

  //
  // Shutdown on a connection that never handshook is valid — documents behavior.
  //
  UT_ASSERT_TRUE (!EFI_ERROR (Status) || (Status == EFI_PROTOCOL_ERROR));

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsInHandshake Tests ==============
//

/**
  Test TlsInHandshake on a fresh connection returns TRUE (handshake not finished).
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsInHandshakeFreshConnection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *TlsCtx;
  VOID     *Tls;
  BOOLEAN  InHandshake;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  //
  // A fresh connection has not finished the handshake, so TlsInHandshake
  // should return TRUE (handshake is still in progress / not finished).
  //
  InHandshake = TlsInHandshake (Tls);
  UT_ASSERT_TRUE (InHandshake);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mTlsConfigTest[] = {
  //
  // Description--------------------------------------Class--------------------------------------Func--------------------------------------PreReq--CleanUp--Context
  //
  { "TlsSetVerify peer mode",                      "CryptoPkg.TlsLib.Config", TestTlsSetVerifyPeer,                   NULL, NULL, NULL },
  { "TlsSetVerify none mode",                      "CryptoPkg.TlsLib.Config", TestTlsSetVerifyNone,                   NULL, NULL, NULL },
  { "TlsSetVerifyHost valid hostname",             "CryptoPkg.TlsLib.Config", TestTlsSetVerifyHostValid,              NULL, NULL, NULL },
  { "TlsSetVerifyHost NULL hostname",              "CryptoPkg.TlsLib.Config", TestTlsSetVerifyHostNull,               NULL, NULL, NULL },
  { "TlsSetServerName valid SNI",                  "CryptoPkg.TlsLib.Config", TestTlsSetServerNameValid,              NULL, NULL, NULL },
  { "TlsSetSessionId valid ID",                    "CryptoPkg.TlsLib.Config", TestTlsSetSessionIdValid,               NULL, NULL, NULL },
  { "TlsSetSignatureAlgoList single",              "CryptoPkg.TlsLib.Config", TestTlsSetSignatureAlgoListValid,       NULL, NULL, NULL },
  { "TlsSetSignatureAlgoList multiple",            "CryptoPkg.TlsLib.Config", TestTlsSetSignatureAlgoListMultiple,    NULL, NULL, NULL },
  { "TlsSetCompressionMethod DEFLATE unsupported", "CryptoPkg.TlsLib.Config", TestTlsSetCompressionMethodUnsupported, NULL, NULL, NULL },
  { "TlsSetCompressionMethod NULL",                "CryptoPkg.TlsLib.Config", TestTlsSetCompressionMethodNull,        NULL, NULL, NULL },
  { "TlsSetSecurityLevel enumeration",             "CryptoPkg.TlsLib.Config", TestTlsSetSecurityLevelValid,           NULL, NULL, NULL },
  { "TlsShutdown on fresh connection",             "CryptoPkg.TlsLib.Config", TestTlsShutdownFreshConnection,         NULL, NULL, NULL },
  { "TlsInHandshake on fresh connection",          "CryptoPkg.TlsLib.Config", TestTlsInHandshakeFreshConnection,      NULL, NULL, NULL },
};

UINTN  mTlsConfigTestNum = ARRAY_SIZE (mTlsConfigTest);
