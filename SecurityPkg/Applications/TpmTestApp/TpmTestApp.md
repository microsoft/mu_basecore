# TpmTestApp

A UEFI shell application for testing TPM 2.0 Physical Presence Interface operations,
including querying and configuring PCR banks. The app communicates exclusively through the
`EFI_TCG2_PROTOCOL` — it has no direct dependencies on TPM command libraries, physical
presence libraries, or TPM hardware.

## Table of Contents

- [Overview](#overview)
- [Commands](#commands)
- [Build Integration](#build-integration)
- [Running on QEMU SBSA](#running-on-qemu-sbsa)
- [Running on QEMU Q35](#running-on-qemu-q35)
- [Protocol Dependency](#protocol-dependency)
- [Expected Behavior with MinimumLib](#expected-behavior-with-minimumlib)
- [Output and Debugging](#output-and-debugging)
- [Findings](#findings)

## Overview

TpmTestApp is a `UEFI_APPLICATION` that validates TPM 2.0 PCR bank operations via the
TCG2 Protocol. It is designed to test that a platform's Physical Presence Interface
correctly handles PCR bank change requests, including verifying that unsupported operations
are gracefully rejected.

The app uses positional command-line arguments (no flags or switches) and outputs all
results through `Print()` (from `UefiLib`), which writes directly to the UEFI shell
console.

## Commands

```text
TpmTestApp help             Show usage information
TpmTestApp info             Show supported and active PCR banks
TpmTestApp setpcr <mask>    Request a PCR bank configuration change
TpmTestApp logall           Request enabling all supported PCR banks
TpmTestApp lastresponse     Show the result of the last SetActivePcrBanks call
```

### `help`

Prints usage information, PCR bank bitmask values, and examples.

### `info`

Calls `Tcg2Protocol->GetCapability()` and displays:

- **Supported PCR banks** — algorithms the TPM hardware supports (`HashAlgorithmBitmap`).
- **Active PCR banks** — algorithms currently enabled (`ActivePcrBanks`).

Both are filtered by the firmware's registered hash algorithms (see
[Hash Algorithm Filtering](#hash-algorithm-filtering) below).

Example output:

```text
[TPM 2.0 Information]

Supported PCR banks: 2
 * SHA256

Active PCR banks: 2
 * SHA256
```

### `setpcr <mask>`

Calls `Tcg2Protocol->SetActivePcrBanks()` with the provided hex bitmask. The mask is a
combination of `EFI_TCG2_BOOT_HASH_ALG_*` values:

| Value | Algorithm |
| ----- | --------- |
| `0x01` | SHA1 |
| `0x02` | SHA256 |
| `0x04` | SHA384 |
| `0x08` | SHA512 |
| `0x10` | SM3_256 |

Values can be combined: `0x06` = SHA256 + SHA384.

The mask parameter accepts hex with or without a `0x` prefix.

```text
TpmTestApp setpcr 0x2       Enable SHA256 only
TpmTestApp setpcr 0x6       Enable SHA256 + SHA384
TpmTestApp setpcr 2         Also valid (no prefix)
```

> **Note**: `SetActivePcrBanks` submits a Physical Presence request. The actual bank
> change takes effect on the next reboot, processed by
> `Tcg2PhysicalPresenceLibProcessRequest()` during BDS.

### `logall`

Calls `GetCapability()` to discover all supported hash algorithms, then calls
`SetActivePcrBanks()` with the full `HashAlgorithmBitmap`. This is equivalent to
requesting that all supported PCR banks be enabled.

### `lastresponse`

Calls `Tcg2Protocol->GetResultOfSetActivePcrBanks()` which queries the Physical Presence
library for the result of the most recent `SetActivePcrBanks` operation. Displays:

- **Operation present** — whether a previous operation result exists (`YES` / `NO`).
- **Response code** — the TCG PP return code (0 = success).

## Build Integration

### INF

The app is defined in `SecurityPkg/Applications/TpmTestApp/TpmTestApp.inf`:

| Property | Value |
| -------- | ----- |
| `MODULE_TYPE` | `UEFI_APPLICATION` |
| `ENTRY_POINT` | `TpmTestAppEntry` |
| `FILE_GUID` | `A3B2D4F1-7E6C-4A89-B5D0-3C1F8E2A9D07` |

Dependencies:

| Section | Items |
| ------- | ----- |
| Packages | `MdePkg`, `MdeModulePkg`, `ShellPkg` |
| LibraryClasses | `BaseLib`, `DebugLib`, `ShellLib`, `UefiApplicationEntryPoint`, `UefiBootServicesTableLib` |
| Protocols | `gEfiTcg2ProtocolGuid` |

The app intentionally has **no dependency** on `SecurityPkg`, TPM command libraries,
physical presence libraries, or runtime services. All TPM interaction goes through the
TCG2 Protocol.

### DSC

Add the INF to the platform DSC components section:

```ini
SecurityPkg/Applications/TpmTestApp/TpmTestApp.inf
```

The app is typically included unconditionally (outside any `!if $(TPM_ENABLE)` guard) so
it can be built regardless of TPM enablement. It will report a clear error when the TCG2
Protocol is not available.

### FDF

Add the INF to the appropriate firmware volume:

```ini
INF SecurityPkg/Applications/TpmTestApp/TpmTestApp.inf
```

On SBSA, this goes in `FV.FvMain`. On Q35, this goes in `FV.DXEFV`.

## Running on QEMU SBSA

On SBSA, the TpmTestApp is placed directly in the firmware volume, which is mapped as an
`FSx:` device in the UEFI shell. Build with:

```bash
stuart_build -c Platforms/QemuSbsaPkg/PlatformBuild.py --FlashRom \
  BLD_*_TPM2_ENABLE=TRUE \
  TPM_DEV=/tmp/mytpm1/swtpm-sock
```

At the UEFI shell, run:

```text
Shell> TpmTestApp.efi info
```

Or navigate to the correct FS mapping first:

```text
Shell> map -r
Shell> FS0:
FS0:\> TpmTestApp.efi info
```

## Running on QEMU Q35

On Q35, shell applications are loaded from a virtual drive (FAT filesystem image), not
directly from the firmware volume. The `FILE_REGEX` build parameter controls which
binaries are copied to the virtual drive:

```bash
stuart_build -c Platforms/QemuQ35Pkg/PlatformBuild.py --FlashRom \
  BLD_*_TPM_ENABLE=TRUE \
  TPM_DEV=/tmp/mytpm1/swtpm-sock \
  FILE_REGEX=TpmTestApp.efi
```

At the UEFI shell, find the virtual drive's FS mapping (typically backed by a PCI device,
not `Fv(...)`) and run:

```text
Shell> map -r
Shell> FS0:
FS0:\> TpmTestApp.efi info
```

If the app is not found, verify the virtual drive was created with the binary:

```text
Shell> FS0:
FS0:\> dir
```

## Protocol Dependency

TpmTestApp uses only `EFI_TCG2_PROTOCOL` (`gEfiTcg2ProtocolGuid`). This protocol is
installed by `Tcg2Dxe.efi`, which only loads when `TPM_ENABLE=TRUE` (Q35) or
`TPM2_ENABLE=TRUE` (SBSA).

If the protocol is not found, the app prints:

```text
TCG2 Protocol not found - Not Found
  TPM 2.0 may not be enabled on this platform.
```

### Protocol Functions Used

| Function | Command | Purpose |
| -------- | ------- | ------- |
| `GetCapability` | `info`, `logall` | Query supported/active PCR banks |
| `SetActivePcrBanks` | `setpcr`, `logall` | Submit PP request for bank change |
| `GetResultOfSetActivePcrBanks` | `lastresponse` | Query result of last bank change request |

### Hash Algorithm Filtering

The `HashAlgorithmBitmap` and `ActivePcrBanks` reported by `GetCapability` are already
filtered by the firmware's hash library configuration. The chain is:

1. `PcdTpm2HashMask` (`0x02` = SHA256 on both platforms) gates which `HashInstanceLib`
   modules register.
2. `Tcg2Dxe` intersects the registered hash bitmap with the TPM's hardware capabilities.
3. `GetCapability` only reports the intersection.

This means the app will typically only see SHA256 as supported and active, even though
swtpm may support all five algorithms. To see additional algorithms, update
`PcdTpm2HashMask` in the platform DSC.

## Expected Behavior with MinimumLib

Both QEMU platforms use `DxeTcg2PhysicalPresenceMinimumLib` when TPM is enabled. This
library has specific behaviors that affect TpmTestApp results:

### `setpcr` with Already-Active Banks

When the requested banks match the currently active banks, `Tcg2Dxe` sends
`TCG2_PHYSICAL_PRESENCE_NO_ACTION` to the PP library. MinimumLib processes `NO_ACTION`
successfully, returning `EFI_SUCCESS`.

```text
Shell> TpmTestApp setpcr 0x2    (SHA256 already active)
Submitting SetActivePcrBanks with parameter 2
Status: Success
Request submitted. Changes will take effect after reboot.
```

### `setpcr` with Different Banks

When the requested banks differ from active banks, `Tcg2Dxe` sends
`TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS` to the PP library. MinimumLib rejects this
operation as it only supports Clear operations.

```text
Shell> TpmTestApp setpcr 0x4    (SHA384, not currently active)
Submitting SetActivePcrBanks with parameter 4
Status: Unsupported
SetActivePcrBanks failed.
```

This is **expected and correct behavior** — MinimumLib is designed to block PCR bank
changes.

### `logall` Expected Behavior

If the platform only has SHA256 registered (default), `logall` requests the same bank
that's already active, resulting in a `NO_ACTION` → `EFI_SUCCESS`. If additional
algorithms were registered, it would request banks different from the active set,
triggering a `SET_PCR_BANKS` rejection.

### `lastresponse` Expected Behavior

Reports the result stored in the `Tcg2PhysicalPresence` NV variable by
`ProcessRequest`. If no prior `SetActivePcrBanks` was processed through a reboot cycle,
the response will show no operation present.

```text
Shell> TpmTestApp lastresponse
  Operation present: NO
  Response code:     0
```

## Output and Debugging

All output uses `Print()` from `UefiLib`, so it appears directly on the UEFI shell
console — no serial log or `AdvancedLogger` configuration is required to see results.

## Findings

**April 2026:**

Manually updating Tpm2HashMask in the platform .dsc is dangerous. On Q35, PEI will
pick up the change and auto enable/disable any PCR banks that differ from the current
active banks in the TPM. (See Tcg2Pei - SyncPcrAllocationsAndPcrMask) If the platform
also disables (i.e. removes) any of the supported hashing algorithms from the platform
.dsc there are no safeguards to prevent the TPM from enabling/disabling any active banks
based on platform support. Because of this there can be active banks that will show up
as active when querying the TPM but will not show up as active in the Tcg2Protocol call.

Example:

- Tpm2HashMask is updated to 0x06 (i.e. SHA256 + SHA384 supported).
- SHA384 support is removed from Tcg2Pei and Tcg2Dxe.
- Build/Run the Q35 platform.
- TPM reports SHA256 as the only active PCR bank.
- Tcg2Pei recognizes there is a mismatch between platform support (i.e. Tpm2HashMask)
  and TPM active banks. Activates SHA384 into a ResetCold.
- TPM reports SHA256 + SHA384 as active PCR banks.
- HashLibCryptoRouterPei and HashLibCryptoRouterDxe set PcdTcg2HashAlgorithmBitmap based on
  successfully registered hash algorithms.
- Tcg2Pei registers SHA256 but is unable to register SHA384.
- Tcg2Dxe registers SHA256 but is unable to register SHA384.
- Tcg2Dxe sets local variables based on what the TPM reports and PcdTcg2HashAlgorithmBitmap.

  ```C
  mTcgDxeData.BsCap.HashAlgorithmBitmap = TpmHashAlgorithmBitmap & PcdGet32 (PcdTcg2HashAlgorithmBitmap);
  mTcgDxeData.BsCap.ActivePcrBanks      = ActivePCRBanks & PcdGet32 (PcdTcg2HashAlgorithmBitmap);
  ```

- Tcg2Protocol is installed with only SHA256 support reported even though SHA384 is active.
