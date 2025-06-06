# SMMU/IOMMU Driver

This document describes the System Memory Management Unit (SMMU) driver implementation, and how it integrates with the
PCI I/O subsystem. The driver configures the SMMUv3 hardware and implements the IOMMU protocol to provide address
translation and memory protection for DMA operations.

## Architecture Overview

The system consists of three main components working together:

1. **PCI I/O Protocol**: Provides interface for PCI device access and DMA operations
2. **IOMMU Protocol**: Implements DMA remapping and memory protection
3. **SMMU Hardware Driver**: Configures and manages the SMMU hardware

### Component Integration Flow

```text
PCI Device Driver
      ↓
PCI I/O Protocol
      ↓
IOMMU Protocol
      ↓
SMMU Hardware
```

## IOMMU Protocol Integration

1. **PCI Driver Initiates DMA**:
   - PCI device driver calls PciIo->Map(), PciIo->Unmap()
   - Provides host memory address and operation type

2. **IOMMU Protocol Setup**:
   - Implements the IOMMU protocol:

     ```c
     struct _EDKII_IOMMU_PROTOCOL {
       UINT64                         Revision;
       EDKII_IOMMU_SET_ATTRIBUTE      SetAttribute;
       EDKII_IOMMU_MAP                Map;
       EDKII_IOMMU_UNMAP              Unmap;
       EDKII_IOMMU_ALLOCATE_BUFFER    AllocateBuffer;
       EDKII_IOMMU_FREE_BUFFER        FreeBuffer;
     };
     ```

   - Configures IOMMU page tables with PageTableInit()

### DMA Mapping

- Maintains a 4-level page table to map HostAddress and DeviceAddress
- Identity Mapped

1. **IoMmu Map**:

   ```c
   EFI_STATUS
   EFIAPI
   IoMmuMap (
     IN     EDKII_IOMMU_PROTOCOL   *This,
     IN     EDKII_IOMMU_OPERATION  Operation,
     IN     VOID                   *HostAddress,
     IN OUT UINTN                  *NumberOfBytes,
     OUT    EFI_PHYSICAL_ADDRESS   *DeviceAddress,
     OUT    VOID                   **Mapping
     );
   ```

- Sets access permissions based on operation type:
  - BusMasterRead: READ access
  - BusMasterWrite: WRITE access
  - BusMasterCommonBuffer: READ/WRITE access

- Maps HostAddress to DeviceAddress
- Validates operation type
- Called by PciIo protocol for mapping

### DMA Unmapping

1. **PCI Driver Completes DMA**:
   - Calls PciIo->Unmap()
   - Provides mapping handle

2. **IoMmu Unmap**:

   ```c
   EFI_STATUS
   EFIAPI
   IoMmuUnmap (
     IN  EDKII_IOMMU_PROTOCOL  *This,
     IN  VOID                  *Mapping
     );
   ```

   - Invalidates mapping in Page Table
   - Invalidates TLB entries

## SMMU Configuration

### 1. SMMUv3 Hardware Setup

The SMMU is configured in stage 2 translation mode with:

- Stream table for device ID mapping
- Command queue for SMMU operations, like TLB management
- Event queue for error handling
- 4KB translation granule

### 2. Page Table Structure

The IOMMU uses a 4-level page table structure for DMA address translation:
<https://developer.arm.com/documentation/101811/0104/Translation-granule/The-starting-level-of-address-translation>

```text
Level 0 Table (L0)
    ↓
Level 1 Table (L1)
    ↓
Level 2 Table (L2)
    ↓
Level 3 Table (L3)
    ↓
Physical Page
```

### 3. Address Translation Process

1. **Device Issues DMA**:
   - Device uses IOVA (I/O Virtual Address)
   - SMMU intercepts access

2. **SMMU Translation**:
   - Looks up Stream Table Entry (STE)
      - 2 level or Linear Stream Table. Depending on configurable maximum StreamId via IORT.
   - Walks up to 4-level page tables
   - Converts IOVA to PA (Physical Address)

## Memory Protection

The IOMMU protocol provides several protection mechanisms:

1. **Access Control**:
   - Read/Write permissions per mapping
   - Device isolation through Stream IDs

2. **Address Range Protection**:
   - Validates DMA addresses
   - Prevents access outside mapped regions

3. **Error Handling**:
   - Translation faults logged to Event Queue

## Continuous Protection Through the OS

### SMMU Initialization Pre-DXE

To ensure continuous protection, the SMMU should be set in abort mode during the pre-DXE phase. This means that the SMMU
will block all transactions unless explicitly configured to allow them. This setup ensures that no unauthorized DMA
operations can occur before the SMMU is fully configured.

