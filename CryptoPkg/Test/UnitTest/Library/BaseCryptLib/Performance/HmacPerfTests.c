/** @file
  HMAC performance benchmarks for BaseCryptLib.

  Benchmarks HMAC-SHA256 throughput on 1 MB data buffers.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptoPerfLib.h"

STATIC CONST UINT8  mHmacPerfKey[32] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
};

UNIT_TEST_STATUS
EFIAPI
TestPerfHmacSha256 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *HmacCtx;
  UINT8    *DataBuffer;
  UINT8    HmacValue[SHA256_DIGEST_SIZE];
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  DataBuffer = AllocatePool (PERF_DATA_SIZE);
  UT_ASSERT_NOT_NULL (DataBuffer);
  SetMem (DataBuffer, PERF_DATA_SIZE, 0xA5);

  HmacCtx = HmacSha256New ();
  if (HmacCtx == NULL) {
    UT_LOG_WARNING ("HMAC-SHA256 not supported, skipping benchmark.");
    FreePool (DataBuffer);
    return UNIT_TEST_PASSED;
  }

  //
  // Verify it works
  //
  Status = HmacSha256SetKey (HmacCtx, mHmacPerfKey, sizeof (mHmacPerfKey));
  if (!Status) {
    UT_LOG_WARNING ("HMAC-SHA256 SetKey not supported, skipping benchmark.");
    HmacSha256Free (HmacCtx);
    FreePool (DataBuffer);
    return UNIT_TEST_PASSED;
  }

  Status = HmacSha256Update (HmacCtx, DataBuffer, PERF_DATA_SIZE);
  UT_ASSERT_TRUE (Status);
  Status = HmacSha256Final (HmacCtx, HmacValue);
  UT_ASSERT_TRUE (Status);

  //
  // Benchmark: SetKey + Update + Final per iteration
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < HMAC_ITERATIONS; Index++) {
    HmacSha256SetKey (HmacCtx, mHmacPerfKey, sizeof (mHmacPerfKey));
    HmacSha256Update (HmacCtx, DataBuffer, PERF_DATA_SIZE);
    HmacSha256Final (HmacCtx, HmacValue);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "HMAC-SHA256",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    HMAC_ITERATIONS,
    PERF_DATA_SIZE
    );

  HmacSha256Free (HmacCtx);
  FreePool (DataBuffer);
  return UNIT_TEST_PASSED;
}

PERF_TEST_DESC  mHmacPerfTest[] = {
  //
  // -----Description-----------Class---------------------------------------Function-----------Pre--Post--Context
  //
  { "PerfHmacSha256()", "CryptoPkg.BaseCryptLib.Perf.Hmac", TestPerfHmacSha256, NULL, NULL, NULL },
};

UINTN  mHmacPerfTestNum = ARRAY_SIZE (mHmacPerfTest);
