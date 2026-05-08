/** @file
  UEFI Shell UnitTest application that validates TCG2 event log dynamic
  scaling after ReadyToBoot.

  This application locates the TcgLogTestProtocol produced by TcgLogTestDxe to
  retrieve pre-ReadyToBoot test logs, then exercises post-ReadyToBoot scaling
  and verifies the truncation marker is present in the ACPI event log.

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>
#include <Protocol/Tcg2Protocol.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <IndustryStandard/Acpi.h>

#include "TcgLogTest.h"
#include "TcgLogTestCommon.h"

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         Flags;
  UINT64                         AddressOfControlArea;
  UINT32                         StartMethod;
  UINT8                          PlatformSpecificParameters[12];
  UINT32                         Laml;
  UINT64                         Lasa;
} TCG_LOG_TEST_TPM2_ACPI_TABLE_V4;

#pragma pack()

#define TCG_LOG_TRUNCATION_EVENT_STRING  "TCG Event Log Truncated"
#define UNIT_TEST_NAME                   "TCG Log Scaling Test"
#define UNIT_TEST_VERSION                "1.0"

STATIC EFI_TCG2_PROTOCOL      *mTcg2Protocol       = NULL;
STATIC TCG_LOG_TEST_PROTOCOL  *mTcgLogTestProtocol = NULL;

/**
  Look up the installed TPM2 ACPI table and return its LAML/LASA values.

  @param[out] Laml  Log Area Minimum Length from the ACPI table.
  @param[out] Lasa  Log Area Start Address from the ACPI table.

  @retval EFI_SUCCESS            Table found and values returned.
  @retval EFI_NOT_FOUND          TPM2 table not installed or too small.
  @retval EFI_INVALID_PARAMETER  Protocol not installed.
**/
STATIC
EFI_STATUS
GetTpm2AcpiTableLogData (
  OUT UINT32                *Laml,
  OUT EFI_PHYSICAL_ADDRESS  *Lasa
  )
{
  EFI_STATUS                       Status;
  EFI_ACPI_SDT_PROTOCOL            *AcpiSdt;
  EFI_ACPI_SDT_HEADER              *SdtHeader;
  EFI_ACPI_TABLE_VERSION           Version;
  UINTN                            TableKey;
  UINTN                            Index;
  TCG_LOG_TEST_TPM2_ACPI_TABLE_V4  *Tpm2Table;

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiSdt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Walk the installed ACPI tables looking for the TPM2 signature.
  Index = 0;
  while (TRUE) {
    Status = AcpiSdt->GetAcpiTable (Index, &SdtHeader, &Version, &TableKey);
    if (EFI_ERROR (Status)) {
      // Reached the end of the table list without finding TPM2.
      DEBUG ((DEBUG_ERROR, "%a: TPM2 table not found\n", __func__));
      return EFI_NOT_FOUND;
    }

    if (SdtHeader->Signature == EFI_ACPI_5_0_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE) {
      break;
    }

    Index++;
  }

  if (SdtHeader->Length < sizeof (TCG_LOG_TEST_TPM2_ACPI_TABLE_V4)) {
    return EFI_NOT_FOUND;
  }

  Tpm2Table = (TCG_LOG_TEST_TPM2_ACPI_TABLE_V4 *)SdtHeader;
  *Laml     = Tpm2Table->Laml;
  *Lasa     = (EFI_PHYSICAL_ADDRESS)Tpm2Table->Lasa;

  return EFI_SUCCESS;
}

