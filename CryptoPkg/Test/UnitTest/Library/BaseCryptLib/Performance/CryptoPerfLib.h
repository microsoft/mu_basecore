/** @file
  Header for BaseCryptLib Performance Tests.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CRYPTO_PERF_LIB_H_
#define CRYPTO_PERF_LIB_H_

#include <PiPei.h>
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseCryptLib.h>

#define PERF_TEST_NAME     "BaseCryptLib Performance Test"
#define PERF_TEST_VERSION  "1.0"

//
// Standard benchmark data size (1 MB).
//
#define PERF_DATA_SIZE  SIZE_1MB

//
// Iteration counts for each operation category.
//
#define HASH_ITERATIONS          100
#define SYMMETRIC_ITERATIONS     100
#define HMAC_ITERATIONS          100
#define RSA_KEYGEN_ITERATIONS     10
#define RSA_SIGN_ITERATIONS       50
#define RSA_VERIFY_ITERATIONS    100
#define EC_ITERATIONS            100

/**
  Compute elapsed nanoseconds between two performance counter readings.
  Handles counters that count up or down.

  @param[in]  StartTick  Performance counter value at start.
  @param[in]  EndTick    Performance counter value at end.

  @return  Elapsed time in nanoseconds.
**/
UINT64
PerfElapsedNanoSeconds (
  IN UINT64  StartTick,
  IN UINT64  EndTick
  );

/**
  Log a performance measurement result via DEBUG output.

  For bulk data operations (DataSize > 0), reports throughput in MB/s.
  For discrete operations (DataSize == 0), reports ops/sec.

  @param[in]  Label       Description of the operation benchmarked.
  @param[in]  ElapsedNs   Elapsed time in nanoseconds.
  @param[in]  Iterations  Number of iterations performed.
  @param[in]  DataSize    Bytes processed per iteration (0 for asymmetric ops).
**/
VOID
PerfLogResult (
  IN CONST CHAR8  *Label,
  IN UINT64       ElapsedNs,
  IN UINTN        Iterations,
  IN UINTN        DataSize
  );

//
// Test descriptor types (mirrors existing TEST_DESC / SUITE_DESC pattern).
//
typedef struct {
  CHAR8                     *Description;
  CHAR8                     *ClassName;
  UNIT_TEST_FUNCTION        Func;
  UNIT_TEST_PREREQUISITE    PreReq;
  UNIT_TEST_CLEANUP         CleanUp;
  UNIT_TEST_CONTEXT         Context;
} PERF_TEST_DESC;

typedef struct {
  CHAR8                       *Title;
  CHAR8                       *Package;
  UNIT_TEST_SUITE_SETUP       Sup;
  UNIT_TEST_SUITE_TEARDOWN    Tdn;
  UINTN                       *TestNum;
  PERF_TEST_DESC              *TestDesc;
} PERF_SUITE_DESC;

//
// Test suite externs from individual test files.
//
extern UINTN           mHashPerfTestNum;
extern PERF_TEST_DESC  mHashPerfTest[];

extern UINTN           mAesPerfTestNum;
extern PERF_TEST_DESC  mAesPerfTest[];

extern UINTN           mHmacPerfTestNum;
extern PERF_TEST_DESC  mHmacPerfTest[];

extern UINTN           mRsaPerfTestNum;
extern PERF_TEST_DESC  mRsaPerfTest[];

extern UINTN           mEcPerfTestNum;
extern PERF_TEST_DESC  mEcPerfTest[];

/**
  Create the performance test framework and register all test suites.
**/
EFI_STATUS
EFIAPI
CreatePerfTest (
  IN     CHAR8                       *UnitTestName,
  IN     CHAR8                       *UnitTestVersion,
  IN OUT UNIT_TEST_FRAMEWORK_HANDLE  *Framework
  );

/**
  Main entry point for the performance tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  );

#endif // CRYPTO_PERF_LIB_H_
