# Tcg2Dxe

Tcg2Dxe is a DXE-phase UEFI driver that publishes the TCG2 protocol defined
by the [TCG EFI Protocol Specification](https://trustedcomputinggroup.org/resource/tcg-efi-protocol-specification/).
It's main responsibilites are to expose a standard interface to a TPM device,
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
   `TcgLogDynamicScalingNeeded` calculates whether the new event (plus a
   reserved truncation marker) would exceed the current allocation
   (`EventLogAreaStruct->Laml`). Space is reserved for the truncation marker
   as long as the ACPI log has not yet been marked truncated.

2. **Reallocation** — When scaling is needed, `TcgScaleEventLog` allocates a
   new `EfiBootServicesData` region at twice the current size, copies the
   existing log, updates the `Lasa`/`Laml` fields in the event log area
   struct, and frees the old region.

3. **Logging** — After scaling, the new event is logged into the resized buffer
   via `TcgDxeLogEvent` inside a TPL-raised critical section.

### Normal Log vs. ACPI Log vs. Final Events Log

Tcg2Dxe maintains three distinct event log regions:

| Log | Memory Type | Lifetime | Can Scale |
| --- | ----------- | -------- | --------- |
| **Normal log** | `EfiBootServicesData` | Available until `ExitBootServices` | Yes |
| **ACPI log** | `EfiACPIMemoryNVS` | Persistent | No |
| **Final Events log** | `EfiACPIMemoryNVS` | Persistent | No |

- The **Normal log** is the main log copy which is returned via `GetEventLog`.
  It can grow dynamically via scaling. Note that previous calls to `GetEventLog`
  could contain stale data if the log was scaled after. It is recommended to
  call `GetEventLog` each time access is required.
- The **ACPI log** is created at `ReadyToBoot` by `GenerateAcpiLog`. It
  allocates an `EfiACPIMemoryNVS` region equal to the **Normal log** size at that
  point, copies the log contents, and updates the TPM2 ACPI table's
  `LAML`/`LASA` fields so the OS can find it. If the TPM2 table was already
  installed via Tcg2Acpi/Tcg2AcpiFfa, `GenerateAcpiLog` will uninstall and
  reinstall the ACPI table with the updated LAML and LASA pointing to the
  newly allocated NVS region.
- The **Final Events log** (`EFI_TCG2_FINAL_EVENTS_TABLE`) records events
  logged after `GetEventLog` has been called. It is installed as a UEFI
  configuration table so the OS can discover events that occurred between its
  call to `GetEventLog` and `ExitBootServices`. Because the **Final Events log**
  does not scale, it can become truncated.

After `ReadyToBoot`, every new event is also appended to the ACPI log. Because
the ACPI log's NVS allocation is fixed (the OS may already be referencing its
address), it cannot be reallocated.

### Post-ReadyToBoot Truncation

When dynamic scaling is triggered after `ReadyToBoot`:

1. A `NO_ACTION` event with the payload `"TCG Event Log Truncated"` is
   appended to the **ACPI log** to notify the OS that the ACPI-visible log is
   now incomplete.
2. The ACPI log is marked truncated (`EventLogTruncated = TRUE`) so no further
   events are written to it and no additional space is reserved for the
   truncation marker.
3. The **normal log** is scaled as usual — it continues to grow and accept new
   events.

This means the normal log accessed via `GetEventLog` always has the complete
set of events, while the ACPI log visible to the OS may be truncated with a
marker at the end.
