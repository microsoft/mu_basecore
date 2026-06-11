/** @file
  Unit tests for X509GetTbsCertHash().

  The tests feed a known self-signed X.509 certificate to
  X509GetTbsCertHash() for each supported digest algorithm and assert
  that the returned digest matches a reference value computed
  independently with the public BaseCryptLib one-shot hash primitives
  (X509GetTBSCert() + ShaXxxHashAll()). Bad-parameter, unsupported-
  algorithm, and malformed-certificate cases are also covered.

  The tests depend only on public BaseCryptLib API, so they run
  identically against the OpenSSL and MbedTLS BaseCryptLib instances.

Copyright (C) Microsoft Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"
#include <Guid/ImageAuthentication.h>

//
// Self-signed test certificate (ECDSA P-256 / SHA-256, 468 bytes DER).
// Identical bytes to the certificate used by the TrustAnchor tests, but
// kept STATIC here so the two test files remain independent. It is only
// used as a stable, deterministic certificate to feed into
// X509GetTbsCertHash(). Regenerate with:
//   openssl ecparam -name prime256v1 -genkey -noout -out anchor.key
//   openssl req -new -x509 -key anchor.key -days 36500 -sha256
//     -subj '/CN=Edk2 BaseCryptLib TrustAnchor Test/O=Edk2'
//     -out anchor.pem
//   openssl x509 -in anchor.pem -outform DER -out anchor.der
//
STATIC CONST UINT8  mTbsHashTestCert[] = {
  0x30, 0x82, 0x01, 0xd0, 0x30, 0x82, 0x01, 0x75, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x14, 0x02,
  0x69, 0xba, 0x47, 0x29, 0x15, 0xe0, 0x37, 0x67, 0xaf, 0x93, 0x02, 0x42, 0x82, 0xb3, 0x8e, 0xba,
  0xe4, 0xe6, 0xf5, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x30,
  0x3c, 0x31, 0x2b, 0x30, 0x29, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x22, 0x45, 0x64, 0x6b, 0x32,
  0x20, 0x42, 0x61, 0x73, 0x65, 0x43, 0x72, 0x79, 0x70, 0x74, 0x4c, 0x69, 0x62, 0x20, 0x54, 0x72,
  0x75, 0x73, 0x74, 0x41, 0x6e, 0x63, 0x68, 0x6f, 0x72, 0x20, 0x54, 0x65, 0x73, 0x74, 0x31, 0x0d,
  0x30, 0x0b, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x04, 0x45, 0x64, 0x6b, 0x32, 0x30, 0x20, 0x17,
  0x0d, 0x32, 0x36, 0x30, 0x36, 0x31, 0x30, 0x32, 0x32, 0x35, 0x36, 0x35, 0x32, 0x5a, 0x18, 0x0f,
  0x32, 0x31, 0x32, 0x36, 0x30, 0x35, 0x31, 0x37, 0x32, 0x32, 0x35, 0x36, 0x35, 0x32, 0x5a, 0x30,
  0x3c, 0x31, 0x2b, 0x30, 0x29, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x22, 0x45, 0x64, 0x6b, 0x32,
  0x20, 0x42, 0x61, 0x73, 0x65, 0x43, 0x72, 0x79, 0x70, 0x74, 0x4c, 0x69, 0x62, 0x20, 0x54, 0x72,
  0x75, 0x73, 0x74, 0x41, 0x6e, 0x63, 0x68, 0x6f, 0x72, 0x20, 0x54, 0x65, 0x73, 0x74, 0x31, 0x0d,
  0x30, 0x0b, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x04, 0x45, 0x64, 0x6b, 0x32, 0x30, 0x59, 0x30,
  0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce,
  0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0xe9, 0xe0, 0x54, 0x8f, 0x06, 0x5e, 0x77, 0x3c,
  0x18, 0x57, 0xe9, 0x24, 0xd0, 0xac, 0xb6, 0xaa, 0x6b, 0x90, 0x31, 0xac, 0xbc, 0x6d, 0x49, 0xc8,
  0x5a, 0xe7, 0x3d, 0x37, 0x68, 0x08, 0x17, 0xcd, 0xde, 0xa1, 0xbb, 0x1f, 0x37, 0x63, 0xd1, 0x74,
  0xe4, 0x7b, 0x5f, 0x41, 0x92, 0x79, 0x93, 0x6b, 0xe6, 0xdb, 0x4d, 0x60, 0xf2, 0x09, 0xe3, 0xe5,
  0x45, 0xf9, 0xf9, 0xd6, 0x24, 0x77, 0x29, 0x3d, 0xa3, 0x53, 0x30, 0x51, 0x30, 0x1d, 0x06, 0x03,
  0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0xb4, 0x77, 0xef, 0xe6, 0xe2, 0x4f, 0x97, 0xd0, 0x12,
  0xc8, 0xa6, 0xb0, 0xec, 0x95, 0xdb, 0xc4, 0xa5, 0x3b, 0x76, 0xf1, 0x30, 0x1f, 0x06, 0x03, 0x55,
  0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0xb4, 0x77, 0xef, 0xe6, 0xe2, 0x4f, 0x97, 0xd0,
  0x12, 0xc8, 0xa6, 0xb0, 0xec, 0x95, 0xdb, 0xc4, 0xa5, 0x3b, 0x76, 0xf1, 0x30, 0x0f, 0x06, 0x03,
  0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0a, 0x06,
  0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x49, 0x00, 0x30, 0x46, 0x02, 0x21,
  0x00, 0xaa, 0x4b, 0x52, 0x77, 0xec, 0xd8, 0x28, 0x2d, 0x0d, 0x3b, 0x15, 0xec, 0xdc, 0xde, 0x10,
  0xdb, 0x22, 0xdc, 0x61, 0x2b, 0xcc, 0x96, 0x12, 0xe7, 0x15, 0xdb, 0x20, 0x0f, 0x42, 0xc9, 0xf6,
  0x4a, 0x02, 0x21, 0x00, 0xcd, 0xbf, 0xb8, 0x04, 0xc8, 0x62, 0x5d, 0xf8, 0xe9, 0xae, 0x2b, 0x81,
  0x8b, 0xf6, 0xcb, 0x31, 0x80, 0xea, 0xbc, 0x59, 0x71, 0xb5, 0xba, 0x20, 0xff, 0xb8, 0x3e, 0x79,
  0x7a, 0x67, 0x85, 0x7e,
};

//
// Signature-type GUIDs, materialized locally so the test does not depend
// on the gEfiCert*Guid link symbols.
//
STATIC CONST EFI_GUID  mTbsSha1Guid   = EFI_CERT_SHA1_GUID;
STATIC CONST EFI_GUID  mTbsSha256Guid = EFI_CERT_SHA256_GUID;
STATIC CONST EFI_GUID  mTbsSha384Guid = EFI_CERT_SHA384_GUID;
STATIC CONST EFI_GUID  mTbsSha512Guid = EFI_CERT_SHA512_GUID;

//
// A GUID that is not a recognized image-hash algorithm.
//
STATIC CONST EFI_GUID  mTbsBogusGuid = {
  0x11223344, 0x5566, 0x7788, { 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00 }
};

/**
  Compute the reference TBSCertificate digest of mTbsHashTestCert under a
  given algorithm, using only the public BaseCryptLib one-shot hash
  primitives.

  @param[in]   HashSize    20, 32, 48, or 64.
  @param[out]  Digest      Caller-allocated, at least HashSize bytes.

  @retval TRUE   Reference digest computed.
  @retval FALSE  X509GetTBSCert / hash failed.
**/
STATIC
BOOLEAN
ComputeExpectedTbsCertHash (
  IN  UINTN  HashSize,
  OUT UINT8  *Digest
  )
{
  UINT8    *TbsCert;
  UINTN    TbsCertSize;
  BOOLEAN  Ok;

  TbsCert     = NULL;
  TbsCertSize = 0;
  if (!X509GetTBSCert (mTbsHashTestCert, sizeof (mTbsHashTestCert), &TbsCert, &TbsCertSize)) {
    return FALSE;
  }

  switch (HashSize) {
    case SHA1_DIGEST_SIZE:
      Ok = Sha1HashAll (TbsCert, TbsCertSize, Digest);
      break;
    case SHA256_DIGEST_SIZE:
      Ok = Sha256HashAll (TbsCert, TbsCertSize, Digest);
      break;
    case SHA384_DIGEST_SIZE:
      Ok = Sha384HashAll (TbsCert, TbsCertSize, Digest);
      break;
    case SHA512_DIGEST_SIZE:
      Ok = Sha512HashAll (TbsCert, TbsCertSize, Digest);
      break;
    default:
      Ok = FALSE;
      break;
  }

  return Ok;
}

