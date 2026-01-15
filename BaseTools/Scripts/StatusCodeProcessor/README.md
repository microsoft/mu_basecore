# UEFI Status Code Processor

A Python tool for parsing and decoding UEFI/EDK2 status codes from debug logs,
including both standard PI specification codes and platform-specific custom codes.

## Overview

This tool analyzes UEFI status codes to help you understand:

- What type of error or progress event occurred
- Which module/driver reported it
- Platform-specific status code meanings
- The severity and classification of the event

**By default, the tool automatically discovers and loads platform-specific status code headers when a search path is
provided with `-s`. You can disable auto-discovery with `--no-auto-discover` if you only want standard PI
specification codes.**

## Features

✅ Parses both **Progress Codes** and **Error Codes**  
✅ Resolves **GUID** to module/driver names  
✅ Auto-discovers platform-specific status code headers  
✅ Supports custom/OEM status codes  
✅ Maintains macro definitions across multiple header files  
✅ Clean output by default (logging level: CRITICAL)  
✅ Optional verbose debug mode for troubleshooting  

---

## Quick Start

### Basic Usage (Standard PI Codes Only)

```bash
# Parse an error code - uses only PiStatusCode.h definitions (no search path)
python StatusCodeprocessor.py -e "ERROR: C40000002:V03040002 I0 12345678-ABCD-1234-5678-123456789ABC 00000001"

# Parse a progress code - uses only PiStatusCode.h definitions (no search path)
python StatusCodeprocessor.py -p "PROGRESS CODE: V03041001 I0"
```

### With Platform-Specific Codes (Auto-Discovery Enabled by Default)

```bash
# Auto-discovers platform-specific status codes when search path is provided
python StatusCodeprocessor.py -e "ERROR: C40000002:VDEADBEEF I0 12345678-ABCD-1234-5678-123456789ABC 00ABCDEF" \
  -s /path/to/workspace
```

### With Debug Output

```bash
# Enable verbose logging to see header discovery and parsing details
python StatusCodeprocessor.py -e "ERROR: C40000002:VDEADBEEF I0 12345678-ABCD-1234-5678-123456789ABC 00ABCDEF" \
  -s /path/to/workspace \
  --debug
```

### Disable Auto-Discovery (Standard PI Codes Only)

```bash
# Disable auto-discovery to use only standard PI specification codes
python StatusCodeprocessor.py -e "ERROR: C40000002:V03040002 I0 12345678-ABCD-1234-5678-123456789ABC 00000001" \
  -s /path/to/workspace \
  --no-auto-discover
```

---

## Command-Line Arguments

### Required Arguments (Choose One)

| Argument | Description |
| ---------- | ------------- |
| `-p CODE`, `--progress CODE` | Process a progress code string |
| `-e CODE`, `--error CODE` | Process an error code string |

### Optional Arguments

| Argument | Description |
| ---------- | ------------- |
| `-s PATH`, `--search PATH` | Path to search for module GUID definitions (.inf/.dec/.fdf files). When provided, automatically discovers and loads platform-specific status code headers (unless `--no-auto-discover` is used). |
| `-c HEADER [HEADER ...]`, `--platform-codes HEADER [HEADER ...]` | Explicitly specify additional platform-specific status code header file(s). Works together with auto-discovery. Use for files that don't match the auto-discovery pattern or to add extra headers. |
| `--no-auto-discover` | Disable automatic discovery of `*StatusCode*.h` headers. Use this flag to parse only standard PI specification codes even when a search path is provided. |
| `--debug` | Enable verbose debug output (DEBUG level logging). By default, logging is set to CRITICAL to minimize output. |
| `-h`, `--help` | Show help message and exit |

---

## Understanding Status Code Format

### Error Code Format

```bash
ERROR: C40000002:V03040002 I0 12345678-ABCD-1234-5678-123456789ABC 00000001
        ├───────┤ ├──────┤ ├─┤├──────────────────────────────────┤ ├──────┤
       CodeType   Value     │                 GUID                 Extended
                          Instance                                   Data
```

