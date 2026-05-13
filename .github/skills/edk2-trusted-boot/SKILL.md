---
name: EDK2 Trusted Boot Chain and Firmware Security Architecture
description: "Use when working on TPM measurement, PCR allocation, TCG event logs, trusted boot chain, attestation, SecurityPkg/Tcg modules, Boot Guard integration, MOR variables, or OPAL storage security."
---

# Trusted Boot Chain and Firmware Security Architecture

## Overview of Firmware Security Concepts

Understanding trusted boot chains is essential for implementing secure firmware that integrates with platform security architecture. The TCG (Trusted Computing Group) specifications define how firmware components are measured, stored, and verified to establish a chain of trust from hardware to operating system.

### Key Security Specifications
- **NIST SP800-155**: Guidelines for platform firmware resiliency and measurement
- **TCG Platform Firmware Profile (PFP)**: Defines firmware measurement requirements
- **TCG Reference Integrity Manifest (RIM-IM)**: Framework for firmware integrity manifests
- **TCG PC Client RIM**: Reference implementation for PC platforms
- **TCG Firmware Integrity Measurement (FIM)**: Standards for firmware measurement

### Root of Trust for Measurement (RTM)
The platform firmware acts as a Static Root of Trust for Measurement (SRTM), which:
- Measures firmware components into TPM Platform Configuration Registers (PCR)
- Records measurement actions in an event log
- Establishes a chain of trust through cryptographic hashing
- Enables remote attestation for platform verification

## TPM Platform Configuration Registers (PCR)

### PCR Allocation and Usage

**PCR extend equation**: `PCR(new) = HASH(PCR(old) || HASH(Data))`

| **PCR Index** | **Purpose** | **Content** |
|---------------|-------------|-------------|
| **PCR[0]** | SRTM, BIOS, Host Platform Extensions | Platform firmware (PEI, DXE, SMM), embedded option ROMs, PI drivers, ACPI tables, non-host components |
| **PCR[1]** | Host Platform Configuration | Microcode, SMBIOS tables, setup variables, policy configuration, device lists |
| **PCR[2]** | UEFI Driver and Application Code | Third-party UEFI drivers, option ROMs, non-host updatable components, SPDM device firmware |
| **PCR[3]** | UEFI Driver and Application Configuration | Third-party configuration data, non-host updatable config, SPDM device configuration |
| **PCR[4]** | UEFI Boot Manager Code | OS loader, boot attempts, pre-OS applications |
| **PCR[5]** | Boot Manager Configuration and Data | GPT/partition tables, ExitBootServices events |
| **PCR[6]** | Host Platform Manufacturer Specific | Platform-specific OEM data |
| **PCR[7]** | Secure Boot Policy and Authority | Secure boot variables (PK, KEK, db, dbx), debug mode, DMA protection status |

### PCR Usage Rules

**Code vs Configuration Pattern**:
- Even-numbered PCRs (0,2,4,6): Code components
- Odd-numbered PCRs (1,3,5,7): Configuration and data

**Ownership Pattern**:
- PCR[0-1]: OEM platform firmware
- PCR[2-3]: Third-party components
- PCR[4-5]: OS boot related
- PCR[7]: Security policy

## Measurement Implementation in EDK2

### Key Modules and Libraries

**PEI Phase Measurement**:
- `SecurityPkg/Tcg/Tcg2Pei` - Main measurement module for PEI
- `SecurityPkg/Library/PeiTpmMeasurementLib` - PEI measurement services
- Measures firmware volumes (FV) at granularity level
- Records measurements in `EFI_TCG_EVENT2_HOB`

**DXE Phase Measurement**:
- `SecurityPkg/Tcg/Tcg2Dxe` - Main measurement module for DXE
- `SecurityPkg/Library/DxeTpm2MeasureBootLib` - PE image measurement
- Measures individual PE images through `EFI_SECURITY2_ARCH_PROTOCOL`
- Handles TCG event log management

**Common Libraries**:
- `SecurityPkg/Library/TcgEventLogRecordLib` - Event log recording services
- `MdeModulePkg/Library/TpmMeasurementLib` - Generic measurement interface
- `SecurityPkg/Library/HashLibBaseCryptoRouter` - Crypto-agile hash support

### Measurement Event Types

| **Event Type** | **Usage** | **PCR** |
|----------------|-----------|---------|
| `EV_S_CRTM_VERSION` | SRTM version string | PCR[0] |
| `EV_EFI_PLATFORM_FIRMWARE_BLOB` | Firmware volumes | PCR[0] |
| `EV_EFI_BOOT_SERVICES_DRIVER` | UEFI drivers | PCR[2] |
| `EV_EFI_BOOT_SERVICES_APPLICATION` | UEFI applications | PCR[4] |
| `EV_EFI_VARIABLE_DRIVER_CONFIG` | Secure boot variables | PCR[7] |
| `EV_EFI_VARIABLE_BOOT` | Boot variables | PCR[1] |
| `EV_SEPARATOR` | Boot phase separator | PCR[0-7] |
| `EV_EFI_ACTION` | Boot actions and events | Various |

### Measurement Exclusion and Pre-hashing

