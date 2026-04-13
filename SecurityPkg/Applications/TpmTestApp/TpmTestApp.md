# TpmTestApp

A UEFI shell application for testing TPM 2.0 Physical Presence Interface operations,
including querying and configuring PCR banks. The app communicates exclusively through the
`EFI_TCG2_PROTOCOL` — it has no direct dependencies on TPM command libraries, physical
presence libraries, or TPM hardware.

## Table of Contents

- [Overview](#overview)
- [Commands](#commands)
- [Build Integration](#build-integration)
- [Protocol Dependency](#protocol-dependency)
- [Output and Debugging](#output-and-debugging)

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
| LibraryClasses | `BaseLib`, `ShellLib`, `UefiApplicationEntryPoint`, `UefiBootServicesTableLib`, `UefiLib` |
| Protocols | `gEfiTcg2ProtocolGuid` |

The app intentionally has **no dependency** on `SecurityPkg`, TPM command libraries,
physical presence libraries, or runtime services. All TPM interaction goes through the
TCG2 Protocol.

### DSC

Add the INF to the platform DSC components section:

```ini
SecurityPkg/Applications/TpmTestApp/TpmTestApp.inf
```

The app is typically included unconditionally (outside any TPM enable guard) so it can be
built regardless of TPM enablement. It will report a clear error when the TCG2 Protocol is
not available.

### FDF

Add the INF to the appropriate firmware volume in the platform FDF:

```ini
INF SecurityPkg/Applications/TpmTestApp/TpmTestApp.inf
```

## Protocol Dependency

TpmTestApp uses only `EFI_TCG2_PROTOCOL` (`gEfiTcg2ProtocolGuid`). This protocol is
installed by `Tcg2Dxe.efi`, which requires TPM to be enabled in the platform
configuration.

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

1. `PcdTpm2HashMask` gates which `HashInstanceLib` modules register.
2. `Tcg2Dxe` intersects the registered hash bitmap with the TPM's hardware capabilities.
3. `GetCapability` only reports the intersection.

This means the app may not see all algorithms the TPM hardware supports. To expose
additional algorithms, update `PcdTpm2HashMask` in the platform DSC.

## Output and Debugging

All output uses `Print()` from `UefiLib`, so it appears directly on the UEFI shell
console — no serial log or `AdvancedLogger` configuration is required to see results.
