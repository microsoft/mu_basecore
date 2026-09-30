# Tcg2Dxe

Tcg2Dxe is a DXE-phase UEFI driver that publishes the TCG2 protocol defined
by the [TCG EFI Protocol Specification](https://trustedcomputinggroup.org/resource/tcg-efi-protocol-specification/).
Its main responsibilites are to expose a standard interface to a TPM device,
measure components and events into PCRs, support measured boot, and enable
secure boot attestation.

## Dynamic Event Log Scaling

The TCG event log is initially allocated with a fixed size defined by a
PCD: PcdTcgLogAreaMinLen. As firmware components log measured boot
events the log fills up. Traditionally, when the log is full, subsequent events
are dropped and the log is marked as truncated.

Tcg2Dxe extends this behavior with **dynamic scaling**: when the log is about
to overflow, the driver doubles its allocation, copies the existing log into
the new buffer, and frees the old one. This allows the log to grow as needed
and avoids losing events.

### How It Works

1. **Scaling check** — Before logging a TCG 2.0 event,
   `TcgLogDynamicScalingNeeded` calculates whether the new event would exceed
   the current allocation (`EventLogAreaStruct->Laml`).

2. **Reallocation** — When scaling is needed, `TcgScaleEventLog` allocates a
   new `EfiBootServicesData` region at twice the current size, copies the
   existing log, updates the `Lasa`/`Laml` fields in the event log area
   struct, and frees the old region.

3. **Logging** — After scaling, the new event is logged into the resized buffer
   via `TcgDxeLogEvent` inside a TPL-raised critical section.

### NormalEventLog vs. FinalEventLog

Tcg2Dxe maintains two distinct event log regions:

| Log | Memory Type | Lifetime | Can Scale |
| --- | ----------- | -------- | --------- |
| **Normal log** | `EfiBootServicesData` | Available until `ExitBootServices` | Yes |
| **Final Events log** | `EfiACPIMemoryNVS` | Persistent | No |
| **ACPI event log** | `EfiACPIMemoryNVS` | Persistent | No |

- The **Normal log** is the main log copy which is returned via `GetEventLog`.
  It can grow dynamically via scaling. Note that previous calls to `GetEventLog`
  could contain stale data if the log was scaled after. It is recommended to
  call `GetEventLog` each time access is required.
- The **Final Events log** (`EFI_TCG2_FINAL_EVENTS_TABLE`) records events
  logged after `GetEventLog` has been called. It is installed as a UEFI
  configuration table so the OS can discover events that occurred between its
  call to `GetEventLog` and `ExitBootServices`. Because the **Final Events log**
  does not scale, it can become truncated.
- The **ACPI event log** is a fixed-size mirror of the normal log, allocated
  in `EfiACPIMemoryNVS` and sized from `PcdTcgLogAreaMinLen`. Because this region
  does not scale, it can become truncated. Its address and length are published
  to `PcdTpm2AcpiTableLasa` and `PcdTpm2AcpiTableLaml` so `Tcg2Acpi` picks them
  up when populating the TPM2 ACPI table.

### Scale Limit

The number of times the normal event log region may be dynamically scaled is
capped by `TCG_EVENT_LOG_MAX_SCALE_COUNT`. Each successful scale doubles the
allocation, so this caps total growth at `PcdTcgLogAreaMinLen << TCG_EVENT_LOG_MAX_SCALE_COUNT`.
Once the limit is reached:

1. `TcgScaleEventLog` refuses to scale further and returns
   `EFI_OUT_OF_RESOURCES`.
2. `EventLogAreaStruct->EventLogTruncated` is set to `TRUE`, so subsequent
   `GetEventLog` callers see `EventLogTruncated == TRUE`.
3. `HashLogExtendEvent` returns `EFI_VOLUME_FULL` for events that would have
   triggered the refused scale.

### Scaling Notification (`gTcg2EventLogScaledGuid`)

Each time the normal log is successfully resized, `TcgScaleEventLog` calls
`EfiEventGroupSignal (&gTcg2EventLogScaledGuid)` to notify interested parties
that the log moved in memory.

Consumers that cache the log base address returned by `GetEventLog` (for
example, parsers walking the log incrementally) must invalidate their cache
on this signal and call `GetEventLog` again to get the current `Lasa`/last
entry. A typical consumer:

```c
gBS->CreateEventEx (
       EVT_NOTIFY_SIGNAL,
       TPL_CALLBACK,
       OnTcgEventLogScaled,
       Context,
       &gTcg2EventLogScaledGuid,
       &Event
       );
```

The event is declared in `gTcg2EventLogScaledGuid` (see
`SecurityPkg/Include/Guid/Tcg2EventLogScaled.h`) and listed in `Tcg2Dxe.inf`.

## FinalEventLog Truncation Marker

Because the **FinalEventLog** is fixed-size and cannot scale, it can fill
up before `ExitBootServices`. When the next event would overflow the log,
`TcgDxeLogEvent` calls `AppendTruncationMarker` which writes a final
`EV_NO_ACTION` event whose payload is the ASCII string
`TCG_LOG_TRUNCATION_EVENT_STRING`. The `NumberOfEvents` counter in
`EFI_TCG2_FINAL_EVENTS_TABLE` is incremented to include the marker, and
`EventLogTruncated` is set so subsequent attempts return `EFI_VOLUME_FULL`
without re-appending the marker.

To guarantee the marker always fits, FinalEventLog initialization in
`SetupEventLog` subtracts `GetTruncationEventSize()` from the usable `Laml`:

```c
mTcgDxeData.FinalEventLogAreaStruct[Index].Laml =
    PcdGet32 (PcdTcg2FinalLogAreaLen)
  - sizeof (EFI_TCG2_FINAL_EVENTS_TABLE)
  - GetTruncationEventSize ();
```

`AppendTruncationMarker` temporarily restores this reserved space so
`TcgCommLogEvent` will accept the marker write.

OS-side and pre-OS consumers can detect FinalEventLog truncation by:

- walking `EFI_TCG2_FINAL_EVENTS_TABLE` and inspecting the last entry for an
  `EV_NO_ACTION` event whose payload begins with `"TCG Event Log Truncated"`.

## ACPI Event Log Truncation Marker

The **ACPI event log** uses the same truncation-marker mechanism as
FinalEventLog. Because it is fixed-size and does not scale, `TcgDxeLogEvent`
appends an `EV_NO_ACTION` truncation event whose payload is
`TCG_LOG_TRUNCATION_EVENT_STRING` when the next event would overflow the
region. `EventLogTruncated` is then set so subsequent writes silently skip the
ACPI region without re-appending the marker. Unlike FinalEventLog, the
truncated ACPI region does **not** cause `HashLogExtendEvent` to return
`EFI_VOLUME_FULL`.

To guarantee the marker always fits, the ACPI event log initialization in
`SetupEventLog` subtracts `GetTruncationEventSize()` from the usable `Laml`:

```c
mTcgDxeData.AcpiEventLogAreaStruct[Index].Laml =
    PcdGet32 (PcdTcgLogAreaMinLen) - GetTruncationEventSize ();
```

Truncation can be detected by walking the region and inspecting the last
entry for the `"TCG Event Log Truncated"` payload.

## SP800-155 PlatformId Events

Tcg2Dxe has dedicated handling for the **SP800-155 PlatformId Event**, an
`EV_NO_ACTION` measurement log entry that carries platform identification
data (manufacturer, model, firmware version, reference-measurement URI, etc.)
so an attestation verifier can identify which platform produced the
measurements that follow it in the log.

### Background: Why the SP800-155 Name Persists

The event type is named after
[NIST SP 800-155, *BIOS Integrity Measurement Guidelines*](https://csrc.nist.gov/pubs/sp/800/155/ipd),
which defined the original notion of a platform-identity measurement record.
That NIST draft was **withdrawn/retired** and never advanced past the initial
public draft, so it is no longer a normative reference on its own.

However, the [TCG PC Client Platform Firmware Profile (PFP) Specification](https://trustedcomputinggroup.org/resource/pc-client-specific-platform-firmware-profile-specification/)
adopted the same event and still defines it normatively:

- `TCG_Sp800_155_PlatformId_Event2` — signature `"SP800-155 Event2"`
- `TCG_Sp800_155_PlatformId_Event3` — signature `"SP800-155 Event3"`

Because the PFP is the governing spec for firmware event logs, tooling and
attestation stacks (OS-side TPM parsers, remote-attestation verifiers, CRTM
reference implementations) still look for these entries. That is why the
handling code remains in Tcg2Dxe even though the originating NIST document
is retired — the code is here to keep the driver **PFP-compliant**, not to
comply with the retired NIST draft directly.

### How Tcg2Dxe Handles the Event

The 800-155 events are supplied to Tcg2Dxe by an earlier boot phase through
GUIDed HOBs identified by `gTcg800155PlatformIdEventHobGuid`. During
`SetupEventLog`, Tcg2Dxe:

1. Logs the `TCG_EfiSpecIdEvent` header first (required to be the first entry
   in a Crypto Agile log per PFP 9.2).
2. Records the current end-of-log offset into
   `EventLogAreaStruct->Next800155EventOffset`. This marks the slot where
   800-155 events belong per PFP ordering rules — immediately after the
   SpecId event and before any measured events.
3. Walks every `gTcg800155PlatformIdEventHobGuid` HOB and logs each one as
   a `TCG_PCR_EVENT2` with `EventType = EV_NO_ACTION`.

### Insertion at the Reserved Offset

`TcgCommLogEvent` uses `Is800155Event` to detect an incoming 800-155 event
(matches `EV_NO_ACTION` + one of the SP800-155 signatures). When one is
detected and `Next800155EventOffset != 0`, the driver does **not** append the
event at the tail of the log. Instead it:

1. Shifts everything from `Next800155EventOffset` onward forward by
   `NewLogSize` bytes to open a gap.
2. Copies the new 800-155 header + payload into that gap.
3. Advances `Next800155EventOffset`, `LastEvent`, and `EventLogSize` by
   `NewLogSize`.

This preserves the PFP-required ordering (all 800-155 platform-identity
events appear as a contiguous block near the top of the log) even when an
800-155 event is produced later than the first measured event, or arrives
after the log has already been partially populated.

If `Next800155EventOffset == 0` (for example, the FinalEventLog, which does
not reserve an insertion slot — see the `SetupEventLog` initialization
setting `FinalEventLogAreaStruct[Index].Next800155EventOffset = 0`), the
800-155 event is silently dropped rather than appended out of order.

### Practical Notes

- Platforms that want their 800-155 identity in the log must produce
  `gTcg800155PlatformIdEventHobGuid` HOBs in a pre-DXE phase (SEC or PEI),
  since the HOB list is finalized at DXE handoff and Tcg2Dxe consumes these
  HOBs during `SetupEventLog`.
- Removing the 800-155 handling would break PFP conformance and prevent
  attestation clients from tying measurements to a specific platform SKU,
  so the code is intentionally retained despite the retirement of the NIST
  draft it is named after.
