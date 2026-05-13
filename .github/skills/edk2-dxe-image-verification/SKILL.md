---
name: EDK2 DXE Image Verification Architect
description: "Important information when continuuing to develop or maintain the EDKII DXE Image Verification library or related components."
applyTo: 'SecurityPkg/Library/DxeImageVerificationLib2/**/*'
---

## EDK2 DXE Image Verification Architecture

## Prerequisites

**CRITICAL**: Before proceeding with this skill, you MUST first load and read these prerequisite skills:
1. Load the [EDK2 Project General Instructions](../edk2-project-general/SKILL.md) skill for foundational knowledge on EDK2-style C code, file formats, build system, coding standards, and best practices.
2. Load the [EDK2 C Code Security Guidelines](../edk2-c-code-security/SKILL.md) skill for comprehensive security guidelines specific to EDK2 firmware C code, including secure coding practices and secure code review principles.

**CRITICAL**: When developing or maintaining the EDKII DXE Image Verification Library or related components, it is critical to update this file with any relevant information about the architecture, design decisions, security considerations, or implementation details that future developers should be aware of. This ensures that knowledge is preserved and shared effectively, enabling future maintainers to understand the context and rationale behind the code, and to continue development in a consistent and informed manner. Failure to do so may lead to confusion, security issues, or inefficient development in the future.

**CRITICAL**: Additionally, a README.md file exists at SecurityPkg/Library/DxeImageVerificationLib2/README.md which provides a more human readable, high level overview of the library, its purpose, and how it fits into the larger EDKII architecture. It is important to keep both this SKILL.md file and the README.md file updated with relevant information to ensure comprehensive documentation for future developers working on this component.

## General Information

The DXE Image verification library is a NULL library implementation. This means that it does not implement a specific library class interface. Instead, when it is linked into a module, its constructor (DxeImageVerificationLibConstructor) will be called. Overall, this constructor does
two things: (1) Registers an event handler for the End of DXE event, and (2) installs an image
verification handler function (`DxeImageVerificationHandler`).

## Requirements

1. Functions should remain small and focused on a single respsonsibility.
2. Functions should be well documented, with clear explanations of their purpose, parameters, return values, and any side effects.
3. All supporting functions (i.e. any function that is not the constructor or the event handler) should be written in a way that allows them to be easily unit tested. When a function is created, unit tests should also be created to validate the functionality of that function.
4. No global variables should be used within any supporting functions and effort should be made to avoid global variables where possible. The only use of global variables allowed should be within the top level `OnReadyToBoot` event handler function and the `DxeImageVerificationHandler`
function. In those cases, the global variable should be returned as a pointer from a getter function that can be mocked during testing.
5. All files should have `\r\n` Line endings.

## Building the library

The below command can be used to build the library, and only the library. This is useful for development and testing purposes, as it allows for faster build times when only working on the library code.

This command will build the DxeImageVerificationLib.inf module, using the CLANGPDB toolchain, for both DEBUG and RELEASE configurations and for IA32, X64, AARCH64 architectures. You can view the build log at `Build/CI_BUILDLOG.txt` and you can also review the actual build artifacts, which will be located in the `Build/SecurityPkg/` directory. There are subfolders that are dependent on the configuration and toolchain, so ensure you are looking in the correct location for the generated files.

```bash
stuart_ci_build -c .pytool/CISettings.py -d CompilerPlugin=run -p SecurityPkg TOOL_CHAIN_TAG=CLANGPDB BUILDMODULE=SecurityPkg/Library/DxeImageVerificationLib2/DxeImageVerificationLib.inf
```

## Unit Testing

