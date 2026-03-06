/** @file
  Unit tests for PKCS#7 Encrypt.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"

//
// Self-signed X509 certificate used for PKCS#7 Encrypt recipient (RSA 2048).
//
// $ openssl req -x509 -newkey rsa:2048 -keyout Pkcs7EncryptTestKey.pem
//     -out Pkcs7EncryptTestCert.pem -days 10000 -nodes -subj "/CN=Pkcs7EncryptTestCA"
// $ openssl x509 -in Pkcs7EncryptTestCert.pem -outform DER -out Pkcs7EncryptTestCert.der
// $ xxd --include Pkcs7EncryptTestCert.der
//
GLOBAL_REMOVE_IF_UNREFERENCED STATIC CONST UINT8  mPkcs7EncryptTestCert[] = {
  0x30, 0x82, 0x03, 0x1d, 0x30, 0x82, 0x02, 0x05, 0xa0, 0x03, 0x02, 0x01,
  0x02, 0x02, 0x14, 0x5f, 0x7c, 0x75, 0x20, 0x0e, 0x73, 0x61, 0x2c, 0x58,
  0x58, 0xe1, 0x6f, 0x3b, 0x1f, 0x92, 0xdb, 0x65, 0x58, 0x41, 0xf6, 0x30,
  0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b,
  0x05, 0x00, 0x30, 0x1d, 0x31, 0x1b, 0x30, 0x19, 0x06, 0x03, 0x55, 0x04,
  0x03, 0x0c, 0x12, 0x50, 0x6b, 0x63, 0x73, 0x37, 0x45, 0x6e, 0x63, 0x72,
  0x79, 0x70, 0x74, 0x54, 0x65, 0x73, 0x74, 0x43, 0x41, 0x30, 0x20, 0x17,
  0x0d, 0x32, 0x36, 0x30, 0x33, 0x30, 0x35, 0x32, 0x33, 0x31, 0x32, 0x30,
  0x39, 0x5a, 0x18, 0x0f, 0x32, 0x30, 0x35, 0x33, 0x30, 0x37, 0x32, 0x31,
  0x32, 0x33, 0x31, 0x32, 0x30, 0x39, 0x5a, 0x30, 0x1d, 0x31, 0x1b, 0x30,
  0x19, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x12, 0x50, 0x6b, 0x63, 0x73,
  0x37, 0x45, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x54, 0x65, 0x73, 0x74,
  0x43, 0x41, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86,
  0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01,
  0x0f, 0x00, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xc0,
  0x69, 0x5a, 0xd9, 0xa4, 0x05, 0x23, 0x13, 0x34, 0x64, 0xc5, 0x29, 0xf8,
  0x33, 0xfe, 0x51, 0xfc, 0x6b, 0x22, 0x73, 0x6e, 0x35, 0x93, 0x62, 0x20,
  0x63, 0xdd, 0x22, 0xf2, 0x49, 0xbd, 0x95, 0xa4, 0x73, 0x88, 0x9e, 0xf6,
  0x2d, 0x19, 0x26, 0x78, 0x40, 0x34, 0xb9, 0x9d, 0xc6, 0x19, 0xea, 0xca,
  0xae, 0xbf, 0x97, 0xe6, 0x22, 0xfc, 0xf7, 0xf3, 0xba, 0x99, 0xe4, 0x0d,
  0xd6, 0x9d, 0x2d, 0x1b, 0x0d, 0x40, 0x6f, 0x02, 0xb8, 0xeb, 0x47, 0xe2,
  0x00, 0xe0, 0x11, 0x02, 0x87, 0x7c, 0xe1, 0xf7, 0x4c, 0x00, 0xe0, 0xaf,
  0x33, 0x59, 0xc4, 0xda, 0x32, 0xc2, 0x87, 0x9b, 0x00, 0x8c, 0xa5, 0x24,
  0xdb, 0xcc, 0x15, 0xe6, 0xbb, 0x26, 0x81, 0x18, 0x28, 0x7b, 0x59, 0x79,
  0x05, 0x33, 0xfc, 0x92, 0x3c, 0x78, 0xd0, 0xee, 0x8e, 0xc1, 0xeb, 0x3b,
  0xfd, 0x02, 0x83, 0xc8, 0xe4, 0xd3, 0x15, 0x72, 0xe4, 0x67, 0x26, 0x59,
  0x04, 0x22, 0xfa, 0xe0, 0x94, 0x10, 0xf0, 0x47, 0x58, 0x72, 0x36, 0xc4,
  0x16, 0x48, 0xd4, 0x95, 0xf2, 0xe9, 0x53, 0x19, 0x9a, 0xf4, 0x50, 0xc3,
  0x62, 0xa5, 0x1b, 0x74, 0x97, 0x46, 0xb1, 0x83, 0x7d, 0xe5, 0x68, 0x6b,
  0xb2, 0x69, 0x4a, 0x49, 0x1b, 0x03, 0x39, 0x36, 0x8e, 0x4d, 0x4b, 0xe7,
  0x6e, 0x7e, 0xb1, 0x3b, 0x58, 0x69, 0x7a, 0x67, 0x74, 0x6b, 0xd8, 0x91,
  0x10, 0xfd, 0x97, 0x81, 0x67, 0x4c, 0x3e, 0x9f, 0x40, 0x3d, 0xd9, 0xc6,
  0x33, 0xc0, 0x31, 0x1d, 0xea, 0x6a, 0xb0, 0xbd, 0xd5, 0xb3, 0x08, 0x9b,
  0x04, 0x95, 0xba, 0xc7, 0x36, 0xdb, 0x3a, 0x6e, 0x5c, 0x2e, 0x28, 0x81,
  0xdc, 0xa1, 0xad, 0xdc, 0x30, 0x93, 0xdd, 0xb9, 0x69, 0xb2, 0x88, 0x06,
  0x6f, 0x90, 0xab, 0xc6, 0x2b, 0x92, 0x99, 0xba, 0x18, 0xf2, 0x65, 0xc9,
  0x43, 0x97, 0x7f, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x53, 0x30, 0x51,
  0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0xad,
  0xb0, 0x53, 0xb7, 0x1f, 0xc5, 0xfc, 0x13, 0x0b, 0x72, 0x95, 0xad, 0xbf,
  0xf6, 0x52, 0x66, 0x5d, 0x7b, 0x2f, 0xda, 0x30, 0x1f, 0x06, 0x03, 0x55,
  0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0xad, 0xb0, 0x53, 0xb7,
  0x1f, 0xc5, 0xfc, 0x13, 0x0b, 0x72, 0x95, 0xad, 0xbf, 0xf6, 0x52, 0x66,
  0x5d, 0x7b, 0x2f, 0xda, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01,
  0x01, 0xff, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0d, 0x06,
  0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00,
  0x03, 0x82, 0x01, 0x01, 0x00, 0x44, 0xfc, 0x84, 0x30, 0x1e, 0xec, 0xc8,
  0xb0, 0x33, 0xbb, 0xa3, 0xd7, 0xf8, 0xec, 0x53, 0x09, 0x80, 0x6f, 0x00,
  0xc9, 0x2e, 0xc2, 0x7f, 0x5d, 0xda, 0x58, 0xec, 0x62, 0x57, 0x2e, 0xba,
  0xd1, 0x57, 0x05, 0xcb, 0x22, 0x07, 0xff, 0xfb, 0x37, 0x85, 0x3d, 0x3c,
  0xed, 0x46, 0x09, 0x23, 0xe7, 0x7a, 0xcf, 0x05, 0xae, 0xd0, 0xd2, 0x23,
  0x05, 0x49, 0x7b, 0x5b, 0x1c, 0x1e, 0xab, 0x23, 0x9e, 0x12, 0x08, 0x95,
  0x5e, 0x1e, 0x67, 0xff, 0xf5, 0xc6, 0xb0, 0xa3, 0x28, 0x6f, 0xbe, 0x86,
  0xb7, 0x3c, 0x4a, 0xe1, 0xae, 0x1f, 0xd0, 0x33, 0x5a, 0x7c, 0x0c, 0xb8,
  0x2f, 0x44, 0x77, 0xd6, 0xc7, 0xc2, 0x07, 0x05, 0x36, 0xed, 0x5a, 0x50,
  0xd0, 0x68, 0x9b, 0x27, 0xbd, 0xd9, 0xea, 0x3f, 0x2a, 0x39, 0xbc, 0x3f,
  0x24, 0xe5, 0x72, 0xfa, 0x5d, 0x44, 0x87, 0x12, 0x35, 0x3e, 0xf2, 0xb4,
  0x11, 0x37, 0xe9, 0x0e, 0xac, 0xa0, 0xe7, 0x4a, 0x85, 0x77, 0x48, 0x1d,
  0xd6, 0xa9, 0x8a, 0x89, 0xc6, 0x35, 0x18, 0x24, 0x5c, 0xa8, 0xb3, 0x37,
  0x84, 0xfa, 0xd2, 0xf5, 0xa1, 0xf7, 0xdd, 0xd7, 0xc4, 0x2c, 0xb8, 0x17,
  0xa9, 0x2b, 0x93, 0x2a, 0xa7, 0xc6, 0x63, 0x0a, 0x85, 0xdb, 0xe8, 0xf5,
  0x9e, 0xc0, 0xfc, 0xa1, 0x44, 0x52, 0xa9, 0xdf, 0x7a, 0xaa, 0x50, 0x4e,
  0xe0, 0x50, 0xf1, 0x3e, 0xbb, 0xa4, 0x2b, 0x8c, 0xf4, 0x01, 0xb7, 0x49,
  0xbb, 0x5c, 0x9f, 0x18, 0x7e, 0x7f, 0x46, 0x42, 0xc3, 0x01, 0x21, 0x9b,
  0xa0, 0x15, 0xde, 0xa4, 0x6b, 0x04, 0xdf, 0x7d, 0x40, 0x9a, 0xd6, 0x9d,
  0x21, 0x4a, 0x17, 0xa5, 0xa7, 0xca, 0xaa, 0xb2, 0x0e, 0x23, 0x3b, 0x94,
  0xc6, 0x62, 0x65, 0x22, 0xa9, 0x6a, 0x98, 0xa2, 0xc8, 0xe9, 0x84, 0xf9,
  0x5c, 0x06, 0xa5, 0x8b, 0xa5, 0x3e, 0x5c, 0xb1, 0x0a
};

//
// PKCS#7 envelopedData OID: 1.2.840.113549.1.7.3
// This is the DER-encoded OID used to identify envelopedData in a PKCS#7 ContentInfo.
//
GLOBAL_REMOVE_IF_UNREFERENCED STATIC CONST UINT8  mPkcs7EnvelopedDataOid[] = {
  0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x03
};

//
// Plaintext data for encryption tests.
//
GLOBAL_REMOVE_IF_UNREFERENCED STATIC CONST CHAR8  *mPkcs7EncryptPayload = "PKCS7 Encrypt Test Payload Data";

/**
  Test basic Pkcs7Encrypt functionality with AES-256-CBC.

  Constructs an X509 certificate stack, encrypts test payload data,
  and validates that the output is non-NULL and non-zero size.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptBasicAes256 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack   = NULL;
  ContentInfo = NULL;
  TestStatus  = UNIT_TEST_ERROR_TEST_FAILED;

  //
  // Construct X509 certificate stack with the test certificate as recipient.
  //
  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  //
  // Encrypt the payload with AES-256-CBC.
  //
  Status = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPayload,
             AsciiStrLen (mPkcs7EncryptPayload),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (ContentInfo);
  UT_ASSERT_TRUE (ContentInfoSize > 0);

  //
  // The output should begin with a SEQUENCE tag (0x30) as a valid DER encoding.
  //
  UT_ASSERT_EQUAL (ContentInfo[0], 0x30);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (ContentInfo != NULL) {
    FreePool (ContentInfo);
  }

  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

/**
  Test Pkcs7Encrypt with AES-128-CBC cipher.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptAes128 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack   = NULL;
  ContentInfo = NULL;
  TestStatus  = UNIT_TEST_ERROR_TEST_FAILED;

  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  Status = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPayload,
             AsciiStrLen (mPkcs7EncryptPayload),
             CRYPTO_NID_AES128CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (ContentInfo);
  UT_ASSERT_TRUE (ContentInfoSize > 0);
  UT_ASSERT_EQUAL (ContentInfo[0], 0x30);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (ContentInfo != NULL) {
    FreePool (ContentInfo);
  }

  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

/**
  Test Pkcs7Encrypt with AES-192-CBC cipher.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptAes192 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack   = NULL;
  ContentInfo = NULL;
  TestStatus  = UNIT_TEST_ERROR_TEST_FAILED;

  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  Status = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPayload,
             AsciiStrLen (mPkcs7EncryptPayload),
             CRYPTO_NID_AES192CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (ContentInfo);
  UT_ASSERT_TRUE (ContentInfoSize > 0);
  UT_ASSERT_EQUAL (ContentInfo[0], 0x30);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (ContentInfo != NULL) {
    FreePool (ContentInfo);
  }

  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

/**
  Test Pkcs7Encrypt returns FALSE when X509Stack is NULL.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptNullX509Stack (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    *ContentInfo;
  UINTN    ContentInfoSize;

  ContentInfo = NULL;

  Status = Pkcs7Encrypt (
             NULL,
             (UINT8 *)mPkcs7EncryptPayload,
             AsciiStrLen (mPkcs7EncryptPayload),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );
  UT_ASSERT_FALSE (Status);

  return UNIT_TEST_PASSED;
}

/**
  Test Pkcs7Encrypt returns FALSE when InData is NULL.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptNullInData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack  = NULL;
  TestStatus = UNIT_TEST_ERROR_TEST_FAILED;

  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  Status = Pkcs7Encrypt (
             X509Stack,
             NULL,
             10,
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );
  UT_ASSERT_FALSE (Status);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

/**
  Test Pkcs7Encrypt returns FALSE when ContentInfo output pointer is NULL.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptNullContentInfo (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINTN             ContentInfoSize;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack  = NULL;
  TestStatus = UNIT_TEST_ERROR_TEST_FAILED;

  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  Status = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPayload,
             AsciiStrLen (mPkcs7EncryptPayload),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             NULL,
             &ContentInfoSize
             );
  UT_ASSERT_FALSE (Status);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

/**
  Test Pkcs7Encrypt returns FALSE when ContentInfoSize output pointer is NULL.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptNullContentInfoSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack  = NULL;
  TestStatus = UNIT_TEST_ERROR_TEST_FAILED;

  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  ContentInfo = NULL;
  Status      = Pkcs7Encrypt (
                  X509Stack,
                  (UINT8 *)mPkcs7EncryptPayload,
                  AsciiStrLen (mPkcs7EncryptPayload),
                  CRYPTO_NID_AES256CBC,
                  CRYPTO_PKCS7_DEFAULT,
                  &ContentInfo,
                  NULL
                  );
  UT_ASSERT_FALSE (Status);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

/**
  Test Pkcs7Encrypt returns FALSE with an unsupported cipher NID.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptInvalidCipherNid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack  = NULL;
  TestStatus = UNIT_TEST_ERROR_TEST_FAILED;

  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  //
  // Use CRYPTO_NID_NULL (0) which is not a supported AES cipher.
  //
  Status = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPayload,
             AsciiStrLen (mPkcs7EncryptPayload),
             CRYPTO_NID_NULL,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );
  UT_ASSERT_FALSE (Status);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

/**
  Test Pkcs7Encrypt returns FALSE with invalid flags.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptInvalidFlags (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack  = NULL;
  TestStatus = UNIT_TEST_ERROR_TEST_FAILED;

  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  //
  // Use an unsupported flags value (0xFFFFFFFF).
  //
  Status = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPayload,
             AsciiStrLen (mPkcs7EncryptPayload),
             CRYPTO_NID_AES256CBC,
             0xFFFFFFFF,
             &ContentInfo,
             &ContentInfoSize
             );
  UT_ASSERT_FALSE (Status);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

/**
  Test that Pkcs7Encrypt output contains the PKCS#7 envelopedData OID.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptOutputContainsEnvelopedDataOid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UINTN             Index;
  BOOLEAN           FoundOid;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack   = NULL;
  ContentInfo = NULL;
  TestStatus  = UNIT_TEST_ERROR_TEST_FAILED;

  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  Status = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPayload,
             AsciiStrLen (mPkcs7EncryptPayload),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (ContentInfo);
  UT_ASSERT_TRUE (ContentInfoSize > sizeof (mPkcs7EnvelopedDataOid));

  //
  // Scan the ContentInfo for the envelopedData OID (1.2.840.113549.1.7.3).
  //
  FoundOid = FALSE;
  for (Index = 0; Index <= ContentInfoSize - sizeof (mPkcs7EnvelopedDataOid); Index++) {
    if (CompareMem (
          ContentInfo + Index,
          mPkcs7EnvelopedDataOid,
          sizeof (mPkcs7EnvelopedDataOid)
          ) == 0)
    {
      FoundOid = TRUE;
      break;
    }
  }

  UT_ASSERT_TRUE (FoundOid);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (ContentInfo != NULL) {
    FreePool (ContentInfo);
  }

  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

/**
  Test that Pkcs7Encrypt returns FALSE when InDataSize exceeds INT_MAX.
**/
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptInDataSizeOverflow (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN           Status;
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UNIT_TEST_STATUS  TestStatus;

  X509Stack  = NULL;
  TestStatus = UNIT_TEST_ERROR_TEST_FAILED;

  Status = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  if (!Status) {
    UT_LOG_ERROR ("X509ConstructCertificateStack failed.\n");
    goto Exit;
  }

  UT_ASSERT_NOT_NULL (X509Stack);

  //
  // Use a size larger than INT_MAX to trigger the size check.
  //
  Status = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPayload,
             ((UINTN)MAX_INT32) + 1,
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );
  UT_ASSERT_FALSE (Status);

  TestStatus = UNIT_TEST_PASSED;