### Interaction with PcdDisableBMEonEBS

The PcdDisableBMEonEBS (Disable Bus Master Enable on Exit Boot Services) setting plays a crucial role in maintaining
system security during the transition from firmware to the operating system.

The following steps outline the end-to-end interaction:

1. Pre-DXE Initialization:

   The SMMU is set to abort mode to block all transactions. This ensures that no DMA operations can bypass the SMMU's
   protection mechanisms.

2. DMA Remapping with SMMU:

   If DMA remapping is required, the SMMU is configured to translate addresses and manage memory protection.
   The SMMU's page tables and stream tables are set up to allow only authorized DMA operations.

3. Exit Boot Services (EBS):

   On exit boot services, the SMMU is set to global bypass mode along with disabling the Bus Master Enable (BME) bit.
   The PcdDisableBMEonEBS PCD is used to disable the Bus Master Enable (BME) bit.
   This prevents any PCI devices from initiating DMA operations until the operating system reconfigures the SMMU.

4. OS Reconfiguration:

   The operating system reconfigures the SMMU as needed for its own DMA remapping and memory protection requirements.
   By following these steps, the system ensures that the SMMU provides continuous protection from the pre-DXE phase
   through to the operating system's reconfiguration. This approach helps prevent unauthorized DMA operations and
   maintains system security.

## Performance Features

The implementation includes optimizations for:

1. **Integration of SmmuV3 with IOMMU Protocol**

2. **TLB Management**:
   - TLB invalidation for unmapped entries via the command queue

## Configuration Options

Key SMMU settings controlled through the SMMU config HOB:

```text
- Smmu base address, num ID's, etc.
- Stream Table Size: Based on num IDs
- IORT info
```

## Limitations

Current implementation constraints:

1. Fixed 4KB granule size
2. 48-bit address space limit
3. Stage 2 translation only
   - Stage 2 is used on its own to simplify the translation process.
4. Identity mapped page tables

## Future Enhancements

Potential improvements:

1. Multiple translation granule support
2. Stage 1 & 2 translation
3. Different page table mapping schemes
4. Updated IoMmu Protocol to optimize redundencies

## Relevant Docs

- SMMUv3 specification <https://developer.arm.com/documentation/ihi0070/latest/>
- Useful ARM SMMU documentation - <https://developer.arm.com/documentation/109242/0100/Programming-the-SMMU>
- Arm AArch64 memory manegemnt guide - <https://developer.arm.com/documentation/101811/0104>
- ARM a_a-profile_architecture_reference_manual <https://developer.arm.com/documentation/102105/ka-07>
- Intel IOMMU for DMA protection in UEFI <https://www.intel.com/content/dam/develop/external/us/en/documents/intel-whitepaper-using-iommu-for-dma-protection-in-uefi.pdf>
- IORT documentation <https://developer.arm.com/documentation/den0049/latest/>

## Platform Integration Instructions

Generic Platform Integration:

- The Platform will construct a SMMU config HOB and publish for SmmuDxe to consume:
- Append the IORT structure to this struct and update the fields accordingly.

  ```c
   // SMMU_CONFIG structure to pass the SMMU configuration data from the platform to the SMMU driver.
   // Platform will configure SmmuDisabledList size and offset to the SMMU disabled list appropriatley
   // for any SMMU that needs be disabled in UEFI and set to bypass.
   typedef struct _SMMU_CONFIG {
      UINT32    VersionMajor;
      UINT32    VersionMinor;
      UINT32    SmmuDisabledListSize;   // Size of SmmuDisabledList in bytes.
      UINT32    SmmuDisabledListOffset; // Offset in bytes to the SmmuDisabledList from the start of the HOB structure.
      UINT32    IortSize;
      UINT32    IortOffset;             // Offset in bytes to the IORT table from the start of the HOB structure.
   } SMMU_CONFIG;
  ```

- Essentialy the same as IORT we want to publish
- The SMMU expects the entire IORT data to be passed into a HOB gSmmuConfigHobGuid.
- The platform must create the IORT structure and create gSmmuConfigHobGuid with that data using BuildGuidDataHob.
- If the platform needs to disable/bypass any Smmu, they can add the SMMU base address to the SmmuDisabledList in the HOB.
- This structure is consumed by SmmuDxe to configure the SMMU hardware

Integration with Qemu:

- SMMU is supported on Qemu but on v9.1.50+ <https://gitlab.com/qemu-project/qemu>
