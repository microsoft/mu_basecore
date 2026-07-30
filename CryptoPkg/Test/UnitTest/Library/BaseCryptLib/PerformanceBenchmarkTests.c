/** @file
  Performance benchmarks for the Authenticode verification BaseCryptLib
  APIs. Compares a plain AuthenticodeVerify() (verify only) against
  AuthenticodeVerifyEx() (verify AND return the signer certificate chain),
  for both the "root is the trust anchor" and "signer is the trust anchor"
  cases.

  Motivation: AuthenticodeVerifyEx() returns the verified signer chain that
  falls out of the single verification OpenSSL already performs. This suite
  quantifies its incremental cost over AuthenticodeVerify(), confirming the
  chain is nearly free versus a second, independent chain-building pass.

  Disabled by default. Define ENABLE_PERF_BENCHMARKS to build and register
  the suite (the application INF does this in [BuildOptions]).

  Measurement notes:
    * Timing uses the platform performance counter (TimerLib). On QEMU Q35
      that is the 24-bit ACPI PM timer (~3.58 MHz), which wraps roughly
      every 4.7 s, so each measured call stays well under that period and
      the tick delta is computed wrap-aware.
    * Every crypto call crosses into MM (OneCrypto) and, in a DEBUG /
      emulated build, costs on the order of hundreds of milliseconds with
      significant run-to-run jitter. We therefore report the MINIMUM
      per-call time over several iterations (least perturbed by scheduling
      / SMI latency) alongside the mean.

  These benchmarks never assert on timing (hardware / emulator dependent);
  they assert only that each underlying call still succeeds, and always
  report their measurements.

Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"

#ifdef ENABLE_PERF_BENCHMARKS

  #include <Library/TimerLib.h>

//
// Timed iterations per benchmark. Each crypto call crosses into MM and, in
// a DEBUG/emulated build, costs hundreds of milliseconds; the counter still
// resolves a single call to sub-microsecond precision, so a handful of
// iterations is enough to find a stable minimum while keeping the run short.
//
#define PERF_ITERATIONS  8

//
// Authenticode sample (SignedData, trust anchor, PE/COFF image hash),
// exported by AuthenticodeTests.c.
//
extern CONST UINT8  *mPerfAuthData;
extern CONST UINTN  mPerfAuthDataSize;
extern CONST UINT8  *mPerfAuthAnchor;
extern CONST UINTN  mPerfAuthAnchorSize;
extern CONST UINT8  *mPerfAuthImageHash;
extern CONST UINTN  mPerfAuthImageHashSize;

//
// Minimum per-call nanoseconds captured by each benchmark, consumed by the
// summary for the ratios.
//
STATIC UINT64  mNsVerify     = 0;
STATIC UINT64  mNsVerifyExRt = 0;
STATIC UINT64  mNsVerifyExSg = 0;

//
// Cached performance-counter range/direction for wrap-aware delta math.
//
STATIC UINT64   mCounterStart       = 0;
STATIC UINT64   mCounterEnd         = 0;
STATIC BOOLEAN  mCounterCountsUp    = TRUE;
STATIC BOOLEAN  mCounterInitialized = FALSE;

/**
  Cache the performance-counter range/direction once.
**/
STATIC
VOID
PerfInitCounter (
  VOID
  )
{
  if (!mCounterInitialized) {
    GetPerformanceCounterProperties (&mCounterStart, &mCounterEnd);
    mCounterCountsUp    = (BOOLEAN)(mCounterEnd >= mCounterStart);
    mCounterInitialized = TRUE;
  }
}

/**
  Wrap-aware elapsed ticks between an earlier read (First) and a later read
  (Second). Handles a single wrap of the (possibly 24-bit) counter in either
  direction; callers keep each measured interval shorter than the counter
  period so at most one wrap can occur.

  @param[in]  First   Counter value read before the work.
  @param[in]  Second  Counter value read after the work.

  @return  Elapsed ticks.
**/
STATIC
UINT64
ElapsedTicks (
  IN UINT64  First,
  IN UINT64  Second
  )
{
  if (mCounterCountsUp) {
    if (Second >= First) {
      return Second - First;
    }

    return (mCounterEnd - First) + (Second - mCounterStart) + 1;
  } else {
    if (First >= Second) {
      return First - Second;
    }

    return (First - mCounterEnd) + (mCounterStart - Second) + 1;
  }
}

/**
  Emit one measurement line to the debug log (DEBUG_ERROR, so it prints
  regardless of the platform debug level) and to the unit-test log.

  @param[in]  Label   Human-readable benchmark name.
  @param[in]  MinNs   Best-case (minimum) per-call time, nanoseconds.
  @param[in]  MeanNs  Mean per-call time, nanoseconds.
**/
STATIC
VOID
PerfEmit (
  IN CONST CHAR8  *Label,
  IN UINT64       MinNs,
  IN UINT64       MeanNs
  )
{
  UINT64  MinUs;
  UINT64  MeanUs;

  MinUs  = DivU64x32 (MinNs, 1000);
  MeanUs = DivU64x32 (MeanNs, 1000);

  DEBUG ((DEBUG_ERROR, "PERF: %-40a min %7Lu us  mean %7Lu us/call\n", Label, MinUs, MeanUs));
  UT_LOG_INFO ("%a: min %Lu us, mean %Lu us/call\n", Label, MinUs, MeanUs);
}

