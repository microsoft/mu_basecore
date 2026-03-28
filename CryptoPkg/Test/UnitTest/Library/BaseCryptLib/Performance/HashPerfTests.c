/** @file
  Hash performance benchmarks for BaseCryptLib.

  Benchmarks SHA-256, SHA-384, and SHA-512 bulk hashing throughput
  using HashAll convenience functions on 1 MB data buffers.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptoPerfLib.h"

UNIT_TEST_STATUS
EFIAPI
TestPerfSha256 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *DataBuffer;
  UINT8    Digest[SHA256_DIGEST_SIZE];
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  DataBuffer = AllocatePool (PERF_DATA_SIZE);
  UT_ASSERT_NOT_NULL (DataBuffer);
  SetMem (DataBuffer, PERF_DATA_SIZE, 0xA5);

  //
  // Verify the operation works before benchmarking.
  //
  Status = Sha256HashAll (DataBuffer, PERF_DATA_SIZE, Digest);
  if (!Status) {
    UT_LOG_WARNING ("SHA-256 not supported, skipping benchmark.");
    FreePool (DataBuffer);
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < HASH_ITERATIONS; Index++) {
    Sha256HashAll (DataBuffer, PERF_DATA_SIZE, Digest);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "SHA-256 HashAll",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    HASH_ITERATIONS,
    PERF_DATA_SIZE
    );

  FreePool (DataBuffer);
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestPerfSha384 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *DataBuffer;
  UINT8    Digest[SHA384_DIGEST_SIZE];
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  DataBuffer = AllocatePool (PERF_DATA_SIZE);
  UT_ASSERT_NOT_NULL (DataBuffer);
  SetMem (DataBuffer, PERF_DATA_SIZE, 0xA5);

  Status = Sha384HashAll (DataBuffer, PERF_DATA_SIZE, Digest);
  if (!Status) {
    UT_LOG_WARNING ("SHA-384 not supported, skipping benchmark.");
    FreePool (DataBuffer);
    return UNIT_TEST_PASSED;
  }

  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < HASH_ITERATIONS; Index++) {
    Sha384HashAll (DataBuffer, PERF_DATA_SIZE, Digest);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "SHA-384 HashAll",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    HASH_ITERATIONS,
    PERF_DATA_SIZE
    );

  FreePool (DataBuffer);
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestPerfSha512 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *DataBuffer;
  UINT8    Digest[SHA512_DIGEST_SIZE];
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  DataBuffer = AllocatePool (PERF_DATA_SIZE);
  UT_ASSERT_NOT_NULL (DataBuffer);
  SetMem (DataBuffer, PERF_DATA_SIZE, 0xA5);

  Status = Sha512HashAll (DataBuffer, PERF_DATA_SIZE, Digest);
  if (!Status) {
    UT_LOG_WARNING ("SHA-512 not supported, skipping benchmark.");
    FreePool (DataBuffer);
    return UNIT_TEST_PASSED;
  }

  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < HASH_ITERATIONS; Index++) {
    Sha512HashAll (DataBuffer, PERF_DATA_SIZE, Digest);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "SHA-512 HashAll",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    HASH_ITERATIONS,
    PERF_DATA_SIZE
    );

  FreePool (DataBuffer);
  return UNIT_TEST_PASSED;
}

PERF_TEST_DESC  mHashPerfTest[] = {
  //
  // -----Description--------Class--------------------------------------Function---------Pre--Post--Context
  //
  { "PerfSha256()", "CryptoPkg.BaseCryptLib.Perf.Hash", TestPerfSha256, NULL, NULL, NULL },
  { "PerfSha384()", "CryptoPkg.BaseCryptLib.Perf.Hash", TestPerfSha384, NULL, NULL, NULL },
  { "PerfSha512()", "CryptoPkg.BaseCryptLib.Perf.Hash", TestPerfSha512, NULL, NULL, NULL },
};

UINTN  mHashPerfTestNum = ARRAY_SIZE (mHashPerfTest);
