/** @file
  Performance test entry point and timing helpers for BaseCryptLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptoPerfLib.h"

UINT64
PerfElapsedNanoSeconds (
  IN UINT64  StartTick,
  IN UINT64  EndTick
  )
{
  if (EndTick >= StartTick) {
    return GetTimeInNanoSecond (EndTick - StartTick);
  } else {
    return GetTimeInNanoSecond (StartTick - EndTick);
  }
}

VOID
PerfLogResult (
  IN CONST CHAR8  *Label,
  IN UINT64       ElapsedNs,
  IN UINTN        Iterations,
  IN UINTN        DataSize
  )
{
  UINT64  ElapsedMs;

  ElapsedMs = ElapsedNs / 1000000;

  if (DataSize > 0) {
    //
    // Bulk data operation - compute throughput in MB/s.
    // TotalKB = Iterations * (DataSize / 1024)
    // MB/s = TotalKB * 1000000 / ElapsedUs / 1024
    //
    UINT64  TotalKB;
    UINT64  ElapsedUs;
    UINT64  MBps;

    TotalKB   = (UINT64)Iterations * ((UINT64)DataSize / 1024);
    ElapsedUs = ElapsedNs / 1000;
    MBps      = 0;
    if (ElapsedUs > 0) {
      MBps = (TotalKB * 1000000ULL) / ElapsedUs / 1024;
    }

    DEBUG ((
      DEBUG_INFO,
      "  %-35a : %6lu ms  (%lu MB/s)  [%u iters x %u KB]\n",
      Label,
      ElapsedMs,
      MBps,
      (UINT32)Iterations,
      (UINT32)(DataSize / 1024)
      ));
  } else {
    //
    // Discrete operation - compute ops/sec.
    //
    UINT64  OpsPerSec;

    OpsPerSec = 0;
    if (ElapsedNs > 0) {
      OpsPerSec = (UINT64)Iterations * 1000000000ULL / ElapsedNs;
    }

    DEBUG ((
      DEBUG_INFO,
      "  %-35a : %6lu ms  (%lu ops/s)  [%u iters]\n",
      Label,
      ElapsedMs,
      OpsPerSec,
      (UINT32)Iterations
      ));
  }
}

EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UINT64                      StartTick;
  UINT64                      EndTick;
  UINT64                      ElapsedNs;

  DEBUG ((DEBUG_INFO, "%a v%a\n", PERF_TEST_NAME, PERF_TEST_VERSION));

  Status = CreatePerfTest (PERF_TEST_NAME, PERF_TEST_VERSION, &Framework);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to create perf test framework! Status = %r\n", Status));
    goto Done;
  }

  StartTick = GetPerformanceCounter ();
  Status    = RunAllTestSuites (Framework);
  EndTick   = GetPerformanceCounter ();

  ElapsedNs = PerfElapsedNanoSeconds (StartTick, EndTick);
  DEBUG ((DEBUG_INFO, "\nAll performance tests completed in %lu ms\n", ElapsedNs / 1000000));

Done:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard PEIM entry point for target based unit test execution from PEI.
**/
EFI_STATUS
EFIAPI
PeiEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  return UefiTestMain ();
}

/**
  Standard UEFI entry point for target based unit test execution from DXE, SMM,
  UEFI Shell.
**/
EFI_STATUS
EFIAPI
DxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UefiTestMain ();
}
