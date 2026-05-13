---
name: EDK2-Style Workspace Instructions
description: Comprehensive instructions for working with EDK2-style C code, including file formats, build system, coding standards, and best practices.
applyTo: '**/*.{c,h,inf,dsc,dec,fdf}'
---
MANDATORY: Completely understand and follow this instructions file. Failure to do so will cost significant time and
effort.

# Repository Detection

These instructions apply to repositories containing:
- `.dec` files (Package Declaration files)
- `.dsc` files (Platform Description files)
- `.inf` files (Module Information files)
- `.fdf` files (Flash Description files)
- Standard edk2 package structure

# EDK2-Style C Code Instructions

This file provides comprehensive instructions for working with EDK2-style C code in repositories containing `.dec` files. These instructions cover edk2 build systems, file formats, coding standards, and best practices.

## EDK2 File Formats and Build System

### Core File Types

#### 1. DEC Files (Package Declaration)
- **Purpose**: Define package-level declarations including GUIDs, protocols, PPIs, library classes, and PCDs
- **Location**: Root of each package directory (e.g., `MdePkg/MdePkg.dec`)
- **Key Sections**:
  - `[Defines]`: Package metadata and version information
  - `[Includes]`: Include directory paths for header files
  - `[LibraryClasses]`: Library class declarations
  - `[Guids]`: GUID definitions with C-format GUID values
  - `[Protocols]`: Protocol GUID definitions
  - `[Ppis]`: PPI (PEIM-to-PEIM Interface) GUID definitions
  - `[PcdsFixedAtBuild]`, `[PcdsPatchableInModule]`, etc.: PCD declarations
- **Documentation**: https://github.com/tianocore-docs/edk2-DecSpecification

#### 2. DSC Files (Platform Description)
- **Purpose**: Define how to build a specific platform including component selection, library mappings, and PCD values
- **Location**: Platform-specific directories (e.g., `OvmfPkg/OvmfPkgX64.dsc`)
- **Key Sections**:
  - `[Defines]`: Platform metadata, output directories, supported architectures
  - `[LibraryClasses]`: Library class to implementation mappings
  - `[Components]`: List of modules to build for the platform
  - `[PcdsFixedAtBuild]`, `[PcdsPatchableInModule]`, etc.: PCD value assignments
  - `[BuildOptions]`: Compiler and linker flags
- **Documentation**: https://github.com/tianocore-docs/edk2-DscSpecification

#### 3. INF Files (Module Information)
- **Purpose**: Describe individual modules (drivers, libraries, applications) including sources, dependencies, and build requirements
- **Location**: Each module directory (e.g., `MdeModulePkg/Core/Dxe/DxeMain.inf`)
- **Key Sections**:
  - `[Defines]`: Module metadata, type, version, entry point
  - `[Sources]`: Source files to compile
  - `[Packages]`: Package dependencies (references to .dec files)
  - `[LibraryClasses]`: Required library classes
  - `[Protocols]`, `[Ppis]`, `[Guids]`: Interface dependencies
  - `[Pcd*]`: PCD usage declarations
  - `[Depex]`: Dependency expressions for load order
- **Documentation**: https://github.com/tianocore-docs/edk2-InfSpecification

#### 4. FDF Files (Flash Description)
- **Purpose**: Define flash device layout and firmware volume organization
- **Location**: Platform directories (e.g., `OvmfPkg/OvmfPkgX64.fdf`)
- **Key Sections**:
  - `[FD.*]`: Flash device definitions with layout and size
  - `[FV.*]`: Firmware volume definitions with file listings
  - `[Rule.*]`: Rules for processing different file types
  - `[Capsule.*]`: Capsule update definitions
- **Documentation**: https://github.com/tianocore-docs/edk2-FdfSpecification

### Build System Architecture

#### Build Process Overview
1. **Setup Phase**: Environment setup with `edksetup.bat` (Windows) or `edksetup.sh` (Linux)
2. **Parse Phase**: Parse DSC, FDF, INF, and DEC files to understand platform configuration
3. **AutoGen Phase**: Generate makefiles, dependency files, and AutoGen.c/AutoGen.h files
4. **Make Phase**: Compile source files and link binaries
5. **Flash Image Generation**: Create firmware volumes and flash images based on FDF

#### Key Build Tools
- **build.exe**: Main build command (Python-based)
- **GenFds**: Flash device image generation
- **GenFw**: Firmware file generation
- **VfrCompile**: Visual Forms Representation compiler
- **GenC**: C code generation for PCDs