**Fields Explained:**

- **CodeType** (`C40000002`): Contains severity and error type
  - `0x40000000` = Severity (MINOR, MAJOR, UNRECOVERED, UNCONTAINED)
  - `0x00000002` = Type (ERROR_CODE)

- **Value** (`V03040002`): The actual status code
  - `0x03000000` = Class (SOFTWARE in this example)
  - `0x00040000` = Subclass (DXE_CORE)
  - `0x0002` = Operation/specific error number

- **Instance** (`I0`): Instance number (0 = first occurrence, or not applicable)

- **GUID**: Module/Driver GUID that reported the error

- **Extended Data**: Additional context-specific data

### Progress Code Format

```bash
PROGRESS CODE: V03041001 I0
               ├───────┤ ├┤
                 Value  Instance
```

**Fields Explained:**

- **Value** (`03041001`): The status code value
  - `0x03000000` = Class (SOFTWARE)
  - `0x00040000` = Subclass (DXE_CORE)
  - `0x1001` = Operation (PC_HANDOFF_TO_NEXT)

- **Instance** (`I0`): Instance number

---

## Examples

### Example 1: Standard PI Code (No Platform Headers)

**Command:**

```bash
python StatusCodeprocessor.py -p "PROGRESS CODE: V03041001 I0"
```

**Output:**

```text
=== Progress Code Analysis ===

Status Code Value: 0x03041001
Class:     SOFTWARE (0x03000000)
Subclass:  DXE_CORE (0x00040000)
Operation: PC_HANDOFF_TO_NEXT (0x1001)
```

**What This Tells You:**

- ✅ Uses only standard PiStatusCode.h definitions
- ✅ This is a standard **SOFTWARE** class progress code
- ✅ From the **DXE_CORE** phase
- ✅ Operation: **PC_HANDOFF_TO_NEXT** - DXE Core is transferring control to next stage
- ✅ This is normal boot progress, not an error

---

### Example 2: Error Code with Platform-Specific Codes (Auto-Discovery)

**Command:**

```bash
python StatusCodeprocessor.py \
  -e "ERROR: C40000002:VDEADBEEF I0 12345678-ABCD-1234-5678-123456789ABC 00ABCDEF" \
  -s /path/to/workspace
```

**Output:**

```text
=== Error Code Analysis ===

CodeType: 0x40000002
  Severity:  0x40000000 (EFI_ERROR_MINOR)
  Reserved:  0x00000000 (none)
  Type:      0x00000002 (EFI_ERROR_CODE)

Status Code Value: 0xDEADBEEF
  Platform Code: PLATFORM_CUSTOM_ERROR
  Source File:   PlatformStatusCodes.h
  Class:     UNKNOWN_CLASS (0xDE000000)
  Subclass:  OEM/CUSTOM (0x00AD0000)
  Operation: OEM/CUSTOM (0xBEEF)

  NOTE: Platform-specific status code: PLATFORM_CUSTOM_ERROR (from PlatformStatusCodes.h)
  Raw byte breakdown:
    Byte 3 (MSB): 0xDE
    Byte 2:       0xAD
    Byte 1:       0xBE
    Byte 0 (LSB): 0xEF

Instance: 0
Module:   SampleDriverDxe (12345678-ABCD-1234-5678-123456789ABC)
Extended Data: 0x00ABCDEF
```

**What This Tells You:**

- ✅ This is a **MINOR ERROR** (not critical)
- ✅ Platform-specific code **PLATFORM_CUSTOM_ERROR** from a custom header
- ✅ Reported by **SampleDriverDxe** driver
- ✅ Extended data provides additional context: `0x00ABCDEF`

---

### Example 3: With Additional Explicit Platform Headers

**Command:**

