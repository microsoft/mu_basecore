/** @file
  This is a unit test for RSA OAEP encrypt/decrypt.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"
#include <Library/TlsLib.h>

typedef void *TLS_OBJ;

// List of Ciphers as appears in TLS Cipher Suite Registry of the IANA
// https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml

// TODO: Verify order of bytes is correct in all cases (or use UINT8)
UINT16  mCipherId[] = {
  0xC030,                        // TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
  0xC02F,                        // TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
  0xC028,                        // TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384
  0xC027                         // TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
};
#define CIPHER_COUNT  (sizeof(mCipherId) / sizeof(mCipherId[0]))

// Note: Setting TLS 1.2 (Redefined to avoid dependency on MdePkg/Include/IndustryStandard/Tls1.h)
#define TLS12_PROTOCOL_VERSION_MAJOR  0x03
#define TLS12_PROTOCOL_VERSION_MINOR  0x03

#define EfiTlsClient  0

UNIT_TEST_STATUS
EFIAPI
TestVerifyTlsPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  // TODO: Flags to be removed with the refactoring of UEFI PCDs

  /*
  if (!PcdGetBool (PcdCryptoServiceTlsInitialize) || !PcdGetBool (PcdCryptoServiceTlsCtxNew) || !PcdGetBool (PcdCryptoServiceTlsCtxFree)) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }
  */

  return UNIT_TEST_PASSED;
}

// Some broken format
VOID
EFIAPI
TestVerifyTlsCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  // TODO: Fill in in case needed
}