#### Environment Variables
- `WORKSPACE`: Root directory of the edk2 source tree
- `PACKAGES_PATH`: Additional package search paths (colon/semicolon separated)
- `EDK_TOOLS_PATH`: Path to BaseTools directory
- `CONF_PATH`: Configuration file directory (defaults to Conf/)

### Platform Configuration Database (PCD) System

PCDs provide a standardized way to configure platform and module behavior without code changes.

#### PCD Types
- **FixedAtBuild**: Compile-time constants, no runtime overhead
- **PatchableInModule**: Can be modified by tools after compilation
- **Dynamic**: Runtime configurable via PCD database
- **DynamicEx**: Dynamic PCDs with extended token space support
- **FeatureFlag**: Boolean compile-time flags for conditional compilation

#### PCD Usage Patterns
- Declare in DEC files with default values and help text
- Override values in DSC files for platform-specific configuration
- Reference in INF files to indicate usage
- Access in C code using `PcdGet*()` and `PcdSet*()` macros

**Documentation**: https://github.com/tianocore-docs/edk2-PcdSpecification

## EDK2 Module Types and Architecture

### Module Types
- **SEC**: Security Phase, earliest boot code
- **PEI_CORE**: PEI Core module
- **PEIM**: PEI Module
- **DXE_CORE**: DXE Core module
- **DXE_DRIVER**: Standard DXE driver
- **DXE_RUNTIME_DRIVER**: DXE driver that remains resident during OS runtime
- **DXE_SMM_DRIVER**: System Management Mode driver
- **UEFI_DRIVER**: UEFI-compliant driver
- **UEFI_APPLICATION**: UEFI application
- **BASE**: Library module with no architectural dependencies

### Boot Phases
1. **SEC (Security)**: Initial CPU setup, temporary memory initialization
2. **PEI (Pre-EFI Initialization)**: Memory initialization, device discovery
3. **DXE (Driver Execution Environment)**: Full driver model, protocol installation
4. **BDS (Boot Device Selection)**: Boot manager, device enumeration
5. **TSL (Transient System Load)**: OS loader execution
6. **RT (Runtime)**: OS runtime services

## Coding Standards and Best Practices

### File Organization
- Use package-based organization (MdePkg, MdeModulePkg, etc.)
- Each package contains Library/, Universal/, Pci/, etc. subdirectories
- Include files in Include/ subdirectories with proper hierarchy
- Test files in separate test directories

### Header File Structure
```c
/** @file
  Brief description of the file.

  Detailed description if needed.

  Copyright (c) Year, Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Library/BaseLib.h>
// Other includes...

// Definitions, structures, function prototypes...
```

### Function Documentation
Use Doxygen-style comments:
```c
/**
  Brief function description.

  Detailed description if needed.

  @param[in]      Parameter1    Description of input parameter.
  @param[in, out] Parameter2    Description of input/output parameter.
  @param[out]     Parameter3    Description of output parameter.

  @retval EFI_SUCCESS          The operation completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid parameter passed.

**/
EFI_STATUS
EFIAPI
FunctionName (
  IN     UINT32  Parameter1,
  IN OUT VOID    *Parameter2,
  OUT    UINT32  *Parameter3
  );
```

## EDK2 C Coding Standards and Conventions

> **Reference**: https://github.com/tianocore-docs/edk2-CCodingStandardsSpecification

All EDK2-style C code must follow the official TianoCore C Coding Standards Specification. This section provides essential conventions and examples for consistent, readable, and maintainable firmware code.

### Naming Conventions

#### Identifiers (Variables and Functions)
- **Format**: PascalCase - each word capitalized, no underscores
- **Examples**: `MyVariable`, `GetSystemConfiguration`
- **Length**: No limit, but 10-30 characters recommended
- **Readability**: Names must clearly represent purpose

**Correct Examples**:
```c
UINTN                 BufferSize;
EFI_STATUS            Status;
EFI_SYSTEM_TABLE      *SystemTable;
MEMORY_DESCRIPTOR     *MemoryMap;
```

**Incorrect Examples**:
```c
UINTN buf_size;        // Underscores not allowed
int   buffer_sz;       // Standard C types not allowed
UINTN bs;              // Unclear abbreviation
```

#### Function and Data Names
- **Functions**: PascalCase (`AllocateMemory`, `InitializeDriver`)
- **Structure Types**: PascalCase (`DRIVER_INSTANCE`, `PROTOCOL_INTERFACE`)
- **No Hungarian notation** (except 'g' for global, 'm' for module, 'p' for pointer)
- **Acronyms**: Only first letter capitalized (`PciDevice`, not `PCIDevice`)