/**
  Run one positive case: X509GetTbsCertHash() must return EFI_SUCCESS, a
  DigestSize equal to the algorithm's digest length, and a digest that
  matches the independently computed reference.

  @param[in]  HashType   Signature-type GUID for the algorithm.
  @param[in]  HashSize   Expected digest size in bytes.

  @retval UNIT_TEST_PASSED  The case passed.
**/
STATIC
UNIT_TEST_STATUS
RunPositiveTbsHashCase (
  IN CONST EFI_GUID  *HashType,
  IN UINTN           HashSize
  )
{
  EFI_STATUS  Status;
  UINT8       Digest[SHA512_DIGEST_SIZE];
  UINT8       Expected[SHA512_DIGEST_SIZE];
  UINTN       DigestSize;

  ZeroMem (Digest, sizeof (Digest));
  ZeroMem (Expected, sizeof (Expected));

  UT_ASSERT_TRUE (ComputeExpectedTbsCertHash (HashSize, Expected));

  DigestSize = 0;
  Status     = X509GetTbsCertHash (
                 (VOID *)mTbsHashTestCert,
                 sizeof (mTbsHashTestCert),
                 HashType,
                 Digest,
                 &DigestSize
                 );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (DigestSize, HashSize);
  UT_ASSERT_MEM_EQUAL (Digest, Expected, HashSize);

  return UNIT_TEST_PASSED;
}