Exit:
  if (X509Stack != NULL) {
    X509StackFree (X509Stack);
  }

  return TestStatus;
}

TEST_DESC  mPkcs7EncryptTest[] = {
  //
  // -----Description---------------------------------------------------Class---------------------------------Function--------------------------------------Pre---Post--Context
  //
  { "TestPkcs7EncryptBasicAes256()",                   "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptBasicAes256,                   NULL, NULL, NULL },
  { "TestPkcs7EncryptAes128()",                        "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptAes128,                        NULL, NULL, NULL },
  { "TestPkcs7EncryptAes192()",                        "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptAes192,                        NULL, NULL, NULL },
  { "TestPkcs7EncryptNullX509Stack()",                 "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptNullX509Stack,                 NULL, NULL, NULL },
  { "TestPkcs7EncryptNullInData()",                    "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptNullInData,                    NULL, NULL, NULL },
  { "TestPkcs7EncryptNullContentInfo()",               "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptNullContentInfo,               NULL, NULL, NULL },
  { "TestPkcs7EncryptNullContentInfoSize()",           "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptNullContentInfoSize,           NULL, NULL, NULL },
  { "TestPkcs7EncryptInvalidCipherNid()",              "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptInvalidCipherNid,              NULL, NULL, NULL },
  { "TestPkcs7EncryptInvalidFlags()",                  "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptInvalidFlags,                  NULL, NULL, NULL },
  { "TestPkcs7EncryptOutputContainsEnvelopedDataOid()", "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptOutputContainsEnvelopedDataOid, NULL, NULL, NULL },
  { "TestPkcs7EncryptInDataSizeOverflow()",            "CryptoPkg.BaseCryptLib.Pkcs7Encrypt", TestPkcs7EncryptInDataSizeOverflow,            NULL, NULL, NULL },
};

UINTN  mPkcs7EncryptTestNum = ARRAY_SIZE (mPkcs7EncryptTest);
