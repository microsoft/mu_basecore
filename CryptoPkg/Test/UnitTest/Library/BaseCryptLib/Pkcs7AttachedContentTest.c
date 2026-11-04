/** @file
  Application for Pkcs7 Attached Content Validation.

Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"

#include "Pkcs7AttachedContentSignatures.h"

/**
  TestPkcs7Attached()

  Pass the API a pkcs7 with content data and contents return should not be null.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required pkcs7 contents return not be null.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestPkcs7Attached (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );

  UT_ASSERT_EQUAL (Status, TRUE);

  // AttachedData != NULL
  if (AttachedData == NULL) {
    UT_ASSERT_TRUE (FALSE);
  }

  return UNIT_TEST_PASSED;
}// TestPkcs7Attached()

/**
  TestPkcs7Detached()

  Pass the API a pkcs7 detached (without content data) and contents return should be null.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required pkcs7 detached contents return be null.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestPkcs7Detached (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7DetachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7DetachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, TRUE);

  // Check no AttachedData
  if ((AttachedData != NULL) || (AttachedDataSize != 0)) {
    UT_ASSERT_TRUE (FALSE);
  }

  return UNIT_TEST_PASSED;
}// TestPkcs7Detached()

/**
  TestVerifyPkcs7ContentData()

  Pass the API a pkcs7 with content data and verify contents return.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required pkcs7 contents return were correct.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestVerifyPkcs7ContentData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, TRUE);

  // Compare AttachedData
  if (CompareMem (AttachedData, &AttachedContents, sizeof (AttachedContents)) != 0) {
    UT_ASSERT_TRUE (FALSE);
  }

  return UNIT_TEST_PASSED;
}// TestVerifyPkcs7ContentData()

/**
  TestWithInvalidpkcs7()

  Pass the API a Invalid pkcs7 and ensure that they don't pass.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required pkcs7 were found.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestWithInvalidPkcs7 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7FailTest);

  Status = Pkcs7GetAttachedContent (
             Pck7FailTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  return UNIT_TEST_PASSED;
}// TestWithInvalidPkcs7()

/**
  TestWithInvalidParameters()

  Pass the API a Invalid Parameters and checks function error handling.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required API handling invalid parameters good.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestWithInvalidParameters (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             NULL,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  SignedDataSize = (UINTN)(2147483647U + 1U); // INT MAX + 1

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             NULL,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             NULL
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  return UNIT_TEST_PASSED;
}// TestWithInvalidParameters()

/**
  Parse attached content with short-, one-byte-, and two-byte DER lengths.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7ContentLengthEncoding (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST UINT8  Prefix64[] = {
    0x30, 0x67, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02,
    0xA0, 0x5A, 0x30, 0x58, 0x02, 0x01, 0x01, 0x31, 0x00, 0x30, 0x4F, 0x06, 0x09,
    0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x01, 0xA0, 0x42, 0x04, 0x40
  };
  STATIC CONST UINT8  Prefix128[] = {
    0x30, 0x81, 0xAC, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02,
    0xA0, 0x81, 0x9E, 0x30, 0x81, 0x9B, 0x02, 0x01, 0x01, 0x31, 0x00, 0x30, 0x81, 0x91,
    0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x01, 0xA0, 0x81, 0x83,
    0x04, 0x81, 0x80
  };
  STATIC CONST UINT8  Prefix256[] = {
    0x30, 0x82, 0x01, 0x31, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02,
    0xA0, 0x82, 0x01, 0x22, 0x30, 0x82, 0x01, 0x1E, 0x02, 0x01, 0x01, 0x31, 0x00, 0x30, 0x82,
    0x01, 0x13, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x01, 0xA0, 0x82,
    0x01, 0x04, 0x04, 0x82, 0x01, 0x00
  };
  CONST UINT8         *Prefixes[]   = { Prefix64, Prefix128, Prefix256 };
  UINTN               PrefixSizes[] = { sizeof (Prefix64), sizeof (Prefix128), sizeof (Prefix256) };
  UINT8               Payload[256]  = { 0 };
  UINT8               *Data;
  VOID                *Content;
  UINTN               ContentSize;
  UINTN               PayloadSize;
  UINTN               Size;
  UINTN               Index;
  BOOLEAN             Passed;

  for (Index = 0; Index < ARRAY_SIZE (Prefixes); Index++) {
    PayloadSize = 64U << Index;
    Size        = PrefixSizes[Index] + PayloadSize + 2;
    Data        = AllocateZeroPool (Size);
    UT_ASSERT_NOT_NULL (Data);
    CopyMem (Data, Prefixes[Index], PrefixSizes[Index]);
    Data[Size - 2] = 0x31; // Empty signerInfos SET follows the attached content.
    Content        = NULL;
    ContentSize    = 0;
    Passed         = Pkcs7GetAttachedContent (Data, Size, &Content, &ContentSize);
    Passed         = (BOOLEAN)(Passed && (Content != NULL) && (ContentSize == PayloadSize) &&
                               (CompareMem (Content, Payload, PayloadSize) == 0));
    if (Content != NULL) {
      FreePool (Content);
    }

    FreePool (Data);
    UT_ASSERT_TRUE (Passed);
  }

  return UNIT_TEST_PASSED;
}

TEST_DESC  mPkcs7ContentTest[] = {
  //
  // -----Description--------------------------------Class----------------------------Function------------------------------Pre---Post--Context
  //
  { "TestPkcs7Attached()",          "CryptoPkg.BaseCryptLib.Pkcs7Content",               TestPkcs7Attached,              NULL, NULL, NULL },
  { "TestPkcs7Detached()",          "CryptoPkg.BaseCryptLib.TestPkcs7Detached",          TestPkcs7Detached,              NULL, NULL, NULL },
  { "TestVerifyPkcs7ContentData()", "CryptoPkg.BaseCryptLib.TestVerifyPkcs7ContentData", TestVerifyPkcs7ContentData,     NULL, NULL, NULL },
  { "TestWithInvalidPkcs7()",       "CryptoPkg.BaseCryptLib.TestWithInvalidPkcs7",       TestWithInvalidPkcs7,           NULL, NULL, NULL },
  { "TestWithInvalidParameters()",  "CryptoPkg.BaseCryptLib.TestWithInvalidParameters",  TestWithInvalidParameters,      NULL, NULL, NULL },
  { "PKCS7 content DER lengths",    "CryptoPkg.BaseCryptLib.Pkcs7Content",               TestPkcs7ContentLengthEncoding, NULL, NULL, NULL },
};

UINTN  mPkcs7ContentTestNum = ARRAY_SIZE (mPkcs7ContentTest);

TEST_DESC  mPkcs7ContentTestMbedTls[] = {
  //
  // -----Description--------------------------------Class----------------------------Function------------------------------Pre---Post--Context
  //
  { "TestWithInvalidPkcs7()",      "CryptoPkg.BaseCryptLib.TestWithInvalidPkcs7",      TestWithInvalidPkcs7,      NULL, NULL, NULL },
  { "TestWithInvalidParameters()", "CryptoPkg.BaseCryptLib.TestWithInvalidParameters", TestWithInvalidParameters, NULL, NULL, NULL },
};

UINTN  mPkcs7ContentTestMbedTlsNum = ARRAY_SIZE (mPkcs7ContentTestMbedTls);