/**
  Time PERF_ITERATIONS calls to AuthenticodeVerifyEx() against the given
  trust anchor, reporting the minimum and mean per-call time.

  @param[in]   Anchor      Trust anchor certificate.
  @param[in]   AnchorSize  Size of Anchor, in bytes.
  @param[out]  MinNs       Minimum per-call time, nanoseconds.
  @param[out]  MeanNs      Mean per-call time, nanoseconds.

  @retval UNIT_TEST_PASSED  All calls succeeded.
**/
STATIC
UNIT_TEST_STATUS
TimeVerifyEx (
  IN  CONST UINT8  *Anchor,
  IN  UINTN        AnchorSize,
  OUT UINT64       *MinNs,
  OUT UINT64       *MeanNs
  )
{
  EFI_STATUS  Status;
  UINT8       *Chain;
  UINTN       ChainSize;
  UINT64      Start;
  UINT64      End;
  UINT64      Delta;
  UINT64      MinTicks;
  UINT64      TotalTicks;
  UINTN       Index;

  PerfInitCounter ();

  //
  // Warm up (page-in, first-touch allocations, MM connect) and confirm the
  // chain actually comes back.
  //
  Chain     = NULL;
  ChainSize = 0;
  Status    = AuthenticodeVerifyEx (mPerfAuthData, mPerfAuthDataSize, Anchor, AnchorSize, mPerfAuthImageHash, mPerfAuthImageHashSize, &Chain, &ChainSize);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (Chain);
  if (Chain != NULL) {
    FreePool (Chain);
    Chain = NULL;
  }

  MinTicks   = MAX_UINT64;
  TotalTicks = 0;
  for (Index = 0; Index < PERF_ITERATIONS; Index++) {
    Start  = GetPerformanceCounter ();
    Status = AuthenticodeVerifyEx (mPerfAuthData, mPerfAuthDataSize, Anchor, AnchorSize, mPerfAuthImageHash, mPerfAuthImageHashSize, &Chain, &ChainSize);
    End    = GetPerformanceCounter ();

    Delta       = ElapsedTicks (Start, End);
    TotalTicks += Delta;
    if (Delta < MinTicks) {
      MinTicks = Delta;
    }

    if (Chain != NULL) {
      FreePool (Chain);
      Chain = NULL;
    }

    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  *MinNs  = GetTimeInNanoSecond (MinTicks);
  *MeanNs = DivU64x32 (GetTimeInNanoSecond (TotalTicks), (UINT32)PERF_ITERATIONS);
  return UNIT_TEST_PASSED;
}

/**
  Baseline: time a full AuthenticodeVerify() (verify only, no chain).
**/
UNIT_TEST_STATUS
EFIAPI
BenchAuthenticodeVerify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Ok;
  UINT64   Start;
  UINT64   End;
  UINT64   Delta;
  UINT64   MinTicks;
  UINT64   TotalTicks;
  UINT64   MeanNs;
  UINTN    Index;

  PerfInitCounter ();

  Ok = AuthenticodeVerify (mPerfAuthData, mPerfAuthDataSize, mPerfAuthAnchor, mPerfAuthAnchorSize, mPerfAuthImageHash, mPerfAuthImageHashSize);
  UT_ASSERT_TRUE (Ok);

  MinTicks   = MAX_UINT64;
  TotalTicks = 0;
  for (Index = 0; Index < PERF_ITERATIONS; Index++) {
    Start = GetPerformanceCounter ();
    Ok    = AuthenticodeVerify (mPerfAuthData, mPerfAuthDataSize, mPerfAuthAnchor, mPerfAuthAnchorSize, mPerfAuthImageHash, mPerfAuthImageHashSize);
    End   = GetPerformanceCounter ();

    Delta       = ElapsedTicks (Start, End);
    TotalTicks += Delta;
    if (Delta < MinTicks) {
      MinTicks = Delta;
    }

    UT_ASSERT_TRUE (Ok);
  }

  mNsVerify = GetTimeInNanoSecond (MinTicks);
  MeanNs    = DivU64x32 (GetTimeInNanoSecond (TotalTicks), (UINT32)PERF_ITERATIONS);
  PerfEmit ("AuthenticodeVerify (verify only)", mNsVerify, MeanNs);
  return UNIT_TEST_PASSED;
}

