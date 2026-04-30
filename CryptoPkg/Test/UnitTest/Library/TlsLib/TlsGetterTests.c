/** @file
  TLS Getter / Query Function Tests.

  Verify TLS Get* query functions that retrieve connection state:
  TlsGetVersion, TlsGetConnectionEnd, TlsGetCurrentCipher,
  TlsGetCurrentCompressionId, TlsGetVerify, TlsGetSessionId,
  TlsGetClientRandom, TlsGetServerRandom, TlsGetHostPublicCert,
  TlsGetHostPrivateKey.

  Most getters require an active connection but can be tested on a
  freshly created (pre-handshake) TLS connection to verify they
  return sensible defaults or expected error codes.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestTlsLib.h"

/**
  Helper: Create a TLS 1.2 client context + connection.
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
// ============== TlsGetVersion Tests ==============
//

/**
  Test TlsGetVersion on a pre-handshake connection.

  Before handshake completion, SSL_version() returns the maximum
  supported version or 0x0000 depending on OpenSSL configuration.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetVersionPreHandshake (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID    *TlsCtx;
  VOID    *Tls;
  UINT16  Version;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  Version = TlsGetVersion (Tls);
  DEBUG ((DEBUG_INFO, "  TlsGetVersion: 0x%04x\n", Version));

  //
  // TLS version before handshake returns the maximum supported version.
  // OpenSSL 3.x with TLS 1.3 support returns 0x0304 (TLS 1.3).
  // With TLS 1.2 only it returns 0x0303. Either is acceptable.
  //
  UT_ASSERT_TRUE ((Version == 0x0303) || (Version == 0x0304) || (Version == 0x0000));

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsGetConnectionEnd Tests ==============
//

/**
  Test TlsGetConnectionEnd returns client mode after SetConnectionEnd(FALSE).
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetConnectionEndClient (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID   *TlsCtx;
  VOID   *Tls;
  UINT8  ConnectionEnd;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  ConnectionEnd = TlsGetConnectionEnd (Tls);
  DEBUG ((DEBUG_INFO, "  TlsGetConnectionEnd: %u (0=client, 1=server)\n", ConnectionEnd));

  //
  // We set client mode (FALSE), so ConnectionEnd should indicate client.
  // TlsGetConnectionEnd wraps SSL_is_server() which returns 0 for client.
  //
  UT_ASSERT_EQUAL (ConnectionEnd, 0);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsGetCurrentCipher Tests ==============
//

/**
  Test TlsGetCurrentCipher on a pre-handshake connection.

  Before handshake completion, no cipher is negotiated. The function
  should return an error or a zero cipher ID.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetCurrentCipherPreHandshake (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINT16      CipherId;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  CipherId = 0;
  Status   = TlsGetCurrentCipher (Tls, &CipherId);
  DEBUG ((DEBUG_INFO, "  TlsGetCurrentCipher(pre-handshake): %r, CipherId=0x%04x\n", Status, CipherId));

  //
  // Before handshake, expect either an error (no cipher negotiated) or
  // success with CipherId == 0.
  //
  if (!EFI_ERROR (Status)) {
    UT_ASSERT_EQUAL (CipherId, 0);
  }

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsGetCurrentCompressionId Tests ==============
//

/**
  Test TlsGetCurrentCompressionId on a pre-handshake connection.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetCurrentCompressionIdPreHandshake (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINT8       CompressionId;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  CompressionId = 0xFF;
  Status        = TlsGetCurrentCompressionId (Tls, &CompressionId);
  DEBUG ((DEBUG_INFO, "  TlsGetCurrentCompressionId: %r, Id=%u\n", Status, CompressionId));

  //
  // Compression is disabled in firmware TLS. Expect UNSUPPORTED or
  // success with NULL compression (id 0).
  //
  if (!EFI_ERROR (Status)) {
    UT_ASSERT_EQUAL (CompressionId, 0);
  } else {
    UT_ASSERT_TRUE (
      (Status == EFI_UNSUPPORTED) || (Status == EFI_ABORTED)
      );
  }

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsGetVerify Tests ==============
//

/**
  Test TlsGetVerify returns default verification mode.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetVerifyDefault (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID    *TlsCtx;
  VOID    *Tls;
  UINT32  VerifyMode;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  VerifyMode = TlsGetVerify (Tls);
  DEBUG ((DEBUG_INFO, "  TlsGetVerify(default): 0x%x\n", VerifyMode));

  //
  // Default verify mode is implementation-defined. Just verify
  // the function returns without crashing.
  //

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test TlsGetVerify reflects what was set via TlsSetVerify.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetVerifyAfterSet (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID    *TlsCtx;
  VOID    *Tls;
  UINT32  VerifyMode;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  //
  // Set peer verification mode.
  //
  TlsSetVerify (Tls, 0x1);
  VerifyMode = TlsGetVerify (Tls);
  UT_ASSERT_EQUAL (VerifyMode, 0x1);

  //
  // Set no verification.
  //
  TlsSetVerify (Tls, 0x0);
  VerifyMode = TlsGetVerify (Tls);
  UT_ASSERT_EQUAL (VerifyMode, 0x0);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsGetSessionId Tests ==============
//

/**
  Test TlsGetSessionId on a pre-handshake connection.

  Before handshake, there is no session ID. The function should
  return EFI_UNSUPPORTED or a zero-length session ID.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetSessionIdPreHandshake (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINT8       SessionId[32];
  UINT16      SessionIdLen;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  SessionIdLen = sizeof (SessionId);
  Status       = TlsGetSessionId (Tls, SessionId, &SessionIdLen);
  DEBUG ((DEBUG_INFO, "  TlsGetSessionId(pre-handshake): %r, Len=%u\n", Status, SessionIdLen));

  //
  // Pre-handshake: expect either UNSUPPORTED (no session) or
  // success with zero-length session ID.
  //
  if (!EFI_ERROR (Status)) {
    UT_ASSERT_EQUAL (SessionIdLen, 0);
  } else {
    UT_ASSERT_STATUS_EQUAL (Status, EFI_UNSUPPORTED);
  }

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsGetClientRandom / TlsGetServerRandom Tests ==============
//

/**
  Test TlsGetClientRandom on a pre-handshake connection.

  Before handshake, client random may be all zeros or unset.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetClientRandomPreHandshake (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID   *TlsCtx;
  VOID   *Tls;
  UINT8  ClientRandom[32];

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  SetMem (ClientRandom, sizeof (ClientRandom), 0xFF);
  TlsGetClientRandom (Tls, ClientRandom);

  //
  // Pre-handshake: client random should be zeroed (no handshake data yet).
  //
  DEBUG (
    (DEBUG_INFO, "  TlsGetClientRandom(pre-handshake): first 4 bytes = %02x %02x %02x %02x\n",
     ClientRandom[0], ClientRandom[1], ClientRandom[2], ClientRandom[3])
    );

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test TlsGetServerRandom on a pre-handshake connection.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetServerRandomPreHandshake (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID   *TlsCtx;
  VOID   *Tls;
  UINT8  ServerRandom[32];

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  SetMem (ServerRandom, sizeof (ServerRandom), 0xFF);
  TlsGetServerRandom (Tls, ServerRandom);

  DEBUG (
    (DEBUG_INFO, "  TlsGetServerRandom(pre-handshake): first 4 bytes = %02x %02x %02x %02x\n",
     ServerRandom[0], ServerRandom[1], ServerRandom[2], ServerRandom[3])
    );

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsGetKeyMaterial Tests ==============
//

/**
  Test TlsGetKeyMaterial on a pre-handshake connection.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetKeyMaterialPreHandshake (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINT8       KeyMaterial[48];

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  ZeroMem (KeyMaterial, sizeof (KeyMaterial));
  Status = TlsGetKeyMaterial (Tls, KeyMaterial);
  DEBUG ((DEBUG_INFO, "  TlsGetKeyMaterial(pre-handshake): %r\n", Status));

  //
  // Before handshake, there is no master secret. Expect UNSUPPORTED
  // or success with zero material.
  //
  UT_ASSERT_TRUE (!EFI_ERROR (Status) || (Status == EFI_UNSUPPORTED));

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

//
// ============== TlsGetHostPublicCert / TlsGetHostPrivateKey Tests ==============
//

/**
  Test TlsGetHostPublicCert when no cert is loaded.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetHostPublicCertNone (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINTN       DataSize;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  DataSize = 0;
  Status   = TlsGetHostPublicCert (Tls, NULL, &DataSize);
  DEBUG ((DEBUG_INFO, "  TlsGetHostPublicCert(none loaded): %r\n", Status));

  //
  // No cert loaded — expect NOT_FOUND or UNSUPPORTED.
  //
  UT_ASSERT_TRUE (EFI_ERROR (Status));

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test TlsGetHostPrivateKey when no key is loaded.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsGetHostPrivateKeyNone (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINTN       DataSize;

  TlsInitialize ();

  UT_ASSERT_NOT_EFI_ERROR (CreateTls12Client (&TlsCtx, &Tls));

  DataSize = 0;
  Status   = TlsGetHostPrivateKey (Tls, NULL, &DataSize);
  DEBUG ((DEBUG_INFO, "  TlsGetHostPrivateKey(none loaded): %r\n", Status));

  //
  // No key loaded — expect UNSUPPORTED or NOT_FOUND.
  //
  UT_ASSERT_TRUE (EFI_ERROR (Status));

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mTlsGetterTest[] = {
  //
  // Description--------------------------------------Class--------------------------------------Func------------------------------------------PreReq--CleanUp--Context
  //
  { "TlsGetVersion pre-handshake",              "CryptoPkg.TlsLib.Getter", TestTlsGetVersionPreHandshake,              NULL, NULL, NULL },
  { "TlsGetConnectionEnd client",               "CryptoPkg.TlsLib.Getter", TestTlsGetConnectionEndClient,              NULL, NULL, NULL },
  { "TlsGetCurrentCipher pre-handshake",        "CryptoPkg.TlsLib.Getter", TestTlsGetCurrentCipherPreHandshake,        NULL, NULL, NULL },
  { "TlsGetCurrentCompressionId pre-handshake", "CryptoPkg.TlsLib.Getter", TestTlsGetCurrentCompressionIdPreHandshake, NULL, NULL, NULL },
  { "TlsGetVerify default mode",                "CryptoPkg.TlsLib.Getter", TestTlsGetVerifyDefault,                    NULL, NULL, NULL },
  { "TlsGetVerify after TlsSetVerify",          "CryptoPkg.TlsLib.Getter", TestTlsGetVerifyAfterSet,                   NULL, NULL, NULL },
  { "TlsGetSessionId pre-handshake",            "CryptoPkg.TlsLib.Getter", TestTlsGetSessionIdPreHandshake,            NULL, NULL, NULL },
  { "TlsGetClientRandom pre-handshake",         "CryptoPkg.TlsLib.Getter", TestTlsGetClientRandomPreHandshake,         NULL, NULL, NULL },
  { "TlsGetServerRandom pre-handshake",         "CryptoPkg.TlsLib.Getter", TestTlsGetServerRandomPreHandshake,         NULL, NULL, NULL },
  { "TlsGetKeyMaterial pre-handshake",          "CryptoPkg.TlsLib.Getter", TestTlsGetKeyMaterialPreHandshake,          NULL, NULL, NULL },
  { "TlsGetHostPublicCert no cert loaded",      "CryptoPkg.TlsLib.Getter", TestTlsGetHostPublicCertNone,               NULL, NULL, NULL },
  { "TlsGetHostPrivateKey no key loaded",       "CryptoPkg.TlsLib.Getter", TestTlsGetHostPrivateKeyNone,               NULL, NULL, NULL },
};

UINTN  mTlsGetterTestNum = ARRAY_SIZE (mTlsGetterTest);