```bash
python StatusCodeprocessor.py \
  -e "ERROR: C40000002:V03040005 I0 ABCDEF01-2345-6789-ABCD-EF0123456789 00000001" \
  -s /path/to/workspace \
  -c /path/to/platform/CustomErrorCodes.h /path/to/platform/DebugCodes.h
```

**Use Case:**

- Auto-discovery finds most headers, but you need additional ones
- Explicitly add custom header files that don't match the `*StatusCode*.h` pattern
- All headers (auto-discovered + explicit) share the same macro definition pool
- **The `-c` option works together with auto-discovery to combine all headers**

---

### Example 4: Debug Mode

**Command:**

```bash
python StatusCodeprocessor.py \
  -e "ERROR: C40000002:VDEADBEEF I0 12345678-ABCD-1234-5678-123456789ABC 00ABCDEF" \
  -s /path/to/workspace \
  --debug
```

**Additional Debug Output Includes:**

```text
=== Discovering Platform Status Code Headers ===
Searching in: /path/to/workspace
  Found: Platform/CustomStatusCodes.h
  Found: Common/PlatformStatusCodes.h
Total discovered: 12 header file(s)

=== Parsing Platform Status Codes ===

Processing: CustomStatusCodes.h

  [DEBUG] Starting parse of CustomStatusCodes.h
  [DEBUG] Initial definitions count: 150
  [DEBUG] Some initial definitions: ['PLATFORM_RSC_CLASS', 'PLATFORM_SUBCLASS_ERROR', ...]
  [DEBUG] Found 8 simple definitions
  [DEBUG] Attempting to evaluate: PLATFORM_CUSTOM_ERROR = (PLATFORM_RSC_CLASS | PLATFORM_SUBCLASS_ERROR | 0x000000EF)
  [DEBUG] SUCCESS: PLATFORM_CUSTOM_ERROR = 0xDEADBEEF
  [DEBUG] Complex expressions: 6 succeeded, 0 failed
  [DEBUG] Final definitions count: 164
  [DEBUG] Status codes found: 6
  [DEBUG] Global definitions now has 164 entries

Processing: PlatformStatusCodes.h
  ...

=== Summary ===
Total platform status codes loaded: 45

Searching for GUID in: /path/to/workspace
  Found in: Build/Platform/DEBUG/Module/SampleDriverDxe/SampleDriverDxe.inf

=== Error Code Analysis ===

CodeType: 0x40000002
  Severity:  0x40000000 (EFI_ERROR_MINOR)
  Reserved:  0x00000000 (none)
  Type:      0x00000002 (EFI_ERROR_CODE)

Status Code Value: 0xDEADBEEF
  Platform Code: PLATFORM_CUSTOM_ERROR
  Source File:   PlatformStatusCodes.h
  Class:     UNKNOWN_CLASS (0xDE000000)
  Subclass:  OEM/CUSTOM (0x00AD0000)
  Operation: OEM/CUSTOM (0xBEEF)

  NOTE: Platform-specific status code: PLATFORM_CUSTOM_ERROR (from PlatformStatusCodes.h)
  Raw byte breakdown:
    Byte 3 (MSB): 0xDE
    Byte 2:       0xAD
    Byte 1:       0xBE
    Byte 0 (LSB): 0xEF

Instance: 0
Module:   SampleDriverDxe (12345678-ABCD-1234-5678-123456789ABC)
Extended Data: 0x00ABCDEF
```

**Note:** Debug mode changes logging level from CRITICAL to DEBUG, showing all internal processing details.

---

## Understanding the Output

### Severity Levels

| Severity | Meaning | Action Required |
| ---------- | --------- | ----------------- |
| `EFI_ERROR_MINOR` | Minor issue, system can continue | Investigate if recurring |
| `EFI_ERROR_MAJOR` | Major issue, functionality affected | Requires attention |
| `EFI_ERROR_UNRECOVERED` | Critical, system cannot recover | Immediate action required |
| `EFI_ERROR_UNCONTAINED` | Uncontained, may affect other components | Critical - immediate action |