Developing unit tests is done via the `GoogleTest` framework. Documentation related to how to write unit tests using this framework can be found in at [UnitTestFrameworkPkg's README](../../../UnitTestFrameworkPkg/ReadMe.md). All unit tests are currently placed in a single unit test INF located at `SecurityPkg/Library/DxeImageVerificationLib2/GoogleTest/*`. Tests should be grouped into separate files based on functionality being tested, but all tests are consolidated to this single INF to make it easier to build and run.

To build and run the unit tests, you can use the below command, which will compile and run ONLY the test executable I specified below. This will both build and run unit tests and you can review the test results in the terminal, in the build output at `Build/SecurityPkg/`, and specifically the test results at `Build/TestSuites.xml`.

```bash
stuart_ci_build -c .pytool/CISettings.py -d  HostUnitTestCompilerPlugin=run -p SecurityPkg TOOL_CHAIN_TAG=GCC5 BUILDMODULE=SecurityPkg/Library/DxeImageVerificationLib2/GoogleTest/DxeImageVerificationLibGoogleTest.inf
```

### Code Coverage

Building and running unit tests to validate pass/fail criteria is only the first step for full validation. Eventually we need to be concerned with ensuring the written unit tests are providing sufficient code coverage. To do this, we can utilize the following command to not only compile and run the test, but also generate code coverage results that can be reviewed. The following command generates a coverage xml file at `Build/SecurityPkg/HostTest/<config>/SecurityPkg_coverage.xml` that can be reviewed to determine which lines of code are being covered by the unit tests and which are not. This coverage file will contain a large amount of coverage results, most of which we do not care about. The main focus should be on the coverage results for the actual library code (i.e. code in `SecurityPkg/Library/DxeImageVerificationLib2`). You should use this information to write more / better unit tests to ensure that all paths are covered. If possible, you should have 100% coverage.

```bash
stuart_ci_build -c .pytool/CISettings.py -d  HostUnitTestCompilerPlugin=run -p SecurityPkg TOOL_CHAIN_TAG=GCC5 BUILDMODULE=SecurityPkg/Library/DxeImageVerificationLib2/GoogleTest/DxeImageVerificationLibGoogleTest.inf CODE_COVERAGE=TRUE CC_REORGANIZE=TRUE CC_FULL=TRUE CC_FLATTEN=TRUE
```

## Code Formatting

Code formatting must be done for every commit. Formatting can be performed automatically using the following command. Do not waste time running it in the middle of active changes. Just run it once you believe the feature is finished.

```bash
stuart_ci_build -c .pytool/CISettings.py -d -p SecurityPkg UncrustifyCheck=run UNCRUSTIFY_IN_PLACE=TRUE
```

## Policy Resolution (`Policy.c` / `Policy.h`)

The first thing the verification handler must do for any image is decide
*how strict* it should be. That decision is driven by where the image
came from, encoded as one of the `IMAGE_*` image source constants, and
mapped to one of the `*_EXECUTE` / `*_ON_SECURITY_VIOLATION` policy
values defined in `Policy.h`.

The classification logic lives in `Policy.c` and is intentionally split
into small, single-responsibility helpers so each branch is unit
testable in isolation. All four classifier helpers share the same
signature and contract:

```c
EFI_STATUS
GetXxxImageType (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File,
  OUT UINT32                          *ImageType
  );
```

- `EFI_INVALID_PARAMETER` if either argument is `NULL`.
- `EFI_SUCCESS` only when the device path matches that helper's
  category; `*ImageType` is then populated.
- `EFI_NOT_FOUND` otherwise (without modifying `*ImageType`).

Helpers:

| Helper | Probe | On match `*ImageType` becomes |
| --- | --- | --- |
| `GetFirmwareVolumeImageType` | `LocateDevicePath` + `OpenProtocol(EFI_FIRMWARE_VOLUME2_PROTOCOL)` | `IMAGE_FROM_FV` |
| `GetBlockIoImageType` | `LocateDevicePath` + `OpenProtocol(EFI_BLOCK_IO_PROTOCOL)`; inspects `Media->RemovableMedia` | `IMAGE_FROM_REMOVABLE_MEDIA` or `IMAGE_FROM_FIXED_MEDIA` |
| `GetSimpleFileSystemImageType` | `LocateDevicePath(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL)` (used only when Block I/O didn't match) | `IMAGE_FROM_FIXED_MEDIA` |
| `GetDevicePathImageType` | Walks the device path looking for `MEDIA_RELATIVE_OFFSET_RANGE_DP` (option ROM) or `MSG_MAC_ADDR_DP` (network) nodes | `IMAGE_FROM_OPTION_ROM` or `IMAGE_FROM_REMOVABLE_MEDIA` |

`GetImageType` is the orchestrator. It validates inputs, then probes
the helpers in the order above. The first helper that returns
`EFI_SUCCESS` wins. If all four return `EFI_NOT_FOUND`, `GetImageType`
sets `*ImageType = IMAGE_UNKNOWN` and still returns `EFI_SUCCESS` — the
caller distinguishes "classified" vs. "unclassified" by inspecting
`*ImageType`, not the status. The only error `GetImageType` returns is
`EFI_INVALID_PARAMETER`.

`GetPolicyForImageType` is a pure function over the `IMAGE_*` value:

- `IMAGE_FROM_FV` -> `ALWAYS_EXECUTE` (firmware volumes are trusted by
  construction; this is hardcoded, not PCD-driven).
- `IMAGE_FROM_OPTION_ROM` -> `PcdOptionRomImageVerificationPolicy`.
- `IMAGE_FROM_REMOVABLE_MEDIA` -> `PcdRemovableMediaImageVerificationPolicy`.
- `IMAGE_FROM_FIXED_MEDIA` -> `PcdFixedMediaImageVerificationPolicy`.
- Anything else (notably `IMAGE_UNKNOWN`) -> `DENY_EXECUTE_ON_SECURITY_VIOLATION`.
  This is the **fail-closed** default and must be preserved by future
  maintainers.

`GetExecutionPolicy` is the public entry point used by the verification
handler. It composes the two: validates inputs, calls `GetImageType`,
bubbles up `EFI_INVALID_PARAMETER` if it fails, then writes
`*Policy = GetPolicyForImageType (ImageType)` and returns
`EFI_SUCCESS`. Because `GetImageType` cannot otherwise fail, callers
that get `EFI_SUCCESS` always have a defined `*Policy` value, including
the fail-closed value for unclassified sources.

### Design notes for future maintainers

- Keep the helpers' signatures uniform. The matrix-style design is
  what lets the unit tests exhaustively cover each branch with a
  single mock pattern.
- The classification order in `GetImageType` matters and mirrors the
  legacy `DxeImageVerificationLib`. Do not reorder without auditing
  the security implications (e.g., a malicious device path that
  matches multiple categories should still resolve to the most
  trustworthy category first).
- Never broaden the `default` arm of `GetPolicyForImageType` to
  something more permissive than `DENY_EXECUTE_ON_SECURITY_VIOLATION`.
  The verification handler relies on this fail-closed behavior for
  unrecognized image sources.
- The legacy `QUERY_USER_ON_SECURITY_VIOLATION` and
  `ALLOW_EXECUTE_ON_SECURITY_VIOLATION` constants are kept for ABI
  reasons but the verification handler explicitly rejects them — see
  the `ASSERT` in `DxeImageVerificationHandler`.
