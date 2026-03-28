/** @file
  AES performance benchmarks for BaseCryptLib.

  Benchmarks AES-256-CBC and AES-256-GCM encryption throughput
  on 1 MB data buffers.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptoPerfLib.h"

//
// AES-256 test key (32 bytes)
//
STATIC CONST UINT8  mAesPerfKey[32] = {
  0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
  0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
  0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
  0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

//
// AES-CBC initialization vector (16 bytes)
//
STATIC CONST UINT8  mAesPerfIv[16] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

//
// AES-GCM initialization vector (12 bytes)
//
STATIC CONST UINT8  mGcmPerfIv[12] = {
  0x99, 0xaa, 0x3e, 0x68, 0xed, 0x81, 0x73, 0xa0,
  0xee, 0xd0, 0x66, 0x84
};

UNIT_TEST_STATUS
EFIAPI
TestPerfAesCbcEncrypt (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID     *AesContext;
  UINT8    *DataBuffer;
  UINT8    *OutBuffer;
  UINT8    Iv[16];
  BOOLEAN  Status;
  UINTN    Index;
  UINTN    CtxSize;
  UINT64   StartTick;
  UINT64   EndTick;

  CtxSize = AesGetContextSize ();
  if (CtxSize == 0) {
    UT_LOG_WARNING ("AES not supported, skipping benchmark.");
    return UNIT_TEST_PASSED;
  }

  AesContext = AllocatePool (CtxSize);
  UT_ASSERT_NOT_NULL (AesContext);

  DataBuffer = AllocatePool (PERF_DATA_SIZE);
  UT_ASSERT_NOT_NULL (DataBuffer);

  OutBuffer = AllocatePool (PERF_DATA_SIZE);
  UT_ASSERT_NOT_NULL (OutBuffer);

  SetMem (DataBuffer, PERF_DATA_SIZE, 0xA5);

  Status = AesInit (AesContext, mAesPerfKey, 256);
  UT_ASSERT_TRUE (Status);

  //
  // Verify it works
  //
  CopyMem (Iv, mAesPerfIv, sizeof (Iv));
  Status = AesCbcEncrypt (AesContext, DataBuffer, PERF_DATA_SIZE, Iv, OutBuffer);
  if (!Status) {
    UT_LOG_WARNING ("AES-CBC Encrypt not supported, skipping benchmark.");
    FreePool (AesContext);
    FreePool (DataBuffer);
    FreePool (OutBuffer);
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < SYMMETRIC_ITERATIONS; Index++) {
    CopyMem (Iv, mAesPerfIv, sizeof (Iv));
    AesCbcEncrypt (AesContext, DataBuffer, PERF_DATA_SIZE, Iv, OutBuffer);
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "AES-256-CBC Encrypt",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    SYMMETRIC_ITERATIONS,
    PERF_DATA_SIZE
    );

  FreePool (AesContext);
  FreePool (DataBuffer);
  FreePool (OutBuffer);
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestPerfAeadAesGcmEncrypt (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *DataBuffer;
  UINT8    *OutBuffer;
  UINT8    Tag[16];
  UINTN    OutSize;
  BOOLEAN  Status;
  UINTN    Index;
  UINT64   StartTick;
  UINT64   EndTick;

  DataBuffer = AllocatePool (PERF_DATA_SIZE);
  UT_ASSERT_NOT_NULL (DataBuffer);

  OutBuffer = AllocatePool (PERF_DATA_SIZE);
  UT_ASSERT_NOT_NULL (OutBuffer);

  SetMem (DataBuffer, PERF_DATA_SIZE, 0xA5);

  //
  // Verify it works
  //
  OutSize = PERF_DATA_SIZE;
  Status  = AeadAesGcmEncrypt (
              mAesPerfKey,
              sizeof (mAesPerfKey),
              mGcmPerfIv,
              sizeof (mGcmPerfIv),
              NULL,
              0,
              DataBuffer,
              PERF_DATA_SIZE,
              Tag,
              sizeof (Tag),
              OutBuffer,
              &OutSize
              );
  if (!Status) {
    UT_LOG_WARNING ("AES-GCM Encrypt not supported, skipping benchmark.");
    FreePool (DataBuffer);
    FreePool (OutBuffer);
    return UNIT_TEST_PASSED;
  }

  //
  // Benchmark
  //
  StartTick = GetPerformanceCounter ();
  for (Index = 0; Index < SYMMETRIC_ITERATIONS; Index++) {
    OutSize = PERF_DATA_SIZE;
    AeadAesGcmEncrypt (
      mAesPerfKey,
      sizeof (mAesPerfKey),
      mGcmPerfIv,
      sizeof (mGcmPerfIv),
      NULL,
      0,
      DataBuffer,
      PERF_DATA_SIZE,
      Tag,
      sizeof (Tag),
      OutBuffer,
      &OutSize
      );
  }

  EndTick = GetPerformanceCounter ();

  PerfLogResult (
    "AES-256-GCM Encrypt",
    PerfElapsedNanoSeconds (StartTick, EndTick),
    SYMMETRIC_ITERATIONS,
    PERF_DATA_SIZE
    );

  FreePool (DataBuffer);
  FreePool (OutBuffer);
  return UNIT_TEST_PASSED;
}

PERF_TEST_DESC  mAesPerfTest[] = {
  //
  // -----Description------------------Class-------------------------------------Function---------------------Pre--Post--Context
  //
  { "PerfAesCbcEncrypt()",     "CryptoPkg.BaseCryptLib.Perf.Aes", TestPerfAesCbcEncrypt,     NULL, NULL, NULL },
  { "PerfAeadAesGcmEncrypt()", "CryptoPkg.BaseCryptLib.Perf.Aes", TestPerfAeadAesGcmEncrypt, NULL, NULL, NULL },
};

UINTN  mAesPerfTestNum = ARRAY_SIZE (mAesPerfTest);