### Code Types

| Type | Meaning |
| ------ | --------- |
| `EFI_PROGRESS_CODE` | Normal boot progress, not an error |
| `EFI_ERROR_CODE` | An error condition occurred |
| `EFI_DEBUG_CODE` | Debug information |

### Instance Field

- **Instance: 0** - First occurrence or not applicable
- **Instance: 1, 2, 3...** - Indicates which instance (e.g., which USB port, memory channel, etc.)

### Raw Byte Breakdown

Only shown for **OEM/Custom status codes** that don't follow standard UEFI PI specification.

**Why it's shown:**

- Helps understand vendor-specific encoding schemes
- Useful for reverse-engineering unknown platform codes
- Documents how the platform organizes custom status codes

**Example:**

```text
0xDEADBEEF = PLATFORM_CUSTOM_ERROR

Byte 3 (MSB): 0xDE  ← Platform class (PLATFORM_RSC_CLASS)
Byte 2:       0xAD  ← Subclass (PLATFORM_SUBCLASS_ERROR)
Byte 1:       0xBE  ← Reserved/category
Byte 0 (LSB): 0xEF  ← Specific error number
```

---

## Platform-Specific Status Codes

### Default Behavior (Auto-Discovery Enabled)

**When a search path is provided with `-s`**, the tool automatically discovers and loads platform-specific headers
matching these patterns:

- `*StatusCode*.h`
- `*StatusCodes*.h`

**Examples:**

- `PlatformStatusCodes.h`
- `CustomStatusCodeDefinitions.h`
- `VendorStatusCodes.h`

**Excluded:**

- Standard UEFI headers like `Pi/PiStatusCode.h`

**Without a search path**, the tool uses only the built-in UEFI PI specification definitions:

- Standard classes: COMPUTING_UNIT, PERIPHERAL, IO_BUS, SOFTWARE
- Standard subclasses and operations from PiStatusCode.h
- **No platform-specific or custom codes**

### Disable Auto-Discovery

Use `--no-auto-discover` to use only standard PI codes even when a search path is provided:

```bash
python StatusCodeprocessor.py -e "ERROR: ..." \
  -s /path/to/workspace \
  --no-auto-discover
```

### Additional Headers with `-c`

Use `-c` to add headers that don't match the auto-discovery pattern (works together with auto-discovery):

```bash
python StatusCodeprocessor.py -e "ERROR: ..." \
  -c /path/to/MyErrors.h /path/to/CustomCodes.h
```

### Macro Resolution

The tool maintains a **global macro pool** across all headers (both auto-discovered and explicit):

1. Base definitions from early files (e.g., `PLATFORM_RSC_CLASS = 0xDE000000`)
2. Platform-specific files can reference these macros
3. Later files inherit all previous definitions

**Example:**

`PlatformStatusCodes.h`:

```c
#define PLATFORM_RSC_CLASS          0xDE000000
#define PLATFORM_SUBCLASS_ERROR     0x00AD0000
```

`CustomStatusCodes.h` (processed later):

```c
#define PLATFORM_CATEGORY_CRITICAL  0x0000BE00
#define PLATFORM_CUSTOM_ERROR  (PLATFORM_RSC_CLASS | PLATFORM_SUBCLASS_ERROR | PLATFORM_CATEGORY_CRITICAL | 0x000000EF)
// ✅ Resolves to: 0xDE000000 | 0x00AD0000 | 0x0000BE00 | 0x000000EF = 0xDEADBEEF
```

---

## Logging Levels

The tool uses Python's logging module with different verbosity levels:

| Mode          | Logging Level | What's Shown                                                                       |
|---------------|---------------|------------------------------------------------------------------------------------|
| **Default**   | CRITICAL      | Only the final analysis output (no discovery/parsing details)                      |
| **--debug**   | DEBUG         | All internal processing, header discovery, macro evaluation, GUID search details   |