#### Macros and Typedefs
- **Format**: ALL_CAPS with underscores
- **Examples**: `MAX_BUFFER_SIZE`, `ALIGN_POINTER`, `EFI_SIGNATURE_32`

```c
#define MAX_DEVICE_COUNT        16
#define SIGNATURE_32(A, B, C, D) ((UINT32)(A) | ((UINT32)(B) << 8) | ((UINT32)(C) << 16) | ((UINT32)(D) << 24))
typedef UINT32 DEVICE_TYPE;
typedef enum {
  DeviceTypeUnknown,
  DeviceTypePci,
  DeviceTypeUsb
} DEVICE_CATEGORY;
```

#### Global and Module Variables
- **Global variables**: Prefix with 'g' (`gSystemTable`, `gBootServices`)
- **Module variables**: Prefix with 'm' (`mDriverInstance`, `mPrivateData`)
- **Pointer variables**: Optionally prefix with 'p' (`pBuffer`, `pDeviceList`)

```c
extern EFI_SYSTEM_TABLE     *gST;                // Global
extern EFI_BOOT_SERVICES    *gBS;                // Global
STATIC DRIVER_INSTANCE      mDriverInstance;     // Module scope
STATIC EFI_EVENT            mExitBootServicesEvent; // Module scope
```

### File Header Requirements

Every file must begin with a Doxygen file header:

```c
/** @file
  Brief description of the file's purpose.

  Detailed description of the file's contents and other useful
  information for a person viewing the file for the first time.

  Copyright (C) 2020 - 2024, Acme Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - UEFI Version 2.8
    - PI Version 1.7

  @par Glossary:
    - API  - Application Programming Interface
    - GUID - Globally Unique Identifier
**/

#pragma once

#include <Uefi.h>
#include <Library/BaseLib.h>
// Other includes...

// Definitions, structures, function prototypes...
```

### Include File Guards
- Use pragma once

### Function Documentation and Declaration

Use Doxygen-style comments for all functions:

```c
/**
  Allocates and initializes a memory buffer for device communication.

  This function allocates a buffer of the specified size and initializes
  it with default values for device communication protocols.

  @param[in]      DeviceType    Type of device requiring buffer allocation.
  @param[in]      BufferSize    Size in bytes of buffer to allocate.
  @param[out]     Buffer        Pointer to allocated and initialized buffer.
  @param[in, out] ActualSize    On input, requested size. On output, actual allocated size.

  @retval EFI_SUCCESS           Buffer allocated and initialized successfully.
  @retval EFI_INVALID_PARAMETER Invalid DeviceType or NULL pointer provided.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory to allocate buffer.
  @retval EFI_UNSUPPORTED       DeviceType not supported.

**/
EFI_STATUS
EFIAPI
AllocateDeviceBuffer (
  IN     DEVICE_TYPE  DeviceType,
  IN     UINTN        BufferSize,
     OUT VOID         **Buffer,
  IN OUT UINTN        *ActualSize OPTIONAL
  );
```

#### Function Declaration Formatting
- Opening brace in column 1 on its own line
- Parameters aligned with proper `IN`, `OUT`, `OPTIONAL` modifiers
- Return type on separate line
- `EFIAPI` calling convention when required

### Formatting and Spacing Rules

#### General Rules
- **No tabs**: Use spaces only
- **Indentation**: 2 spaces per level
- **Line length**: Maximum 120 characters
- **Line endings**: CRLF (0x0D 0x0A) for all source files (tool-specific files may differ if required)
- **File ending**: All source files must end with CRLF (tool-specific files may differ if required)

#### Vertical Spacing
- **One statement per line** - no exceptions
- **Blank lines**: Group related code blocks
- **Braces**: Opening brace on same line for simple predicates, own line for complex

```c
// Simple predicate - brace on same line
if (Status == EFI_SUCCESS) {
  ProcessSuccess ();
}

// Complex predicate - brace on own line
if ((DeviceType == PciDevice) &&
    (BufferSize > MIN_BUFFER_SIZE) &&
    (Buffer != NULL))
{
  AllocateBuffer ();
}
```

#### Horizontal Spacing
- **Binary operators**: Space before and after (`A + B`, `X == Y`)
- **Unary operators**: No space (`!Found`, `++Index`)
- **Commas/semicolons**: Space after if code follows
- **Parentheses**: Space before opening except for function calls
- **Function calls**: No space before opening parenthesis