/**
  AuthenticodeVerifyEx with the root as the trust anchor: one call that
  verifies the image AND returns the full signer..root chain. Should cost
  about the same as AuthenticodeVerify alone (the chain falls out of the
  verify OpenSSL already did).
**/
UNIT_TEST_STATUS
EFIAPI
BenchAuthenticodeVerifyExRoot (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64            MeanNs;
  UNIT_TEST_STATUS  TestStatus;

  MeanNs     = 0;
  TestStatus = TimeVerifyEx (mPerfAuthAnchor, mPerfAuthAnchorSize, &mNsVerifyExRt, &MeanNs);
  if (TestStatus == UNIT_TEST_PASSED) {
    PerfEmit ("AuthenticodeVerifyEx (verify + chain)", mNsVerifyExRt, MeanNs);
  }

  return TestStatus;
}

/**
  AuthenticodeVerifyEx with the signer itself as the trust anchor: the trust
  path is a single certificate (the chain is trimmed at the anchor). Exposes
  the cost of the trim path relative to the full-chain case.
**/
UNIT_TEST_STATUS
EFIAPI
BenchAuthenticodeVerifyExSignerIsAnchor (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS        Status;
  UINT8             *Chain;
  UINTN             ChainSize;
  UINT8             *SignerCert;
  UINT32            SignerLen;
  UINT64            MeanNs;
  UNIT_TEST_STATUS  TestStatus;

  //
  // Obtain the signer (leaf) certificate from a normal (root-anchored)
  // verification: it is entry 0 of the returned chain.
  //
  Chain     = NULL;
  ChainSize = 0;
  Status    = AuthenticodeVerifyEx (mPerfAuthData, mPerfAuthDataSize, mPerfAuthAnchor, mPerfAuthAnchorSize, mPerfAuthImageHash, mPerfAuthImageHashSize, &Chain, &ChainSize);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (Chain);
  UT_ASSERT_TRUE (1 + sizeof (UINT32) <= ChainSize);

  SignerLen = ReadUnaligned32 ((CONST UINT32 *)(Chain + 1));
  UT_ASSERT_TRUE ((UINTN)(1 + sizeof (UINT32) + SignerLen) <= ChainSize);
  SignerCert = AllocatePool (SignerLen);
  UT_ASSERT_NOT_NULL (SignerCert);
  CopyMem (SignerCert, Chain + 1 + sizeof (UINT32), SignerLen);
  FreePool (Chain);

  MeanNs     = 0;
  TestStatus = TimeVerifyEx (SignerCert, SignerLen, &mNsVerifyExSg, &MeanNs);
  FreePool (SignerCert);
  if (TestStatus == UNIT_TEST_PASSED) {
    PerfEmit ("AuthenticodeVerifyEx (signer==anchor)", mNsVerifyExSg, MeanNs);
  }

  return TestStatus;
}

/**
  Emit a percentage ratio line (Value / Baseline) using the minimum per-call
  times.
**/
STATIC
VOID
PerfRatio (
  IN CONST CHAR8  *Label,
  IN UINT64       ValueNs,
  IN UINT64       BaselineNs
  )
{
  UINT64  Pct;

  if (BaselineNs == 0) {
    return;
  }

  Pct = DivU64x64Remainder (MultU64x32 (ValueNs, 100), BaselineNs, NULL);
  DEBUG ((DEBUG_ERROR, "PERF:   %-38a %4Lu %% of AuthenticodeVerify\n", Label, Pct));
  UT_LOG_INFO ("%a: %Lu%% of AuthenticodeVerify\n", Label, Pct);
}

/**
  Summary: report each AuthenticodeVerifyEx case as a percentage of a full
  AuthenticodeVerify(), quantifying how nearly free the returned chain is.
  All ratios use the minimum (best-case) per-call times.
**/
UNIT_TEST_STATUS
EFIAPI
BenchSummary (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  DEBUG ((DEBUG_ERROR, "PERF: ===== SUMMARY (min-of-%d) =====\n", PERF_ITERATIONS));
  PerfRatio ("VerifyEx root   (verify + chain)", mNsVerifyExRt, mNsVerify);
  PerfRatio ("VerifyEx signer (single-cert)", mNsVerifyExSg, mNsVerify);
  DEBUG ((DEBUG_ERROR, "PERF: ===================================\n"));
  return UNIT_TEST_PASSED;
}

TEST_DESC  mPerfBenchmarkTest[] = {
  //
  // -----Description---------------------------Class--------------------------Function-------------------------------Pre--Post-Context
  //
  { "AuthenticodeVerify baseline",          "CryptoPkg.BaseCryptLib.Perf", BenchAuthenticodeVerify,                 NULL, NULL, NULL },
  { "AuthenticodeVerifyEx (root anchor)",   "CryptoPkg.BaseCryptLib.Perf", BenchAuthenticodeVerifyExRoot,           NULL, NULL, NULL },
  { "AuthenticodeVerifyEx (signer anchor)", "CryptoPkg.BaseCryptLib.Perf", BenchAuthenticodeVerifyExSignerIsAnchor, NULL, NULL, NULL },
  { "Summary and ratios",                   "CryptoPkg.BaseCryptLib.Perf", BenchSummary,                            NULL, NULL, NULL },
};

UINTN  mPerfBenchmarkTestNum = ARRAY_SIZE (mPerfBenchmarkTest);

#endif // ENABLE_PERF_BENCHMARKS
