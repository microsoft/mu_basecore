/** @file
  RSA performance benchmarks for BaseCryptLib.

  Benchmarks RSA-2048 key generation, PKCS1v1.5 sign, and verify.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptoPerfLib.h"

#define RSA_PERF_MODULUS_LENGTH  2048

STATIC CONST UINT8  mRsaPerfPublicExponent[] = { 0x01, 0x00, 0x01 };

UNIT_TEST_STATUS
EFIAPI
TestPerfRsaGenerateKey (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *Rsa;
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  Status = RandomSeed (NULL, 0);
  if (!Status) {
    UT_LOG_WARNING ("RandomSeed failed, skipping RSA keygen benchmark.");
    return UNIT_TEST_PASSED;
  }

  //
  // Verify RSA keygen works
  //
  Rsa = RsaNew ();
  if (Rsa == NULL) {
    UT_LOG_WARNING ("RsaNew not supported, skipping benchmark.");
    return UNIT_TEST_PASSED;
  }

  Status = RsaGenerateKey (
             Rsa,
             RSA_PERF_MODULUS_LENGTH,
             mRsaPerfPublicExponent,
             sizeof (mRsaPerfPublicExponent)
             );
  RsaFree (Rsa);

  if (!Status) {
    UT_LOG_WARNING ("RsaGenerateKey not supported for 2048-bit, skipping.");
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < RSA_KEYGEN_ITERATIONS; Index++) {
    Rsa = RsaNew ();
    RsaGenerateKey (
      Rsa,
      RSA_PERF_MODULUS_LENGTH,
      mRsaPerfPublicExponent,
      sizeof (mRsaPerfPublicExponent)
      );
    RsaFree (Rsa);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "RSA-2048 KeyGen",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    RSA_KEYGEN_ITERATIONS,
    0
    );

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestPerfRsaPkcs1Sign (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *Rsa;
  UINT8    HashValue[SHA256_DIGEST_SIZE];
  UINT8    Signature[RSA_PERF_MODULUS_LENGTH / 8];
  UINTN    SigSize;
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  Status = RandomSeed (NULL, 0);
  if (!Status) {
    UT_LOG_WARNING ("RandomSeed failed, skipping RSA sign benchmark.");
    return UNIT_TEST_PASSED;
  }

  Rsa = RsaNew ();
  UT_ASSERT_NOT_NULL (Rsa);

  Status = RsaGenerateKey (
             Rsa,
             RSA_PERF_MODULUS_LENGTH,
             mRsaPerfPublicExponent,
             sizeof (mRsaPerfPublicExponent)
             );
  if (!Status) {
    UT_LOG_WARNING ("RsaGenerateKey failed, skipping RSA sign benchmark.");
    RsaFree (Rsa);
    return UNIT_TEST_PASSED;
  }

  SetMem (HashValue, sizeof (HashValue), 0x42);

  //
  // Verify sign works
  //
  SigSize = sizeof (Signature);
  Status  = RsaPkcs1Sign (Rsa, HashValue, SHA256_DIGEST_SIZE, Signature, &SigSize);
  if (!Status) {
    UT_LOG_WARNING ("RsaPkcs1Sign not supported, skipping benchmark.");
    RsaFree (Rsa);
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < RSA_SIGN_ITERATIONS; Index++) {
    SigSize = sizeof (Signature);
    RsaPkcs1Sign (Rsa, HashValue, SHA256_DIGEST_SIZE, Signature, &SigSize);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "RSA-2048 PKCS1v1.5 Sign",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    RSA_SIGN_ITERATIONS,
    0
    );

  RsaFree (Rsa);
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestPerfRsaPkcs1Verify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *Rsa;
  UINT8    HashValue[SHA256_DIGEST_SIZE];
  UINT8    Signature[RSA_PERF_MODULUS_LENGTH / 8];
  UINTN    SigSize;
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  Status = RandomSeed (NULL, 0);
  if (!Status) {
    UT_LOG_WARNING ("RandomSeed failed, skipping RSA verify benchmark.");
    return UNIT_TEST_PASSED;
  }

  Rsa = RsaNew ();
  UT_ASSERT_NOT_NULL (Rsa);

  Status = RsaGenerateKey (
             Rsa,
             RSA_PERF_MODULUS_LENGTH,
             mRsaPerfPublicExponent,
             sizeof (mRsaPerfPublicExponent)
             );
  if (!Status) {
    UT_LOG_WARNING ("RsaGenerateKey failed, skipping RSA verify benchmark.");
    RsaFree (Rsa);
    return UNIT_TEST_PASSED;
  }

  SetMem (HashValue, sizeof (HashValue), 0x42);

  //
  // Create a signature to verify
  //
  SigSize = sizeof (Signature);
  Status  = RsaPkcs1Sign (Rsa, HashValue, SHA256_DIGEST_SIZE, Signature, &SigSize);
  if (!Status) {
    UT_LOG_WARNING ("RsaPkcs1Sign failed, skipping RSA verify benchmark.");
    RsaFree (Rsa);
    return UNIT_TEST_PASSED;
  }

  //
  // Verify it works
  //
  Status = RsaPkcs1Verify (Rsa, HashValue, SHA256_DIGEST_SIZE, Signature, SigSize);
  if (!Status) {
    UT_LOG_WARNING ("RsaPkcs1Verify not supported, skipping benchmark.");
    RsaFree (Rsa);
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < RSA_VERIFY_ITERATIONS; Index++) {
    RsaPkcs1Verify (Rsa, HashValue, SHA256_DIGEST_SIZE, Signature, SigSize);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "RSA-2048 PKCS1v1.5 Verify",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    RSA_VERIFY_ITERATIONS,
    0
    );

  RsaFree (Rsa);
  return UNIT_TEST_PASSED;
}

PERF_TEST_DESC  mRsaPerfTest[] = {
  //
  // -----Description----------------Class----------------------------------Function-------------------Pre--Post--Context
  //
  { "PerfRsaGenerateKey()",  "CryptoPkg.BaseCryptLib.Perf.Rsa", TestPerfRsaGenerateKey,  NULL, NULL, NULL },
  { "PerfRsaPkcs1Sign()",   "CryptoPkg.BaseCryptLib.Perf.Rsa", TestPerfRsaPkcs1Sign,   NULL, NULL, NULL },
  { "PerfRsaPkcs1Verify()", "CryptoPkg.BaseCryptLib.Perf.Rsa", TestPerfRsaPkcs1Verify, NULL, NULL, NULL },
};

UINTN  mRsaPerfTestNum = ARRAY_SIZE (mRsaPerfTest);