```c
// Correct spacing
if (Index < MaxCount) {
  Result = (Value1 + Value2) * Multiplier;
  FunctionCall (Parameter1, Parameter2);
}

// Multi-line function calls
Status = gBS->AllocatePool (
                EfiBootServicesData,
                sizeof (DRIVER_INSTANCE),
                (VOID **)&PrivateData
                );
```

### Data Types and Declarations

#### UEFI Data Types Only
Use only UEFI-defined data types, never standard C types:

```c
// Correct UEFI types
BOOLEAN       Found;
UINT8         ByteValue;
UINT16        WordValue;
UINT32        DwordValue;
UINT64        QwordValue;
UINTN         NativeSize;
CHAR16        *UnicodeString;
VOID          *GenericPointer;
EFI_STATUS    Status;

// WRONG - Standard C types not allowed
bool          found;        // Use BOOLEAN
int           value;        // Use UINT32 or appropriate size
char          *string;      // Use CHAR8* or CHAR16*
```

#### Structure Definitions
- Always use typedef format
- Document each member
- Use alignment for readability

```c
/// Brief description of this structure.
/// Detailed description if needed.
typedef struct {
  UINT32                Signature;    ///< Unique structure identifier.
  EFI_HANDLE            Handle;       ///< Associated device handle.
  LIST_ENTRY            Link;         ///< Link to next instance.
  DEVICE_TYPE           Type;         ///< Type of device represented.
  BOOLEAN               Initialized;  ///< TRUE if instance is initialized.
} DRIVER_INSTANCE;

#define DRIVER_INSTANCE_SIGNATURE  SIGNATURE_32('D','R','V','I')
```

#### Enumerated Types
- Must end with maximum element
- Should begin with minimum element
- Document purpose of each member

```c
/// Device power states for power management.
typedef enum {
  DevicePowerStateMinimum = 0,    ///< Minimum valid power state.
  DevicePowerD0,                  ///< Fully powered and operational.
  DevicePowerD1,                  ///< Reduced power, context preserved.
  DevicePowerD2,                  ///< Lower power, some context lost.
  DevicePowerD3,                  ///< Lowest power, most context lost.
  DevicePowerStateMaximum         ///< Maximum power state value.
} DEVICE_POWER_STATE;
```

### Control Flow and Statements

#### Conditional Statements
```c
// Simple conditions
if (Found) {
  ProcessItem ();
}

if (Buffer != NULL) {
  FreePool (Buffer);
  Buffer = NULL;
}

// Complex conditions with explicit comparisons
if ((Index < ArraySize) &&
    (Array[Index] != NULL) &&
    (Status == EFI_SUCCESS))
{
  ProcessArrayElement (Array[Index]);
}

// Switch statements
switch (DeviceType) {
case PciDevice:
  InitializePciDevice ();
  break;

case UsbDevice:
  InitializeUsbDevice ();
  break;

default:
  return EFI_UNSUPPORTED;
}
```

#### Loop Constructs
```c
// For loops
for (Index = 0; Index < DeviceCount; Index++) {
  if (DeviceArray[Index] == NULL) {
    continue;
  }
  ProcessDevice (DeviceArray[Index]);
}

// While loops
while (MoreDataAvailable ()) {
  Data = GetNextData ();
  ProcessData (Data);
}

// Do-while loops
do {
  Status = AttemptOperation ();
  RetryCount++;
} while (EFI_ERROR (Status) && (RetryCount < MAX_RETRIES));
```

### Error Handling
- Always use EFI_STATUS return values for functions that can fail
- Use ASSERT() for debug-time validation of programmer errors
- Use proper EFI error codes
- Clean up resources on all error paths

```c
EFI_STATUS
EFIAPI
InitializeDevice (
  IN EFI_HANDLE  DeviceHandle
  )
{
  EFI_STATUS      Status;
  DEVICE_CONTEXT  *Context;

  if (DeviceHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Context = AllocateZeroPool (sizeof (DEVICE_CONTEXT));
  if (Context == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InitializeDeviceHardware (DeviceHandle, Context);
  if (EFI_ERROR (Status)) {
    FreePool (Context);
    return Status;
  }

  Status = RegisterDeviceProtocol (DeviceHandle, Context);
  if (EFI_ERROR (Status)) {
    CleanupDeviceHardware (Context);
    FreePool (Context);
    return Status;
  }

  return EFI_SUCCESS;
}
```

### Memory Management
- Use `AllocatePool()`, `AllocateZeroPool()` for dynamic allocation
- Always check for NULL returns from allocation functions
- Use `FreePool()` to release allocated memory
- Prefer stack allocation for small, temporary buffers
- Set pointers to NULL after freeing