UNIT_TEST_STATUS
EFIAPI
TestTsl12CreatCtxObjNewFree (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status = TlsInitialize ();

  UT_ASSERT_TRUE (Status);

  TLS_OBJ  SslCtxObj = TlsCtxNew (TLS12_PROTOCOL_VERSION_MAJOR, TLS12_PROTOCOL_VERSION_MINOR);

  UT_ASSERT_NOT_NULL (SslCtxObj);

  TLS_OBJ  TlsObj = TlsNew (SslCtxObj);

  UT_ASSERT_NOT_NULL (TlsObj);

  // Cleanup
  TlsFree (TlsObj);
  TlsCtxFree (SslCtxObj);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTsl12CreateConnection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  BOOLEAN     Result = FALSE;

  Result = TlsInitialize ();
  UT_ASSERT_TRUE (Result);

  TLS_OBJ  TlsCtx = TlsCtxNew (TLS12_PROTOCOL_VERSION_MAJOR, TLS12_PROTOCOL_VERSION_MINOR);

  UT_ASSERT_NOT_NULL (TlsCtx);

  TLS_OBJ  TlsConn = TlsNew (TlsCtx);

  UT_ASSERT_NOT_NULL (TlsConn);

  Status = TlsSetConnectionEnd (TlsConn, EfiTlsClient);
  UT_ASSERT_EQUAL (EFI_SUCCESS, Status);

  // Cleanup
  // NOTE: this is aligned with other tests, but will not be called if test fails
  TlsFree (TlsConn);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

// TODO: Check if we need to call other stages to establish connection
//       For example: Handshake, etc.

UNIT_TEST_STATUS
EFIAPI
TestTsl12VerifySetCipherList (
  IN
  UNIT_TEST_CONTEXT
  Context
  )
{
  UINT16      CipherId = 0;
  EFI_STATUS  Status   = EFI_SUCCESS;
  BOOLEAN     Result   = FALSE;

  Result = TlsInitialize ();
  UT_ASSERT_TRUE (Result);

  TLS_OBJ  TlsCtx = TlsCtxNew (TLS12_PROTOCOL_VERSION_MAJOR, TLS12_PROTOCOL_VERSION_MINOR);

  UT_ASSERT_NOT_NULL (TlsCtx);

  TLS_OBJ  TlsConn = TlsNew (TlsCtx);

  UT_ASSERT_NOT_NULL (TlsConn);

  Status = TlsSetConnectionEnd (TlsConn, EfiTlsClient);
  UT_ASSERT_EQUAL (EFI_SUCCESS, Status);

  Status = TlsSetCipherList (TlsConn, mCipherId, CIPHER_COUNT);
  UT_ASSERT_EQUAL (EFI_SUCCESS, Status);

  TlsGetCurrentCipher (TlsConn, &CipherId);
  UT_ASSERT_EQUAL (EFI_SUCCESS, Status);

  BOOLEAN  Found = FALSE;

  for (int i = 0; i < CIPHER_COUNT; i++) {
    if (mCipherId[i] == CipherId) {
      Found = TRUE;
      break;
    }
  }

  UT_ASSERT_TRUE (Found);

  // Cleanup
  // NOTE: this is aligned with other tests, but will not be called if test fails
  TlsFree (TlsConn);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTsl12GetCurrentCipher (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT16      CipherId = 0;
  EFI_STATUS  Status   = EFI_SUCCESS;
  BOOLEAN     Result   = FALSE;

  Result = TlsInitialize ();
  UT_ASSERT_TRUE (Result);

  TLS_OBJ  TlsCtx = TlsCtxNew (TLS12_PROTOCOL_VERSION_MAJOR, TLS12_PROTOCOL_VERSION_MINOR);

  UT_ASSERT_NOT_NULL (TlsCtx);

  TLS_OBJ  TlsConn = TlsNew (TlsCtx);

  UT_ASSERT_NOT_NULL (TlsConn);

  TlsGetCurrentCipher (TlsConn, &CipherId);
  UT_ASSERT_EQUAL (EFI_SUCCESS, Status);

  BOOLEAN  Found = FALSE;

  // Check if default config support ciphers
  for (int i = 0; i < CIPHER_COUNT; i++) {
    if (mCipherId[i] == CipherId) {
      Found = TRUE;
      break;
    }
  }

  UT_ASSERT_TRUE (Found);

  Status = TlsSetConnectionEnd (TlsConn, EfiTlsClient);
  UT_ASSERT_EQUAL (EFI_SUCCESS, Status);

  // Cleanup
  // NOTE: this is aligned with other tests, but will not be called if test fails
  TlsFree (TlsConn);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mTlsTest[] = {
  //
  // -----Description--------------------------------Class---------------------Function----------------Pre-----------------Post------------Context
  //
  { "TestTsl12CreatCtxObjNewFree()",  "CryptoPkg.BaseCryptLib.Tls", TestTsl12CreatCtxObjNewFree,  TestVerifyTlsPreReq, NULL, NULL },
  { "TestTsl12CreateConnection()",    "CryptoPkg.BaseCryptLib.Tls", TestTsl12CreateConnection,    TestVerifyTlsPreReq, NULL, NULL },
  { "TestTsl12VerifySetCipherList()", "CryptoPkg.BaseCryptLib.Tls", TestTsl12VerifySetCipherList, TestVerifyTlsPreReq, NULL, NULL },
  { "TestTsl12GetCurrentCipher()",    "CryptoPkg.BaseCryptLib.Tls", TestTsl12GetCurrentCipher,    TestVerifyTlsPreReq, NULL, NULL }
};

UINTN  mTlsTestNum = ARRAY_SIZE (mTlsTest);

// ~~~~ TODO: check if any of these tests are needed ~~~~

/*
UNIT_TEST_STATUS
EFIAPI
TestTlsHandleAlert (
  VOID
  )
{
  BOOLEAN Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);

  auto SslCtxObj = TlsCtxNew(3,1);
  UT_ASSERT_NOT_NULL(SslCtxObj);

  auto TlsObj = TlsNew(SslCtxObj);
  UT_ASSERT_NOT_NULL(TlsObj);

  Status = TlsHandleAlert(TlsObj, NULL, 0, NULL, NULL);
  UT_ASSERT_TRUE(Status);

  // Cleanup
  TlsFree(TlsObj);
  TlsCtxFree(SslCtxObj);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTlsCloseNotify (
  VOID
  )
{
  BOOLEAN Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);

  auto SslCtxObj = TlsCtxNew(3,1);
  UT_ASSERT_NOT_NULL(SslCtxObj);

  auto TlsObj = TlsNew(SslCtxObj);
  UT_ASSERT_NOT_NULL(TlsObj);

  Status = TlsCloseNotify(TlsObj, NULL, NULL);
  UT_ASSERT_TRUE(Status);

  // Cleanup
  TlsFree(TlsObj);
  TlsCtxFree(SslCtxObj);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTlsCtrlTrafficOut (
  VOID
  )
{
  BOOLEAN Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);

  auto SslCtxObj = TlsCtxNew(3,1);
  UT_ASSERT_NOT_NULL(SslCtxObj);

  auto TlsObj = TlsNew(SslCtxObj);
  UT_ASSERT_NOT_NULL(TlsObj);

  Status = TlsCtrlTrafficOut(TlsObj, NULL, 0);
  UT_ASSERT_TRUE(Status);

  // Cleanup
  TlsFree(TlsObj);
  TlsCtxFree(SslCtxObj);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTlsCtrlTrafficIn (
  VOID
  )
{
  BOOLEAN Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);

  auto SslCtxObj = TlsCtxNew(3,1);
  UT_ASSERT_NOT_NULL(SslCtxObj);

  auto TlsObj = TlsNew(SslCtxObj);
  UT_ASSERT_NOT_NULL(TlsObj);

  Status = TlsCtrlTrafficIn(TlsObj, NULL, 0);
  UT_ASSERT_TRUE(Status);

  // Cleanup
  TlsFree(TlsObj);
  TlsCtxFree(SslCtxObj);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTlsRead (
  VOID
  )
{
  BOOLEAN Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);

  auto SslCtxObj = TlsCtxNew(3,1);
  UT_ASSERT_NOT_NULL(SslCtxObj);

  auto TlsObj = TlsNew(SslCtxObj);
  UT_ASSERT_NOT_NULL(TlsObj);

  UINT8 Buffer[256];
  UINTN BufferSize = sizeof(Buffer);
  Status = TlsRead(TlsObj, Buffer, &BufferSize);
  UT_ASSERT_TRUE(Status);

  // Cleanup
  TlsFree(TlsObj);
  TlsCtxFree(SslCtxObj);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTlsWrite (
  VOID
  )
{
  BOOLEAN Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);

  auto SslCtxObj = TlsCtxNew(3,1);
  UT_ASSERT_NOT_NULL(SslCtxObj);

  auto TlsObj = TlsNew(SslCtxObj);
  UT_ASSERT_NOT_NULL(TlsObj);

  UINT8 Buffer[256] = {0};
  UINTN BufferSize = sizeof(Buffer);
  Status = TlsWrite(SslCtxObj, Buffer, BufferSize);
  UT_ASSERT_TRUE(Status);

  // Cleanup
  TlsFree(TlsObj);
  TlsCtxFree(SslCtxObj);

  return UNIT_TEST_PASSED;
}
*/
