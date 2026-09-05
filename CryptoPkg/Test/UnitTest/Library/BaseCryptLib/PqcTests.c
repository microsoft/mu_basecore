/** @file
  Post-Quantum Cryptography (PQC) Verification Tests.

  Tests ML-DSA-44, ML-DSA-65, and ML-DSA-87 CMS signature verification
  through the Pkcs7Verify() API. The CMS signed data was generated with
  OpenSSL 4.0.0 using Scripts/build-openssl-local.sh.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

//
// Auto-generated test vectors: ML-DSA self-signed certs and CMS signed blobs.
// Payload is "Hello PQC World" (15 bytes), signed with -binary -noattr.
//
#include "PqcTestData.h"

/**
  Verify an ML-DSA CMS signature using Pkcs7Verify.

  @param[in] CmsData      DER-encoded CMS ContentInfo (signed data).
  @param[in] CmsDataSize  Size of CmsData in bytes.
  @param[in] Cert         DER-encoded X.509 signer certificate.
  @param[in] CertSize     Size of Cert in bytes.
  @param[in] Payload      Original signed payload.
  @param[in] PayloadSize  Size of Payload in bytes.
  @param[in] AlgoName     Algorithm name for logging.

  @retval UNIT_TEST_PASSED  Verification succeeded.
**/
STATIC
UNIT_TEST_STATUS
VerifyMlDsaCmsSignature (
  IN CONST UINT8  *CmsData,
  IN UINTN        CmsDataSize,
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  IN CONST UINT8  *Payload,
  IN UINTN        PayloadSize,
  IN CONST CHAR8  *AlgoName
  )
{
  BOOLEAN  Status;

  Status = Pkcs7Verify (
             CmsData,
             CmsDataSize,
             Cert,
             CertSize,
             Payload,
             PayloadSize
             );

  UT_LOG_INFO ("%a CMS verification: %a\n", AlgoName, Status ? "PASSED" : "FAILED");
  UT_ASSERT_TRUE (Status);

  return UNIT_TEST_PASSED;
}

/**
  Verify ML-DSA-44 CMS signature through Pkcs7Verify.
**/
UNIT_TEST_STATUS
EFIAPI
TestPqcMlDsa44Verify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return VerifyMlDsaCmsSignature (
           mMlDsa44CmsSigned,
           sizeof (mMlDsa44CmsSigned),
           mMlDsa44Cert,
           sizeof (mMlDsa44Cert),
           mPqcTestPayload,
           sizeof (mPqcTestPayload),
           "ML-DSA-44"
           );
}

/**
  Verify ML-DSA-65 CMS signature through Pkcs7Verify.
**/
UNIT_TEST_STATUS
EFIAPI
TestPqcMlDsa65Verify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return VerifyMlDsaCmsSignature (
           mMlDsa65CmsSigned,
           sizeof (mMlDsa65CmsSigned),
           mMlDsa65Cert,
           sizeof (mMlDsa65Cert),
           mPqcTestPayload,
           sizeof (mPqcTestPayload),
           "ML-DSA-65"
           );
}

/**
  Verify ML-DSA-87 CMS signature through Pkcs7Verify.
**/
UNIT_TEST_STATUS
EFIAPI
TestPqcMlDsa87Verify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return VerifyMlDsaCmsSignature (
           mMlDsa87CmsSigned,
           sizeof (mMlDsa87CmsSigned),
           mMlDsa87Cert,
           sizeof (mMlDsa87Cert),
           mPqcTestPayload,
           sizeof (mPqcTestPayload),
           "ML-DSA-87"
           );
}

/**
  Verify that ML-DSA verification rejects tampered data.
**/
UNIT_TEST_STATUS
EFIAPI
TestPqcMlDsa87VerifyTampered (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    TamperedPayload[] = "Hello PQC Xorld";

  Status = Pkcs7Verify (
             mMlDsa87CmsSigned,
             sizeof (mMlDsa87CmsSigned),
             mMlDsa87Cert,
             sizeof (mMlDsa87Cert),
             TamperedPayload,
             sizeof (TamperedPayload) - 1
             );

  UT_LOG_INFO ("ML-DSA-87 tampered data rejection: %a\n", !Status ? "PASSED" : "FAILED");
  UT_ASSERT_FALSE (Status);

  return UNIT_TEST_PASSED;
}

/**
  Verify that ML-DSA verification rejects a wrong certificate.
**/
UNIT_TEST_STATUS
EFIAPI
TestPqcMlDsa87VerifyWrongCert (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;

  //
  // Use ML-DSA-44 cert to verify ML-DSA-87 signature — must fail.
  //
  Status = Pkcs7Verify (
             mMlDsa87CmsSigned,
             sizeof (mMlDsa87CmsSigned),
             mMlDsa44Cert,
             sizeof (mMlDsa44Cert),
             mPqcTestPayload,
             sizeof (mPqcTestPayload)
             );

  UT_LOG_INFO ("ML-DSA-87 wrong cert rejection: %a\n", !Status ? "PASSED" : "FAILED");
  UT_ASSERT_FALSE (Status);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mPqcTest[] = {
  //
  // Description-------------------------------------------Class----------------------------Func-------------------------------PreReq--CleanUp--Context
  //
  { "ML-DSA-44 CMS signature verification",            "CryptoPkg.BaseCryptLib.Pqc", TestPqcMlDsa44Verify,        NULL, NULL, NULL },
  { "ML-DSA-65 CMS signature verification",            "CryptoPkg.BaseCryptLib.Pqc", TestPqcMlDsa65Verify,        NULL, NULL, NULL },
  { "ML-DSA-87 CMS signature verification",            "CryptoPkg.BaseCryptLib.Pqc", TestPqcMlDsa87Verify,        NULL, NULL, NULL },
  { "ML-DSA-87 rejects tampered data",                 "CryptoPkg.BaseCryptLib.Pqc", TestPqcMlDsa87VerifyTampered, NULL, NULL, NULL },
  { "ML-DSA-87 rejects wrong certificate",             "CryptoPkg.BaseCryptLib.Pqc", TestPqcMlDsa87VerifyWrongCert, NULL, NULL, NULL },
};

UINTN  mPqcTestNum = ARRAY_SIZE (mPqcTest);