**Example - Default Output (Clean):**

```bash
python StatusCodeprocessor.py -e "ERROR: ..." -s /path
# Output: Only the final "=== Error Code Analysis ===" section
```

**Example - Debug Output (Verbose):**

```bash
python StatusCodeprocessor.py -e "ERROR: ..." -s /path --debug
# Output: Discovery messages, parsing details, macro resolution, GUID search, then analysis
```

---

## Troubleshooting

### "Module name not found in search path"

**Problem:** GUID is displayed but module name is not resolved.

**Solution:**

- Ensure `-s` points to the correct workspace root
- Check that `.inf`, `.dec`, or `.fdf` files exist in the workspace
- Use `--debug` to see search details

### "No status codes found" or Custom Code Not Recognized

**Problem:** Platform header is processed but no codes are extracted, or custom code shows as "UNKNOWN_CLASS".

**Possible Causes:**

1. **No search path provided with `-s`** - Auto-discovery requires a search path
2. **Auto-discovery disabled with `--no-auto-discover`** - Platform codes won't be loaded
3. Header doesn't have `#define` statements in expected format
4. Macro dependencies are not resolved
5. Header uses complex expressions that can't be evaluated

**Solution:**

- **Provide a search path with `-s` to enable auto-discovery**
- **Remove `--no-auto-discover` if present**
- Use `--debug` to see what's happening during parsing
- Check if the header uses standard C preprocessor syntax
- Ensure base macros are defined in earlier headers

### "Unresolved macros" Warning (Debug Mode)

**What it means:** A macro references another macro that hasn't been defined yet.

**Common Causes:**

- Header ordering issue (define used before its definition)
- Macro defined in a header that wasn't included
- Typo in macro name

**Solution:**

- Use `-c` to explicitly include the header with the missing definition
- Check header dependencies and include order

---

## Integration with Build Systems

### Example: Parse All Errors from Build Log

```bash
# Extract error codes and parse them
grep "ERROR: C" build.log | while read line; do
    python StatusCodeprocessor.py -e "$line" -s /path/to/workspace
    echo "---"
done
```

### Example: Create Error Summary

```bash
# Parse and save to file
python StatusCodeprocessor.py \
  -e "ERROR: C40000002:VDEADBEEF I0 12345678-ABCD-1234-5678-123456789ABC 00ABCDEF" \
  -s /path/to/workspace > error_analysis.txt
```

---

## Technical Details

### Status Code Structure (32-bit)

**CodeType:**

```text
31           24 23           16 15            8 7             0
┌──────────────┬──────────────┬──────────────┬──────────────┐
│   Severity   │   Reserved   │   Reserved   │  Code Type   │
└──────────────┴──────────────┴──────────────┴──────────────┘
```

**Value:**

```text
31           24 23           16 15            8 7             0
┌──────────────┬──────────────┬──────────────┬──────────────┐
│    Class     │   Subclass   │        Operation/Error      │
└──────────────┴──────────────┴──────────────┴──────────────┘
```

### Standard UEFI PI Classes

| Class | Value | Description |
| ------- | ------- | ------------- |
| COMPUTING_UNIT | 0x00000000 | CPU, Memory, Chipset |
| PERIPHERAL | 0x01000000 | Keyboard, Mouse, Storage |
| IO_BUS | 0x02000000 | PCI, USB, SCSI |
| SOFTWARE | 0x03000000 | PEI, DXE, Drivers |

### Requirements

- **Python 3.6+**
- **Standard library only** (no external dependencies)
- **Read access** to workspace files for GUID lookup

---

## Support

For issues or questions:

1. Check this README for common scenarios
2. Use `--debug` to see detailed parsing information
3. Review the UEFI PI Specification for standard status code definitions
4. Check platform-specific documentation for custom codes
5. Remember: **Platform codes are auto-discovered when using `-s` (disable with `--no-auto-discover` if needed)**

---

## Copyright

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
