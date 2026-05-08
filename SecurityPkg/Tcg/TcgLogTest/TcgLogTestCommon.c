/** @file
  TCG Log Test common implementation shared by TcgLogTestDxe and TcgLogTestApp.

  Contains event log walking, truncation event checking, log-until-scaled
  logic, and TPM2 ACPI table lookup.

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/Tcg2Protocol.h>
#include <IndustryStandard/UefiTcgPlatform.h>

#include "TcgLogTestCommon.h"

#define TCG_LOG_TEST_PCR_INDEX      8
#define TCG_LOG_TEST_EVENT_TYPE     EV_NO_ACTION
#define TCG_LOG_TEST_EVENT_PAYLOAD  "TcgLogTestDxeEvent"

STATIC CHAR8  mCommonEventPayload[] = TCG_LOG_TEST_EVENT_PAYLOAD;

/**
  Allocate and initialize an EFI_TCG2_EVENT structure with a fixed payload.

  @param[out] Event  On success, pointer to the allocated event. Caller must
                     free with FreePool().

  @retval EFI_SUCCESS           Event allocated and initialized.
  @retval EFI_OUT_OF_RESOURCES  Allocation failed.
**/
STATIC
EFI_STATUS
TcgLogTestBuildEvent (
  OUT EFI_TCG2_EVENT  **Event
  )
{
  UINT32          PayloadSize;
  UINT32          TotalSize;
  EFI_TCG2_EVENT  *TestEvent;

  PayloadSize = (UINT32)AsciiStrSize (mCommonEventPayload);
  TotalSize   = (UINT32)(sizeof (EFI_TCG2_EVENT) - sizeof (TestEvent->Event) + PayloadSize);

  TestEvent = AllocateZeroPool (TotalSize);
  if (TestEvent == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TestEvent->Size                 = TotalSize;
  TestEvent->Header.HeaderSize    = sizeof (EFI_TCG2_EVENT_HEADER);
  TestEvent->Header.HeaderVersion = EFI_TCG2_EVENT_HEADER_VERSION;
  TestEvent->Header.PCRIndex      = TCG_LOG_TEST_PCR_INDEX;
  TestEvent->Header.EventType     = TCG_LOG_TEST_EVENT_TYPE;

  CopyMem (TestEvent->Event, mCommonEventPayload, PayloadSize);

  *Event = TestEvent;
  return EFI_SUCCESS;
}

/**
  Log a single event via the TCG2 protocol.

  @param[in] Tcg2Protocol  TCG2 protocol instance.
  @param[in] Event         Pre-built TCG2 event structure.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.
**/
STATIC
EFI_STATUS
TcgLogTestLogEvent (
  IN EFI_TCG2_PROTOCOL  *Tcg2Protocol,
  IN EFI_TCG2_EVENT     *Event
  )
{
  return Tcg2Protocol->HashLogExtendEvent (
                         Tcg2Protocol,
                         0,
                         (EFI_PHYSICAL_ADDRESS)(UINTN)mCommonEventPayload,
                         AsciiStrSize (mCommonEventPayload),
                         Event
                         );
}

/**
  Advance one entry in the event log.

  @param[in,out] CurrentEvent  On entry, points to the start of the event (PCRIndex).
                               On success, updated to point to the next event.
  @param[in]     LogEnd        One byte past the end of the log buffer.
  @param[out]    PcrIndex      PCRIndex of the parsed event.
  @param[out]    EventType     EventType of the parsed event.
  @param[out]    EventSize     Size of the event data payload.
  @param[out]    EventData     Pointer to the event data payload.

  @retval TRUE   Event parsed successfully and CurrentEvent updated.
  @retval FALSE  Invalid pointers, buffer, or digest algorithm.
**/
BOOLEAN
TcgLogTestAdvanceEvent (
  IN OUT UINT8   **CurrentEvent,
  IN     UINT8   *LogEnd,
  OUT    UINT32  *PcrIndex   OPTIONAL,
  OUT    UINT32  *EventType  OPTIONAL,
  OUT    UINT32  *EventSize  OPTIONAL,
  OUT    UINT8   **EventData OPTIONAL
  )
{
  UINT32  DigestCount;
  UINT32  DigestIndex;
  UINT16  AlgId;
  UINT32  DigestLen;
  UINT32  Size;
  UINT8   *EventPtr;

  // Verify the required pointers are valid
  if ((CurrentEvent == NULL) || (LogEnd == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid input parameters\n", __func__));
    return FALSE;
  }

  // Start on the current event.
  EventPtr = *CurrentEvent;

  // Verify there are 8 bytes (PCRIndex (4 bytes) + EventType (4 bytes))
  // in the log before attempting to read.
  if ((UINTN)(LogEnd - EventPtr) < sizeof (UINT32) + sizeof (UINT32)) {
    DEBUG ((DEBUG_ERROR, "%a: PCRIndex & EventType invalid\n", __func__));
    return FALSE;
  }

  // Store the PCRIndex, if provided.
  if (PcrIndex != NULL) {
    *PcrIndex = *(UINT32 *)EventPtr;
  }

  // Store the EventType, if provided.
  if (EventType != NULL) {
    *EventType = *(UINT32 *)(EventPtr + sizeof (UINT32));
  }

  // Move the pointer past the PcrIndex and EventType.
  EventPtr += sizeof (UINT32) + sizeof (UINT32);

  // Verify there are 4 bytes (DigestCount (4 bytes)) in the log before
  // attempting to read.
  // TPML_DIGEST_VALUES = DigestCount followed by (AlgId + Digest) pairs.
  if ((UINTN)(LogEnd - EventPtr) < sizeof (UINT32)) {
    DEBUG ((DEBUG_ERROR, "%a: DigestCount invalid\n", __func__));
    return FALSE;
  }

  // Acquire the DigestCount.
  DigestCount = *(UINT32 *)EventPtr;

  // Move the pointer past the DigestCount.
  EventPtr += sizeof (UINT32);

  // Loop through the number of digests.
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    // Verify there are 2 bytes (AlgId (2 bytes)) in the log
    // before attempting to read.
    if ((UINTN)(LogEnd - EventPtr) < sizeof (UINT16)) {
      DEBUG ((DEBUG_ERROR, "%a: AlgId invalid\n", __func__));
      return FALSE;
    }

    // Acquire the AlgId.
    AlgId = *(UINT16 *)EventPtr;

    // Move the pointer past the AlgId.
    EventPtr += sizeof (UINT16);

    // DigestLen depends on the AlgId.
    switch (AlgId) {
      case TPM_ALG_SHA1:
        DigestLen = SHA1_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA256:
        DigestLen = SHA256_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA384:
        DigestLen = SHA384_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA512:
        DigestLen = SHA512_DIGEST_SIZE;
        break;
      case TPM_ALG_SM3_256:
        DigestLen = SM3_256_DIGEST_SIZE;
        break;
      default:
        DEBUG ((DEBUG_ERROR, "%a: Unknown AlgId 0x%x\n", __func__, AlgId));
        return FALSE;
    }

    // Verify there are DigestLen bytes in the log.
    if ((UINTN)(LogEnd - EventPtr) < DigestLen) {
      DEBUG ((DEBUG_ERROR, "%a: DigestLen invalid\n", __func__));
      return FALSE;
    }

    // Move the pointer past the Digest based on the DigestLen.
    EventPtr += DigestLen;
  }

  // Verify there are 4 bytes (EventSize (4 bytes)) in the log
  // before attempting to read.
  if ((UINTN)(LogEnd - EventPtr) < sizeof (UINT32)) {
    DEBUG ((DEBUG_ERROR, "%a: EventSize invalid\n", __func__));
    return FALSE;
  }

  // Acquire the size of the event.
  Size = *(UINT32 *)EventPtr;

  // Move the pointer past the event size
  EventPtr += sizeof (UINT32);

  // Verify there are EventSize bytes in the log.
  if ((UINTN)(LogEnd - EventPtr) < Size) {
    DEBUG ((DEBUG_ERROR, "%a: Size of event invalid\n", __func__));
    return FALSE;
  }

  // Store the EventSize, if provided.
  if (EventSize != NULL) {
    *EventSize = Size;
  }

  // Store the EventData, if provided.
  if (EventData != NULL) {
    *EventData = EventPtr;
  }

  // Move the pointer to next Event.
  EventPtr += Size;

  // Update the current event pointer.
  *CurrentEvent = EventPtr;

  return TRUE;
}

/**
  Log events until the event log base address changes (i.e. dynamic scaling
  occurs) or an error is returned.

  @param[in]  Tcg2Protocol  TCG2 protocol instance.
  @param[out] Scaled        TRUE if scaling was detected.

  @retval EFI_SUCCESS            Scaling detected and events logged successfully.
  @retval EFI_INVALID_PARAMETER  One or more invalid parameters.
**/
EFI_STATUS
TcgLogTestLogEventsUntilScaled (
  IN  EFI_TCG2_PROTOCOL  *Tcg2Protocol,
  OUT BOOLEAN            *Scaled
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  LocationBefore;
  EFI_PHYSICAL_ADDRESS  LocationAfter;
  EFI_PHYSICAL_ADDRESS  LastEntry;
  BOOLEAN               Truncated;
  EFI_TCG2_EVENT        *Event;

  // Validate the input parameters.
  if ((Tcg2Protocol == NULL) || (Scaled == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Scaled = FALSE;

  // Build a test event.
  Status = TcgLogTestBuildEvent (&Event);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the event log before scaling.
  Status = Tcg2Protocol->GetEventLog (
                           Tcg2Protocol,
                           EFI_TCG2_EVENT_LOG_FORMAT_TCG_2,
                           &LocationBefore,
                           &LastEntry,
                           &Truncated
                           );

  if (EFI_ERROR (Status)) {
    FreePool (Event);
    return Status;
  }

  // Log multiple events until scaling occurs.
  do {
    Status = TcgLogTestLogEvent (Tcg2Protocol, Event);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: LogEvent failed - %r\n", __func__, Status));
      FreePool (Event);
      return Status;
    }

    Status = Tcg2Protocol->GetEventLog (
                             Tcg2Protocol,
                             EFI_TCG2_EVENT_LOG_FORMAT_TCG_2,
                             &LocationAfter,
                             &LastEntry,
                             &Truncated
                             );

    if (EFI_ERROR (Status)) {
      FreePool (Event);
      return Status;
    }
  } while (LocationBefore == LocationAfter);

  *Scaled = TRUE;

  DEBUG ((DEBUG_INFO, "%a: Log scaled (0x%lx -> 0x%lx)\n", __func__, LocationBefore, LocationAfter));

  FreePool (Event);
  return EFI_SUCCESS;
}

/**
  Dump the contents of a TCG 2.0 event log via DEBUG prints.

  Walks the event log skipping the TCG 1.2 SpecID header event.
  Prints each event's index, PCRIndex, EventType, and EventSize.

  @param[in]  Tcg2Protocol  TCG2 protocol instance used to retrieve the event log.
  @param[out] Truncated     If the log is truncated.

  @retval EFI_SUCCESS            Log dumped successfully.
  @retval EFI_INVALID_PARAMETER  One or more invalid parameters.
**/
EFI_STATUS
TcgLogTestDumpEventLog (
  IN  EFI_TCG2_PROTOCOL  *Tcg2Protocol,
  OUT BOOLEAN            *Truncated
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  LogBase;
  EFI_PHYSICAL_ADDRESS  LastEntry;
  BOOLEAN               LogTruncated;
  UINT8                 *CurrentEvent;
  UINT8                 *EndOfLog;
  UINT32                PcrIndex;
  UINT32                EventType;
  UINT32                EventSize;
  UINTN                 Index;

  // Validate the input parameters.
  if ((Tcg2Protocol == NULL) || (Truncated == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Tcg2Protocol->GetEventLog (
                           Tcg2Protocol,
                           EFI_TCG2_EVENT_LOG_FORMAT_TCG_2,
                           &LogBase,
                           &LastEntry,
                           &LogTruncated
                           );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: GetEventLog failed - %r\n", __func__, Status));
    return Status;
  }

  // Check if the log is truncated. This should never happen.
  if (Truncated != NULL) {
    *Truncated = LogTruncated;
  }

  // Check if the log is empty.
  if (LastEntry < LogBase) {
    DEBUG ((DEBUG_WARN, "%a: EventLog is empty\n", __func__));
    return EFI_SUCCESS;
  }

  // Skip the TCG 1.2 SpecID header event.
  CurrentEvent  = (UINT8 *)(UINTN)LogBase;
  CurrentEvent += sizeof (TCG_PCR_EVENT_HDR) + ((TCG_PCR_EVENT_HDR *)CurrentEvent)->EventSize;

  // Determine what the EndOfLog should be.
  EndOfLog = (UINT8 *)(UINTN)LastEntry;
  if (!TcgLogTestAdvanceEvent (&EndOfLog, EndOfLog + MAX_UINT16, NULL, NULL, NULL, NULL)) {
    DEBUG ((DEBUG_WARN, "%a: Could not determine EndOfLog\n", __func__));
    return EFI_SUCCESS;
  }

  // Dump the contents of the EventLog.
  DEBUG ((DEBUG_INFO, "%a: Event log dump (base=0x%lx, truncated=%d):\n", __func__, LogBase, LogTruncated));
  DEBUG ((DEBUG_INFO, "  %-6a %-10a %-12a %a\n", "Index", "PCRIndex", "EventType", "EventSize"));
  DEBUG ((DEBUG_INFO, "  %-6a %-10a %-12a %a\n", "-----", "--------", "---------", "---------"));

  Index = 0;
  while (TcgLogTestAdvanceEvent (&CurrentEvent, EndOfLog, &PcrIndex, &EventType, &EventSize, NULL)) {
    DEBUG ((DEBUG_INFO, "  %-6u %-10u 0x%08x    %u\n", Index, PcrIndex, EventType, EventSize));
    Index++;
  }

  DEBUG ((DEBUG_INFO, "%a: Total events: %u\n", __func__, Index));
  return EFI_SUCCESS;
}