**Measurement Exclusion PPI**:
```c
// Exclude already-measured FV (e.g., by hardware root of trust)
EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI
```

**Pre-hashed FV Support**:
```c
// Use hardware-provided hash instead of re-calculating
EDKII_PEI_FIRMWARE_VOLUME_INFO_PREHASHED_FV_PPI
```

**Stored Hash Verification**:
```c
// Verify FV against stored hash, then use for measurement
EDKII_PEI_FIRMWARE_VOLUME_INFO_STORED_HASH_FV_PPI
```

## Platform Security Implementation

### Hardware Root of Trust Integration

**Intel Boot Guard Integration**:
- ACM (Authenticated Code Module) measures initial FV
- Platform reports `EV_S_CRTM_CONTENTS` with ACM, Key Manifest, Boot Policy Manifest
- `TCG_EfiStartupLocalityEvent` for Locality 3 startup
- Pre-hashed FV support for Boot Guard verified components

**Error Handling and Recovery**:
- TPM error detection creates `EFI_TPM_ERROR` HOB
- Status code reporting via `REPORT_STATUS_CODE()`
- Error separator events (`EV_SEPARATOR` with 0x00000001) cap PCRs
- Platform-specific error handling callbacks

### TPM Device Management

**Device Selection and Interface**:
- Support TPM 2.0 only (no TPM 1.2)
- FIFO and CRB interface detection
- Dynamic interface type configuration via `PcdActiveTpmInterfaceType`
- Device startup coordination via PPIs

**Bank Selection and Crypto Agility**:
- Multiple hash algorithm support (SHA256, SHA384, SHA512, SM3)
- Runtime bank reconfiguration via Physical Presence
- Hash algorithm capability synchronization
- Crypto-agile event log support

### Physical Presence and Management

**Physical Presence Operations**:
- TPM Clear, Enable/Disable hierarchies
- PCR bank selection and reconfiguration
- Platform hierarchy authentication
- Vendor-specific extensions

**Management Flow**:
1. OS submits PP request via ACPI `_DSM` method
2. Request stored in `Tcg2PhysicalPresence` variable
3. Platform BDS processes request before EndOfDxe
4. User confirmation for destructive operations
5. TPM configuration and system reset

## Security Policy Implementation

### Memory Overwrite Request (MOR)

**MOR Variable Management**:
- `MemoryOverwriteRequestControl` - Controls memory clearing
- `MemoryOverwriteRequestControlLock` - Protects MOR variable
- SMM-based variable protection against tampering
- Secret key unlock mechanism (MORLock v2)

**Platform Integration**:
- Memory initialization checks MOR variable
- Clear all system memory if MOR requested
- TPer reset for TCG storage devices
- Trusted storage connection before EndOfDxe

### TCG Storage Security

**OPAL Password Management**:
- User password prompt and validation
- S3 auto-unlock via secure LockBox storage
- Password update and device management
- PSID revert and secure erase capabilities

**BlockSid Protection**:
- Prevent unauthorized SID access
- Physical Presence controlled policy
- Applied during normal boot and S3 resume
- Blocks malicious device locking attacks

## Attestation and Verification

### Remote Attestation Flow

**Device Verification**:
1. TPM Endorsement Key (EK) verification against vendor CA
2. Attestation Key (AK) challenge-response protocol
3. TPM device authenticity confirmation

**Event Log Verification**:
1. Quote generation with AK-signed PCR values
2. Event log replay to reproduce PCR values
3. Comparison with Reference Integrity Manifests (RIM)
4. Policy-based validation of measured components

### Event Log Management

**Event Log Interfaces**:
- `EFI_TCG2_PROTOCOL.GetEventLog()` - Runtime access
- `EFI_TCG2_FINAL_EVENTS_TABLE` - Post-GetEventLog events
- TPM2 ACPI table event log exposure
- Event log format compliance with TCG specifications

## Platform Developer Checklist

### Essential Implementation Requirements

**TPM Measurement**:
- [ ] Configure `PcdTcgPfpMeasurementRevision` for compliance
- [ ] Set `PcdFirmwareVersionString` for version measurement
- [ ] Report all FV information in PEI phase
- [ ] Link `DxeTpm2MeasureBootLib` as LAST instance for SecurityStubDxe
- [ ] Measure platform-specific components (microcode, ACPI, SMBIOS)

**Device Configuration**:
- [ ] Use TPM 2.0 only (no TPM 1.2 support)
- [ ] Link appropriate `Tpm2DeviceLib` instance
- [ ] Configure TPM interface detection and selection
- [ ] Set `PcdTpm2InitializationPolicy` appropriately

**Physical Presence Integration**:
- [ ] Call PP processing in platform BDS before EndOfDxe
- [ ] Connect trusted consoles for user confirmation
- [ ] Support crypto-agile hash libraries
- [ ] Implement TPM hierarchy management

**Security Policy**:
- [ ] Check and process MOR variable in memory initialization
- [ ] Connect trusted storages before EndOfDxe
- [ ] Enable BlockSid by default
- [ ] Configure ACPI tables with proper PCDs

**Error Handling**:
- [ ] Register ReportStatusCode callback for TPM errors
- [ ] Implement platform-specific error responses
- [ ] Handle TPM startup failures in S3 resume