```c
// Dynamic allocation
Buffer = AllocateZeroPool (BufferSize);
if (Buffer == NULL) {
  return EFI_OUT_OF_RESOURCES;
}

// Always free and nullify
if (Buffer != NULL) {
  FreePool (Buffer);
  Buffer = NULL;
}
```

### Comments and Documentation

#### Internal Comments
- Use C++ style comments (`//`) for local comments
- Blank line before comment blocks
- Comments explain why, not what
- Match indentation of code

```c
//
// Initialize the device list to prepare for enumeration.
// This must be done before any device detection operations.
//
Status = InitializeDeviceList ();

// Check if the device is already initialized to avoid duplicate work
if (Device->Initialized) {
  return EFI_ALREADY_STARTED;
}
```

#### What NOT to Comment
- No comment markers like `BUGBUG`, `FIX_THIS`, `TODO` in code
- No personal names or initials
- Use bug tracking systems instead of code markers

This comprehensive coding standard ensures consistent, readable, and maintainable EDK2-style C code that follows official TianoCore specifications and best practices.

## Common EDK2 Libraries and APIs

### Essential Libraries
- **BaseLib**: Basic CPU and string manipulation functions
- **BaseMemoryLib**: Memory operation functions
- **UefiLib**: UEFI-specific utility functions
- **UefiBootServicesTableLib**: Access to UEFI Boot Services
- **UefiRuntimeServicesTableLib**: Access to UEFI Runtime Services
- **DebugLib**: Debug printing and assertion functions
- **PrintLib**: Formatted string printing functions
- **DevicePathLib**: Device path manipulation
- **HiiLib**: Human Interface Infrastructure functions

### Common APIs
```c
// Debug printing
DEBUG ((DEBUG_INFO, "Message: %d\n", Value));

// Memory operations
CopyMem (Dest, Src, Size);
SetMem (Buffer, Size, Value);
ZeroMem (Buffer, Size);

// String operations (use safe variants - see edk2-c-code-security.instructions.md)
StrCpyS (Destination, DestMax, Source);
StrLen (String);
AsciiStrCmp (String1, String2);

// UEFI services access
Status = gBS->AllocatePool (EfiBootServicesData, Size, &Buffer);
Status = gRT->SetVariable (Name, Guid, Attributes, Size, Data);
```
## References and Documentation

### Official Specifications
- **Build Specification**: https://github.com/tianocore-docs/edk2-BuildSpecification
- **DEC Specification**: https://github.com/tianocore-docs/edk2-DecSpecification
- **DSC Specification**: https://github.com/tianocore-docs/edk2-DscSpecification
- **INF Specification**: https://github.com/tianocore-docs/edk2-InfSpecification
- **FDF Specification**: https://github.com/tianocore-docs/edk2-FdfSpecification
- **PCD Specification**: https://github.com/tianocore-docs/edk2-PcdSpecification
- **UNI Specification**: https://github.com/tianocore-docs/edk2-UniSpecification
- **VFR Specification**: https://github.com/tianocore-docs/edk2-VfrSpecification
- **Module Write Guide**: https://github.com/tianocore-docs/edk2-ModuleWriteGuide
- **UEFI Driver Writers Guide**: https://github.com/tianocore-docs/edk2-UefiDriverWritersGuide

### Key Resources
- **TianoCore.org**: Main EDK2 project website
- **EDK2 GitHub**: https://github.com/tianocore/edk2
- **EDK2 Documentation**: https://github.com/tianocore/tianocore.github.io/wiki
- **UEFI Specification**: Official UEFI Forum specifications
- **PI Specification**: Platform Initialization specifications

## Common Patterns and Examples

### Driver Entry Point
```c
EFI_STATUS
EFIAPI
DriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  // Initialize driver
  Status = InitializeDriver ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Install protocols
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gMyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMyProtocol
                  );

  return Status;
}
```

### Library Constructor
```c
EFI_STATUS
EFIAPI
LibraryConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  // Initialize library state
  return EFI_SUCCESS;
}
```

### Protocol Implementation
```c
typedef struct {
  UINT64               Signature;
  MY_PROTOCOL          Protocol;
  EFI_HANDLE           Handle;
  // Private data members
} MY_PROTOCOL_INSTANCE;

#define MY_PROTOCOL_INSTANCE_FROM_PROTOCOL(a) \
  CR (a, MY_PROTOCOL_INSTANCE, Protocol, MY_PROTOCOL_SIGNATURE)
```