/**
  Walk the ACPI event log and check whether it contains a NO_ACTION event
  whose payload matches the given truncation string.

  The ACPI log starts with a TCG 1.2 format SpecID event (TCG_PCR_EVENT_HDR
  with a fixed 20-byte SHA1 digest), followed by TCG 2.0 format events.
  This function skips the SpecID header, then walks all TCG 2.0 events
  looking for the truncation marker.

  @param[in] Lasa  Log Area Start Address (ACPI log base).
  @param[in] Laml  Log Area Min Length (ACPI log size).

  @retval TRUE   A matching NO_ACTION truncation event was found.
  @retval FALSE  No match or log could not be parsed.
**/
BOOLEAN
CheckTruncationEvent (
  IN EFI_PHYSICAL_ADDRESS  Lasa,
  IN UINTN                 Laml
  )
{
  UINT8       *CurrentEvent;
  UINT8       *EndOfLog;
  UINT32      PcrIndex;
  UINT32      EventType;
  UINT32      EventSize;
  UINT8       *EventData;
  UINT32      EventDataLen;
  UINT32      SpecIdEventSize;
  CONST CHAR8 *TruncEventStr = TCG_LOG_TRUNCATION_EVENT_STRING;

  // Verify the input parameters.
  if ((Lasa == 0) || (Laml == 0) || (TruncEventStr == NULL)) {
    return FALSE;
  }

  CurrentEvent = (UINT8 *)(UINTN)Lasa;
  EndOfLog     = CurrentEvent + Laml;
  EventDataLen = (UINT32)AsciiStrSize (TruncEventStr);

  // Skip the first event which is the TCG 1.2 format SpecID event:
  //  PCRIndex (4) + EventType (4) + Digest (20 = SHA1) + EventSize (4) + Event[EventSize]
  if ((UINTN)(EndOfLog - CurrentEvent) < sizeof (TCG_PCR_EVENT_HDR)) {
    return FALSE;
  }

  SpecIdEventSize = ((TCG_PCR_EVENT_HDR *)CurrentEvent)->EventSize;
  if ((UINTN)(EndOfLog - CurrentEvent) < sizeof (TCG_PCR_EVENT_HDR) + SpecIdEventSize) {
    return FALSE;
  }

  CurrentEvent += sizeof (TCG_PCR_EVENT_HDR) + SpecIdEventSize;

  // Walk all TCG 2.0 events looking for the truncation marker.
  while (TcgLogTestAdvanceEvent (&CurrentEvent, EndOfLog, &PcrIndex, &EventType, &EventSize, &EventData)) {
    if ((EventType == EV_NO_ACTION) && (PcrIndex == 0) && (EventSize == EventDataLen)) {
      if (CompareMem (TruncEventStr, EventData, EventDataLen) == 0) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
  Test that the DXE driver ran and its pre-ReadyToBoot log contains PASS.

  This runs on the second boot after TestPostReadyToBootScaling enabled the
  DXE driver and rebooted.  The DXE driver ran before ReadyToBoot on this
  boot, so results are available via the protocol.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED              Log contains PASS and no FAIL.
  @retval UNIT_TEST_ERROR_TEST_FAILED   Assertion failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestPreReadyToBootResults (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  CHAR8       *LogBuffer;
  UINTN       LogSize;

  // The prerequisite is skipped on resume from a reboot, so locate the
  // protocol here if it was not already set.
  if (mTcgLogTestProtocol == NULL) {
    Status = gBS->LocateProtocol (&gTcgLogTestProtocolGuid, NULL, (VOID **)&mTcgLogTestProtocol);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  Status = mTcgLogTestProtocol->GetLog (mTcgLogTestProtocol, &LogBuffer, &LogSize);
  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("GetLog failed: %r\n", Status);
  }

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (LogBuffer);
  UT_ASSERT_TRUE (LogSize > 1);

  // Dump the DXE driver's log for visibility.
  UT_LOG_INFO ("TcgLogTestDxe Log (%u bytes):\n%a\n", LogSize, LogBuffer);

  // Verify the log contains "PASS".
  UT_ASSERT_NOT_NULL (AsciiStrStr (LogBuffer, "PASS"));

  // Verify the log does not contain "FAIL".
  UT_ASSERT_TRUE (AsciiStrStr (LogBuffer, "FAIL") == NULL);

  return UNIT_TEST_PASSED;
}

/**
  Test post-ReadyToBoot scaling: log events until the log scales, then verify
  the truncation marker is present in the ACPI log region.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED              Scaling and truncation marker verified.
  @retval UNIT_TEST_ERROR_TEST_FAILED   Assertion failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestPostReadyToBootScaling (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS            Status;
  BOOLEAN               Scaled;
  BOOLEAN               Truncated;
  EFI_PHYSICAL_ADDRESS  AcpiLasa;
  UINT32                AcpiLaml;

  Status = GetTpm2AcpiTableLogData (&AcpiLaml, &AcpiLasa);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_TRUE (AcpiLasa != 0);
  UT_ASSERT_TRUE (AcpiLaml != 0);

  Status = TcgLogTestLogEventsUntilScaled (mTcg2Protocol, &Scaled);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_TRUE (Scaled);

  Status = TcgLogTestDumpEventLog (mTcg2Protocol, &Truncated);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // The normal event log must not be truncated after scaling.
  UT_ASSERT_FALSE (Truncated);

  UT_LOG_INFO ("Post-ReadyToBoot scaling succeeded\n");

  // Verify truncation marker in ACPI log.
  UT_ASSERT_TRUE (CheckTruncationEvent (AcpiLasa, (UINTN)AcpiLaml));

  return UNIT_TEST_PASSED;
}

/**
  Save the unit test framework state and perform a cold reboot.

  @param[in] Context  Unit test context (unused).
**/
STATIC
VOID
EFIAPI
SaveAndReboot (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  SaveFrameworkState (NULL, 0);
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
}

/**
  Cleanup for TestPostReadyToBootScaling: enable the DXE pre-ReadyToBoot test
  for the next boot, then save and reboot so the DXE driver runs before
  ReadyToBoot on the second boot.

  @param[in] Context  Unit test context (unused).
**/
STATIC
VOID
EFIAPI
EnableDxeTestAndReboot (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mTcgLogTestProtocol != NULL) {
    Status = mTcgLogTestProtocol->Enable (mTcgLogTestProtocol, TRUE);
    DEBUG ((DEBUG_INFO, "%a: Enable (TRUE) - %r\n", __func__, Status));
  } else {
    DEBUG ((DEBUG_ERROR, "%a: mTcgLogTestProtocol is NULL, cannot enable\n", __func__));
  }

  SaveAndReboot (Context);
}

/**
  Prerequisite: locate the TCG2 and TcgLogTest protocols.

  @param[in] Context  Unit test context (unused).

  @retval UNIT_TEST_PASSED                      Protocols located.
  @retval UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Protocol not found.
**/
UNIT_TEST_STATUS
EFIAPI
LocateProtocols (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **)&mTcg2Protocol);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  Status = gBS->LocateProtocol (&gTcgLogTestProtocolGuid, NULL, (VOID **)&mTcgLogTestProtocol);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  return UNIT_TEST_PASSED;
}

/**
  Entry point for TcgLogTestApp.

  @param[in] ImageHandle  Image handle.
  @param[in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS  Tests dispatched and framework freed.
  @retval Other        Framework initialization failed.
**/
EFI_STATUS
EFIAPI
TcgLogTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Suite;

  Framework = NULL;

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InitUnitTestFramework failed: %r\n", __func__, Status));
    return Status;
  }

  Status = CreateUnitTestSuite (&Suite, Framework, "TCG Log Scaling Tests", "TcgLogTest", NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  AddTestCase (Suite, "Post-ReadyToBoot scaling produces truncation event", "PostRtbScaling", TestPostReadyToBootScaling, LocateProtocols, EnableDxeTestAndReboot, NULL);
  AddTestCase (Suite, "Pre-ReadyToBoot DXE results contain PASS", "PreRtbResults", TestPreReadyToBootResults, LocateProtocols, SaveAndReboot, NULL);

  Status = RunAllTestSuites (Framework);

Done:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}
