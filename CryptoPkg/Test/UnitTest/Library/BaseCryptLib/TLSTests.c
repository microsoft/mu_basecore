/** @file
  Application for Diffie-Hellman Primitives Validation.

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"
#include <Library/TlsLib.h>
#include "TlsDriver.h"
#include "TlsImpl.h"           // For pulling "EfiTlsClient" enum


// List of Ciphers as appears in TLS Cipher Suite Registry of the IANA
// https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml

// TODO: Verify order of bytes is correct in all cases (or use UINT8)
CONST UINT16 mCipherId[] = {  0xC030,  // TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
                              0xC02F,  // TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
                              0xC028,  // TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384
                              0xC027   // TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
                            };
#define CIPHER_COUNT (sizeof(mCipherId) / sizeof(mCipherId[0]))


// TODO: Check if we need to test other versions then SSL3.1
#define TLS_PROTOCOL_VERSION_MAJOR 0x03
#define TLS_PROTOCOL_VERSION_MINOR 0x01


UNIT_TEST_STATUS
EFIAPI
TestVerifyTlsPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  // TODO: Flags to be removed with the refactoring of UEFI PCDs
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
  // TODO: Fill in in case needed
}

UNIT_TEST_STATUS
EFIAPI
TestTls31CreatCtxObjNewFree (
  VOID
  )
{
  TLS_SERVICE  *TlsService;

  BOOLEAN Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);
  
  auto SslCtxObj = TlsCtxNew(TLS_PROTOCOL_VERSION_MAJOR,TLS_PROTOCOL_VERSION_MINOR);
  UT_ASSERT_NOT_NULL(SslCtxObj);
  
  auto TlsObj = TlsNew(SslCtxObj);
  UT_ASSERT_NOT_NULL(TlsObj);

  // Cleanup
  TlsFree(TlsObj);
  TlsCtxFree(SslCtxObj);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTls31ServiceCreateConnection (
  VOID
  )
{
  EFI_HANDLE ImageHandle;
  TLS_SERVICE  *TlsService;
  TLS_INSTANCE  *TlsInstance;
  EFI_STATUS Status;

  Status = TlsCreateService(ImageHandle, &TlsService);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);
  
  TlsService->TlsCtx = TlsCtxNew(TLS_PROTOCOL_VERSION_MAJOR,TLS_PROTOCOL_VERSION_MINOR);
  UT_ASSERT_NOT_NULL(TlsService->TlsCtx);
  
  Status = TlsCreateInstance (TlsService, &TlsInstance);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  TlsInstance->TlsConn = TlsNew(TlsService->TlsCtx);
  UT_ASSERT_NOT_NULL(TlsInstance->TlsConn);
  
  Status = TlsSetConnectionEnd (TlsInstance->TlsConn, EfiTlsClient);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  // Cleanup 
  // NOTE: this is aligned with other tests, but will not be called if test fails
  TlsFree(TlsInstance->TlsConn);
  TlsCtxFree(TlsService->TlsCtx);
  TlsCleanService(TlsService);

  return UNIT_TEST_PASSED;
}


// TODO: Check if we need to call other stages to establish connection
//       For example: Handshake, etc.

UNIT_TEST_STATUS
EFIAPI
TestTls31VerifySetCipherList (
  VOID
  )
{
  UINT16  CipherId = 0;
  EFI_HANDLE ImageHandle;
  TLS_SERVICE  *TlsService;
  TLS_INSTANCE  *TlsInstance;
  EFI_STATUS Status;

  Status = TlsCreateService(ImageHandle, &TlsService);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);
  
  TlsService->TlsCtx = TlsCtxNew(TLS_PROTOCOL_VERSION_MAJOR,TLS_PROTOCOL_VERSION_MINOR);
  UT_ASSERT_NOT_NULL(TlsService->TlsCtx);
  
  Status = TlsCreateInstance (TlsService, &TlsInstance);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  TlsInstance->TlsConn = TlsNew(TlsService->TlsCtx);
  UT_ASSERT_NOT_NULL(TlsInstance->TlsConn);
  
  Status = TlsSetConnectionEnd (TlsInstance->TlsConn, EfiTlsClient);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  Status = TlsSetCipherList (TlsInstance->TlsConn, mCipherId, CIPHER_COUNT);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  TlsGetCurrentCipher(TlsInstance->TlsConn, &CipherId);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  BOOLEAN Found = FALSE;

  for (int i = 0 ; i < CIPHER_COUNT ; i++) {
    if (mCipherId[i] == CipherId) {
      Found = TRUE;
      break;
    }
  }
  UT_ASSERT_TRUE(Found);

  // Cleanup 
  // NOTE: this is aligned with other tests, but will not be called if test fails
  TlsFree(TlsInstance->TlsConn);
  TlsCtxFree(TlsService->TlsCtx);
  TlsCleanService(TlsService);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTls31GetCurrentCipher (
  VOID
  )
{
  UINT16  CipherId = 0;
  EFI_HANDLE ImageHandle;
  TLS_SERVICE  *TlsService;
  TLS_INSTANCE  *TlsInstance;
  EFI_STATUS Status;

  Status = TlsCreateService(ImageHandle, &TlsService);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  Status = TlsInitialize();
  UT_ASSERT_TRUE (Status);
  
  TlsService->TlsCtx = TlsCtxNew(TLS_PROTOCOL_VERSION_MAJOR,TLS_PROTOCOL_VERSION_MINOR);
  UT_ASSERT_NOT_NULL(TlsService->TlsCtx);
  
  Status = TlsCreateInstance (TlsService, &TlsInstance);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  TlsInstance->TlsConn = TlsNew(TlsService->TlsCtx);
  UT_ASSERT_NOT_NULL(TlsInstance->TlsConn);

  TlsGetCurrentCipher(TlsInstance->TlsConn, &CipherId);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  BOOLEAN Found = FALSE;

  for (int i = 0 ; i < CIPHER_COUNT ; i++) {
    if (mCipherId[i] == CipherId) {
      Found = TRUE;
      break;
    }
  }
  UT_ASSERT_TRUE(Found);

  Status = TlsSetConnectionEnd (TlsInstance->TlsConn, EfiTlsClient);
  UT_ASSERT_EQUAL(EFI_SUCCESS, Status);

  // Cleanup 
  // NOTE: this is aligned with other tests, but will not be called if test fails
  TlsFree(TlsInstance->TlsConn);
  TlsCtxFree(TlsService->TlsCtx);
  TlsCleanService(TlsService);

  return UNIT_TEST_PASSED;
}


TEST_DESC  mTlsTest[] = {
  //
  // -----Description--------------------------------Class---------------------Function----------------Pre-----------------Post------------Context
  //
  { "TestTls31CreatCtxObjNewFree()", "CryptoPkg.BaseCryptLib.Tls", TestTls31CreatCtxObjNewFree, TestVerifyTlsPreReq, NULL, NULL},
  { "TestTls31ServiceCreateConnection()", "CryptoPkg.BaseCryptLib.Tls", TestTls31ServiceCreateConnection, TestVerifyTlsPreReq, NULL, NULL},
  { "TestTls31VerifyConnection()", "CryptoPkg.BaseCryptLib.Tls", TestTls31VerifySetCipherList, TestVerifyTlsPreReq, NULL, NULL},
  { "TestTls31VerifyCurrentCipher()", "CryptoPkg.BaseCryptLib.Tls", TestTls31GetCurrentCipher, TestVerifyTlsPreReq, NULL, NULL}
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