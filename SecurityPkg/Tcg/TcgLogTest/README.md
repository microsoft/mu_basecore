# TcgLogTest

TcgLogTest validates the dynamic event log scaling functionality implemented
by `Tcg2Dxe`. It consists of a DXE driver (`TcgLogTestDxe`) and a UEFI shell
unit test application (`TcgLogTestApp`) that coordinate across multiple boots
to exercise scaling both before and after `ReadyToBoot`.

## Components

### TcgLogTestDxe (DXE_DRIVER)

A DXE driver that runs pre-ReadyToBoot scaling tests on demand. It installs
the `TCG_LOG_TEST_PROTOCOL` which allows the test application to enable/disable
the tests and retrieve results.

**Entry flow:**

1. Installs the `TCG_LOG_TEST_PROTOCOL` on a new handle.
2. Checks the NV variable `TcgLogTestEnable` (existence-based: present =
   enabled, absent = disabled).
3. If disabled:
   - Returns immediately. The protocol is still available for the test app
     to call `Enable` on.
4. If enabled:
   - Deletes the enable variable. This makes it so the test only runs once.
   - Locates `EFI_TCG2_PROTOCOL`.
   - Runs `TestPreReadyToBootScaling`.
   - Records results in an internal log buffer which can be acquired via
     `GetLog`.

#### Protocol

The `TCG_LOG_TEST_PROTOCOL` provides the following function(s):

| Function | Description |
| -------- | ----------- |
| `GetLog` | Returns a pointer to the DXE driver's internal ASCII log buffer and its size. Returns `EFI_NOT_STARTED` if the test did not run this boot. |
| `Enable` | Creates or deletes the `TcgLogTestEnable` NV variable to enable or disable the DXE test for the next boot. |

The `TCG_LOG_TEST_PROTOCOL` GUID is defined in `TcgLogTest.h` and declared
in `SecurityPkg.dec`.

```code
#define TCG_LOG_TEST_PROTOCOL_GUID \
  { 0xA3C12F80, 0x7D9E, 0x4B5A, { 0x91, 0xE4, 0x6C, 0xF8, 0x2D, 0xA1, 0xB7, 0x03 } }
```

#### NV Variable

The enable/disable mechanism uses an NV variable rather than UnitTest saved
context because the DXE driver and the test application are separate binaries.
The DXE driver does not use `UnitTestLib` and cannot access the framework's
persisted state. An NV variable is the standard cross-module communication
channel in UEFI.

| Attribute | Value |
| --------- | ----- |
| Name | `TcgLogTestEnable` |
| Vendor GUID | `gTcgLogTestProtocolGuid` |
| Attributes | `NV + BS` |
| Semantics | Existence-based: variable present = enabled, variable absent = disabled |

#### Test: TestPreReadyToBootScaling

Executed before `ReadyToBoot` when the NV variable is present indicating the
test was enabled. Exercises dynamic scaling before `ReadyToBoot` has fired.

1. Calls `TcgLogTestLogEventsUntilScaled` to repeatedly log `EV_NO_ACTION`
   events to PCR 8 until the event log base address changes (indicating
   scaling occurred).
2. Calls `TcgLogTestDumpEventLog` to dump every event in the log and verify
   the normal log is **not** truncated.
3. Writes `PASS` or `FAIL` (with details) to the internal log buffer.

### TcgLogTestApp (UEFI_APPLICATION)

A UnitTest framework shell application that runs post-ReadyToBoot scaling
tests and collects pre-ReadyToBoot results from the DXE driver.

#### Test: TestPostReadyToBootScaling

Executed after `ReadyToBoot` in the UEFI shell. Exercises dynamic scaling
after `ReadyToBoot` has fired.

1. Retrieves the TPM2 ACPI table's `LAML`/`LASA` values via the ACPI SDT
   protocol.
2. Calls `TcgLogTestLogEventsUntilScaled` to repeatedly log `EV_NO_ACTION`
   events to PCR 8 until the event log base address changes (indicating
   scaling occurred).
3. Calls `TcgLogTestDumpEventLog` to dump every event in the log and verify
   the normal log is **not** truncated.
4. Calls `CheckTruncationEvent` to verify the `"TCG Event Log Truncated"`
   `NO_ACTION` marker **is** present in the ACPI log (since the ACPI log
   cannot scale and should have been marked truncated).

#### Test: TestPreReadyToBootResults

Verifies the DXE driver's pre-ReadyToBoot results.

1. Locates `TCG_LOG_TEST_PROTOCOL` and calls `GetLog`.
2. Dumps the DXE log for visibility.
3. Asserts the log contains `"PASS"` and does not contain `"FAIL"`.

## Three-Boot Reboot Flow

