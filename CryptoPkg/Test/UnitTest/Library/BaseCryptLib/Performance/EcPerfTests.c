/** @file
  EC performance benchmarks for BaseCryptLib.

  Benchmarks EC P-256 key generation, ECDH key exchange,
  and ECDSA sign/verify operations.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptoPerfLib.h"

//
// P-256 sizes
//
#define EC_P256_PUB_KEY_SIZE  64
#define EC_P256_HALF_SIZE     32
#define EC_P256_SIG_SIZE      64

UNIT_TEST_STATUS
EFIAPI
TestPerfEcGenerateKey (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *Ec;
  UINT8    PublicKey[EC_P256_PUB_KEY_SIZE];
  UINTN    PublicKeySize;
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  Status = RandomSeed (NULL, 0);
  if (!Status) {
    UT_LOG_WARNING ("RandomSeed failed, skipping EC keygen benchmark.");
    return UNIT_TEST_PASSED;
  }

  //
  // Verify it works
  //
  Ec = EcNewByNid (CRYPTO_NID_SECP256R1);
  if (Ec == NULL) {
    UT_LOG_WARNING ("EcNewByNid P-256 not supported, skipping benchmark.");
    return UNIT_TEST_PASSED;
  }

  PublicKeySize = sizeof (PublicKey);
  Status        = EcGenerateKey (Ec, PublicKey, &PublicKeySize);
  EcFree (Ec);

  if (!Status) {
    UT_LOG_WARNING ("EcGenerateKey not supported, skipping benchmark.");
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < EC_ITERATIONS; Index++) {
    Ec            = EcNewByNid (CRYPTO_NID_SECP256R1);
    PublicKeySize = sizeof (PublicKey);
    EcGenerateKey (Ec, PublicKey, &PublicKeySize);
    EcFree (Ec);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "EC P-256 KeyGen",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    EC_ITERATIONS,
    0
    );

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestPerfEcDhCompute (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *Ec1;
  VOID     *Ec2;
  UINT8    Public1[EC_P256_PUB_KEY_SIZE];
  UINTN    Public1Size;
  UINT8    Public2[EC_P256_PUB_KEY_SIZE];
  UINTN    Public2Size;
  UINT8    Key[EC_P256_HALF_SIZE];
  UINTN    KeySize;
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  Status = RandomSeed (NULL, 0);
  if (!Status) {
    UT_LOG_WARNING ("RandomSeed failed, skipping ECDH benchmark.");
    return UNIT_TEST_PASSED;
  }

  Ec1 = EcNewByNid (CRYPTO_NID_SECP256R1);
  Ec2 = EcNewByNid (CRYPTO_NID_SECP256R1);
  if ((Ec1 == NULL) || (Ec2 == NULL)) {
    UT_LOG_WARNING ("EcNewByNid P-256 not supported, skipping ECDH benchmark.");
    EcFree (Ec1);
    EcFree (Ec2);
    return UNIT_TEST_PASSED;
  }

  Public1Size = sizeof (Public1);
  Status      = EcGenerateKey (Ec1, Public1, &Public1Size);
  if (!Status) {
    UT_LOG_WARNING ("EcGenerateKey failed, skipping ECDH benchmark.");
    EcFree (Ec1);
    EcFree (Ec2);
    return UNIT_TEST_PASSED;
  }

  Public2Size = sizeof (Public2);
  Status      = EcGenerateKey (Ec2, Public2, &Public2Size);
  UT_ASSERT_TRUE (Status);

  //
  // Verify ECDH works
  //
  KeySize = sizeof (Key);
  Status  = EcDhComputeKey (Ec1, Public2, Public2Size, NULL, Key, &KeySize);
  if (!Status) {
    UT_LOG_WARNING ("EcDhComputeKey not supported, skipping ECDH benchmark.");
    EcFree (Ec1);
    EcFree (Ec2);
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark: compute shared key repeatedly
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < EC_ITERATIONS; Index++) {
    KeySize = sizeof (Key);
    EcDhComputeKey (Ec1, Public2, Public2Size, NULL, Key, &KeySize);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "ECDH P-256 Compute",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    EC_ITERATIONS,
    0
    );

  EcFree (Ec1);
  EcFree (Ec2);
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestPerfEcDsaSign (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *Ec;
  UINT8    PublicKey[EC_P256_PUB_KEY_SIZE];
  UINTN    PublicKeySize;
  UINT8    HashValue[SHA256_DIGEST_SIZE];
  UINT8    Signature[EC_P256_SIG_SIZE];
  UINTN    SigSize;
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  Status = RandomSeed (NULL, 0);
  if (!Status) {
    UT_LOG_WARNING ("RandomSeed failed, skipping ECDSA sign benchmark.");
    return UNIT_TEST_PASSED;
  }

  Ec = EcNewByNid (CRYPTO_NID_SECP256R1);
  if (Ec == NULL) {
    UT_LOG_WARNING ("EcNewByNid P-256 not supported, skipping ECDSA sign benchmark.");
    return UNIT_TEST_PASSED;
  }

  PublicKeySize = sizeof (PublicKey);
  Status        = EcGenerateKey (Ec, PublicKey, &PublicKeySize);
  if (!Status) {
    UT_LOG_WARNING ("EcGenerateKey failed, skipping ECDSA sign benchmark.");
    EcFree (Ec);
    return UNIT_TEST_PASSED;
  }

  SetMem (HashValue, sizeof (HashValue), 0x42);

  //
  // Verify sign works
  //
  SigSize = sizeof (Signature);
  Status  = EcDsaSign (Ec, CRYPTO_NID_SHA256, HashValue, sizeof (HashValue), Signature, &SigSize);
  if (!Status) {
    UT_LOG_WARNING ("EcDsaSign not supported, skipping benchmark.");
    EcFree (Ec);
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < EC_ITERATIONS; Index++) {
    SigSize = sizeof (Signature);
    EcDsaSign (Ec, CRYPTO_NID_SHA256, HashValue, sizeof (HashValue), Signature, &SigSize);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "ECDSA P-256 Sign",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    EC_ITERATIONS,
    0
    );

  EcFree (Ec);
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestPerfEcDsaVerify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *Ec;
  UINT8    PublicKey[EC_P256_PUB_KEY_SIZE];
  UINTN    PublicKeySize;
  UINT8    HashValue[SHA256_DIGEST_SIZE];
  UINT8    Signature[EC_P256_SIG_SIZE];
  UINTN    SigSize;
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  Status = RandomSeed (NULL, 0);
  if (!Status) {
    UT_LOG_WARNING ("RandomSeed failed, skipping ECDSA verify benchmark.");
    return UNIT_TEST_PASSED;
  }

  Ec = EcNewByNid (CRYPTO_NID_SECP256R1);
  if (Ec == NULL) {
    UT_LOG_WARNING ("EcNewByNid P-256 not supported, skipping ECDSA verify benchmark.");
    return UNIT_TEST_PASSED;
  }

  PublicKeySize = sizeof (PublicKey);
  Status        = EcGenerateKey (Ec, PublicKey, &PublicKeySize);
  if (!Status) {
    UT_LOG_WARNING ("EcGenerateKey failed, skipping ECDSA verify benchmark.");
    EcFree (Ec);
    return UNIT_TEST_PASSED;
  }

  SetMem (HashValue, sizeof (HashValue), 0x42);

  //
  // Create a signature to verify
  //
  SigSize = sizeof (Signature);
  Status  = EcDsaSign (Ec, CRYPTO_NID_SHA256, HashValue, sizeof (HashValue), Signature, &SigSize);
  if (!Status) {
    UT_LOG_WARNING ("EcDsaSign failed, skipping ECDSA verify benchmark.");
    EcFree (Ec);
    return UNIT_TEST_PASSED;
  }

  //
  // Verify it works
  //
  Status = EcDsaVerify (Ec, CRYPTO_NID_SHA256, HashValue, sizeof (HashValue), Signature, SigSize);
  if (!Status) {
    UT_LOG_WARNING ("EcDsaVerify not supported, skipping benchmark.");
    EcFree (Ec);
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < EC_ITERATIONS; Index++) {
    EcDsaVerify (Ec, CRYPTO_NID_SHA256, HashValue, sizeof (HashValue), Signature, SigSize);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "ECDSA P-256 Verify",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    EC_ITERATIONS,
    0
    );

  EcFree (Ec);
  return UNIT_TEST_PASSED;
}

PERF_TEST_DESC  mEcPerfTest[] = {
  //
  // -----Description--------------Class---------------------------------Function--------------Pre--Post--Context
  //
  { "PerfEcGenerateKey()", "CryptoPkg.BaseCryptLib.Perf.Ec", TestPerfEcGenerateKey, NULL, NULL, NULL },
  { "PerfEcDhCompute()",  "CryptoPkg.BaseCryptLib.Perf.Ec", TestPerfEcDhCompute,  NULL, NULL, NULL },
  { "PerfEcDsaSign()",    "CryptoPkg.BaseCryptLib.Perf.Ec", TestPerfEcDsaSign,    NULL, NULL, NULL },
  { "PerfEcDsaVerify()",  "CryptoPkg.BaseCryptLib.Perf.Ec", TestPerfEcDsaVerify,  NULL, NULL, NULL },
};

UINTN  mEcPerfTestNum = ARRAY_SIZE (mEcPerfTest);
