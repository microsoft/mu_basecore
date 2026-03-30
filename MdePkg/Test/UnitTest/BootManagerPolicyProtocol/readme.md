# BootManagerPolicyProtocol Unit Test

## Overview

A UEFI Shell application that exercises the `EFI_BOOT_MANAGER_POLICY_PROTOCOL`
as defined in the UEFI Specification. The test locates a live protocol instance
via `gBS->LocateProtocol` and calls both `ConnectDevicePath` and
`ConnectDeviceClass` with every standard class GUID, plus a fabricated unknown
GUID to verify the expected error path.  Unfortunately, to test more of the actual connect
functionality the test would require much tigher control of the pre-conditions and configuration
of the system.  This would make the test more fragile and problematic to maintain.

## Prerequisites

- The firmware image must include `BootManagerPolicyDxe` (or another driver that
  installs `EFI_BOOT_MANAGER_POLICY_PROTOCOL`) so the protocol is available at
  test time.
- The test must run at `TPL_APPLICATION` (the default for UEFI Shell apps).

## Test Suites and Cases

### Suite 1 — Protocol Discovery

| Test | Description |
|------|-------------|
| `TestLocateProtocol` | Verifies `gBS->LocateProtocol` succeeds and both `ConnectDevicePath` and `ConnectDeviceClass` function pointers are non-NULL. |
| `TestProtocolRevision` | Verifies the protocol `Revision` field equals `EFI_BOOT_MANAGER_POLICY_PROTOCOL_REVISION` (`0x00010000`). |

### Suite 2 — ConnectDeviceClass with Known GUIDs

Each test calls `ConnectDeviceClass` with a standard class GUID and asserts the
return status is one of the spec-valid values: `EFI_SUCCESS`, `EFI_DEVICE_ERROR`,
`EFI_NOT_FOUND`, or `EFI_UNSUPPORTED`. The actual result depends on the platform
and available devices.

| Test | Class GUID | Comment |
|------|------------| --------|
| `TestConnectDeviceClassConsole` |`EFI_BOOT_MANAGER_POLICY_CONSOLE_GUID` |
| `TestConnectDeviceClassNetwork` | `EFI_BOOT_MANAGER_POLICY_NETWORK_GUID` |
| `TestConnectDeviceClassStorage` | `EFI_BOOT_MANAGER_POLICY_STORAGE_GUID` |
| `TestConnectDeviceClassConnectAll` | `EFI_BOOT_MANAGER_POLICY_CONNECT_ALL_GUID` | This test should always be last as it will connect all device types |

### Suite 3 — ConnectDeviceClass with Unknown GUID

| Test | Description |
|------|-------------|
| `TestConnectDeviceClassUnknownGuid` | Calls `ConnectDeviceClass` with a fabricated GUID (`DEADBEEF-1234-5678-...`). Strictly asserts the return is `EFI_NOT_FOUND`, per the UEFI Specification. |

### Suite 4 — ConnectDevicePath

Each test calls `ConnectDevicePath` with a `NULL` device path and asserts the
return status is spec-valid: `EFI_SUCCESS`, `EFI_NOT_FOUND`,
`EFI_SECURITY_VIOLATION`, or `EFI_UNSUPPORTED`. Per the UEFI Specification, a
`NULL` device path instructs the Boot Manager to connect all controllers using
platform policy.

| Test | Recursive | Description |
|------|-----------|-------------|
| `TestConnectDevicePathNull` | `FALSE` | NULL path, non-recursive. |
| `TestConnectDevicePathNullRecursive` | `TRUE` | NULL path, recursive (spec says Recursive is ignored when DevicePath is NULL). |

## Design Notes

- **Platform-tolerant assertions**: Known-GUID tests accept any spec-valid
  return code rather than requiring `EFI_SUCCESS`, since success depends on
  what hardware or virtual devices the platform provides.
- **Prerequisite fixture**: Every test case uses `LocateProtocolPreReq` which
  locates and caches the protocol pointer. If the protocol is not installed, all
  tests fail in the prerequisite and report a clear error.
- **DEBUG logging**: Each protocol call is bracketed with `DEBUG` prints so that
  the UEFI serial/debug log clearly shows which operation was invoked, aiding
  post-mortem triage.