/**
  SHA-1 TBSCertificate digest is computed correctly.
**/
UNIT_TEST_STATUS
EFIAPI
TestTbsHashSha1 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPositiveTbsHashCase (&mTbsSha1Guid, SHA1_DIGEST_SIZE);
}

/**
  SHA-256 TBSCertificate digest is computed correctly.
**/
UNIT_TEST_STATUS
EFIAPI
TestTbsHashSha256 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPositiveTbsHashCase (&mTbsSha256Guid, SHA256_DIGEST_SIZE);
}

/**
  SHA-384 TBSCertificate digest is computed correctly.
**/
UNIT_TEST_STATUS
EFIAPI
TestTbsHashSha384 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPositiveTbsHashCase (&mTbsSha384Guid, SHA384_DIGEST_SIZE);
}

/**
  SHA-512 TBSCertificate digest is computed correctly.
**/
UNIT_TEST_STATUS
EFIAPI
TestTbsHashSha512 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPositiveTbsHashCase (&mTbsSha512Guid, SHA512_DIGEST_SIZE);
}

/**
  NULL pointers and a zero CertSize each yield EFI_INVALID_PARAMETER.
**/
UNIT_TEST_STATUS
EFIAPI
TestTbsHashBadParameters (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Digest[SHA512_DIGEST_SIZE];
  UINTN       DigestSize;

  DigestSize = 0;

  //
  // NULL Cert.
  //
  Status = X509GetTbsCertHash (NULL, sizeof (mTbsHashTestCert), &mTbsSha256Guid, Digest, &DigestSize);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // Zero CertSize.
  //
  Status = X509GetTbsCertHash ((VOID *)mTbsHashTestCert, 0, &mTbsSha256Guid, Digest, &DigestSize);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // NULL HashType.
  //
  Status = X509GetTbsCertHash ((VOID *)mTbsHashTestCert, sizeof (mTbsHashTestCert), NULL, Digest, &DigestSize);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // NULL Digest.
  //
  Status = X509GetTbsCertHash ((VOID *)mTbsHashTestCert, sizeof (mTbsHashTestCert), &mTbsSha256Guid, NULL, &DigestSize);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // NULL DigestSize.
  //
  Status = X509GetTbsCertHash ((VOID *)mTbsHashTestCert, sizeof (mTbsHashTestCert), &mTbsSha256Guid, Digest, NULL);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  An unrecognized HashType GUID yields EFI_UNSUPPORTED.
**/
UNIT_TEST_STATUS
EFIAPI
TestTbsHashUnsupportedAlgorithm (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Digest[SHA512_DIGEST_SIZE];
  UINTN       DigestSize;

  DigestSize = 0;
  Status     = X509GetTbsCertHash (
                 (VOID *)mTbsHashTestCert,
                 sizeof (mTbsHashTestCert),
                 &mTbsBogusGuid,
                 Digest,
                 &DigestSize
                 );
  UT_ASSERT_EQUAL (Status, EFI_UNSUPPORTED);

  return UNIT_TEST_PASSED;
}

/**
  A buffer that is not a well-formed X.509 certificate yields
  EFI_INVALID_PARAMETER.
**/
UNIT_TEST_STATUS
EFIAPI
TestTbsHashMalformedCert (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Garbage[16];
  UINT8       Digest[SHA512_DIGEST_SIZE];
  UINTN       DigestSize;

  SetMem (Garbage, sizeof (Garbage), 0xAB);

  DigestSize = 0;
  Status     = X509GetTbsCertHash (
                 Garbage,
                 sizeof (Garbage),
                 &mTbsSha256Guid,
                 Digest,
                 &DigestSize
                 );
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mX509GetTbsCertHashTest[] = {
  //
  // -----Description-------------------------------------Class--------------------------------------Function-------------------------Pre--Post-Context
  //
  { "SHA1 TBSCertificate digest",              "CryptoPkg.BaseCryptLib.X509GetTbsCertHash", TestTbsHashSha1,               NULL, NULL, NULL },
  { "SHA256 TBSCertificate digest",            "CryptoPkg.BaseCryptLib.X509GetTbsCertHash", TestTbsHashSha256,             NULL, NULL, NULL },
  { "SHA384 TBSCertificate digest",            "CryptoPkg.BaseCryptLib.X509GetTbsCertHash", TestTbsHashSha384,             NULL, NULL, NULL },
  { "SHA512 TBSCertificate digest",            "CryptoPkg.BaseCryptLib.X509GetTbsCertHash", TestTbsHashSha512,             NULL, NULL, NULL },
  { "Bad parameters return INVALID_PARAMETER", "CryptoPkg.BaseCryptLib.X509GetTbsCertHash", TestTbsHashBadParameters,      NULL, NULL, NULL },
  { "Unsupported algorithm returns UNSUPPORTED", "CryptoPkg.BaseCryptLib.X509GetTbsCertHash", TestTbsHashUnsupportedAlgorithm, NULL, NULL, NULL },
  { "Malformed certificate rejected",          "CryptoPkg.BaseCryptLib.X509GetTbsCertHash", TestTbsHashMalformedCert,      NULL, NULL, NULL },
};

UINTN  mX509GetTbsCertHashTestNum = ARRAY_SIZE (mX509GetTbsCertHashTest);