The tests require three boots to complete because scaling must be tested in
two different phases of the boot process, and each phase requires a separate
boot. The final boot should guarantee that the TCG event log is not polluted
with the test `NO_ACTION_EVENT` events used to scale the log.

```text
Boot 1 (TestApp Test)
├── TcgLogTestDxe:
│   ├── Installs TCG_LOG_TEST_PROTOCOL.
│   ├── NV variable absent → Test not enabled → SKIPPED.
├── TcgLogTestApp:
│   ├── Launched from UEFI shell. (UnitTest Framework)
│   ├── Test Prerequisites:
│   │   └── Calls LocateProtocols() to locate the TCG2 and TcgLogTest protocols.
│   ├── Calls TestPostReadyToBootScaling():
│   │   ├── Calls TcgLogTestLogEventsUntilScaled() to scale the event log.
│   │   ├── Verifies the log was not truncated.
│   │   ├── Checks ACPI truncation event.
│   │   └── PASS.
│   └── Test Cleanup:
│       └── Calls EnableDxeTestAndReboot().
│           ├── Calls Enable (TRUE) to create the NV variable.
│           └── Calls SaveAndReboot() to SaveFrameworkState + EfiResetCold.
│
Boot 2 (DXE Driver Test)
├── TcgLogTestDxe: 
│   ├── Installs TCG_LOG_TEST_PROTOCOL.
│   ├── NV variable present → Test enabled → Deletes the NV variable → Runs.
│   ├── Calls TestPreReadyToBootScaling():
│   │   ├── Calls TcgLogTestLogEventsUntilScaled() to scale the event log.
│   │   ├── Verifies the log was not truncated.
│   │   ├── PASS.
│   │   └── Logs results into internal buffer for later access via GetLog().
├── TcgLogTestApp:
│   ├── Resumes execution from UEFI shell. (UnitTest Framework)
│   ├── Test Prerequisite:
│   │   └── SKIPPED.
│   ├── Calls TestPostReadyToBootScaling():
│   │   └── Already PASSED in saved state → SKIPPED.
│   ├── Calls TestPreReadyToBootResults():
│   │   ├── Locates TcgLogTest protocol.
│   │   ├── Calls GetLog() to acquire the TcgLogTestDxe log.
│   │   ├── Verifies PASS in TcgLogTestDxe log.
│   │   └── PASS.
│   └── Test Cleanup:
│       └── Calls SaveAndReboot() to SaveFrameworkState + EfiResetCold.
│
Boot 3 (Final Report/Results)
├── TcgLogTestDxe:
│   ├── Installs TCG_LOG_TEST_PROTOCOL.
│   ├── NV variable absent → Test not enabled → Exit.
├── TcgLogTestApp:
│   ├── Resumes execution from UEFI shell. (UnitTest Framework)
│   ├── Both tests already PASSED → SKIPPED.
│   └── Reports final results, cleans up framework state.
```

## Shared Code (TcgLogTestCommon)

Common functions compiled into both binaries:

| Function | Description |
| -------- | ----------- |
| `TcgLogTestAdvanceEvent` | Parses one TCG 2.0 event entry, advancing the pointer to the next event. Handles SHA-1/256/384/512/SM3 digest algorithms. |
| `TcgLogTestLogEventsUntilScaled` | Builds a test event and logs it repeatedly via `HashLogExtendEvent` until `GetEventLog` reports a different base address. |
| `TcgLogTestDumpEventLog` | Calls `GetEventLog`, walks the entire log (skipping the TCG 1.2 SpecID header), and prints each event's index, PCR index, event type, and event size via `DEBUG`. Returns the truncation status. |

## Platform Integration

### DSC

Add both modules to the platform DSC under the `[Components]` section,
typically gated behind a TPM enable flag:

```ini
!if $(TPM2_ENABLE) == TRUE
  SecurityPkg/Tcg/TcgLogTest/TcgLogTestDxe.inf
  SecurityPkg/Tcg/TcgLogTest/TcgLogTestApp.inf
!endif
```

### FDF

Add both modules to the platform FDF so they are included in the firmware
volume, typically gated behind a TPM enable flag. The DXE driver must be in
the DXE FV so it loads during DXE dispatch. The test application can be in
the same FV or a separate one accessible from the UEFI shell:

```ini
!if $(TPM2_ENABLE) == TRUE
  INF SecurityPkg/Tcg/TcgLogTest/TcgLogTestDxe.inf
  INF SecurityPkg/Tcg/TcgLogTest/TcgLogTestApp.inf
!endif
```

### Running the Test

1. Boot to the UEFI shell.
2. Run the test application: `TcgLogTestApp.efi`
3. The system will automatically reboot twice more to complete the three-boot
   flow.
4. On the third boot, the framework reports final results to the shell.
