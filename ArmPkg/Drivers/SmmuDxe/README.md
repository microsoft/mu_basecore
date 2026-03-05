# SmmuDxe Driver

This document describes the System Memory Management Unit (SMMU) driver implementation, and how it integrates with the
PCI I/O subsystem. The driver configures the SMMUv3 hardware and implements the IOMMU protocol to provide address
translation and memory protection for DMA operations.

## Architecture Overview

The SmmuDxe driver will consume the SMMU_CONFIG HOB with the IORT data to configure the SMMU's found on the platform.
It will set them up for Stage 2 Translation by default. SmmuDxe will install the IoMmu Protocol.
Translation table mapping can be done by leveraging the IoMmu Protocol. The protocol functions are outlined below.
Seperatley, an IoMmuLib is provided for platforms to use to do DMA mappings for the SMMU.
SmmuDxe will install the IORT ACPI Table. Platform should not install the IORT, but instead pass in the IORT data
with the SMMU_CONFIG HOB.

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

   - SmmuDxe will handle Translation Table initialization

## DMA Mapping with IoMmuLib and IoMmu Protocol

- Maintains up to a 4-level page table, depending on configuration, to map HostAddress and DeviceAddress
- Identity Mapped

1. **IoMmu Map**:

   ```c
   EFI_STATUS
   EFIAPI
   IoMmuMap (
     IN     EDKII_IOMMU_OPERATION  Operation,
     IN     VOID                   *HostAddress,
     IN OUT UINTN                  *NumberOfBytes,
     OUT    EFI_PHYSICAL_ADDRESS   *DeviceAddress,
     OUT    VOID                   **Mapping
     );
   ```

- Maps HostAddress to DeviceAddress
- Validates operation type
- Called by PciIo protocol for mapping

### Bounce Buffering

In certain conditions, `IoMmuMap` will allocate a bounce buffer instead of using the original host address directly.
A bounce buffer is a temporary intermediate buffer allocated that is used when the original DMA buffer
cannot be used directly by the device.

**Bounce Buffer Conditions:**

A bounce buffer is allocated when the operation is NOT `EdkiiIoMmuOperationBusMasterCommonBuffer` or
`EdkiiIoMmuOperationBusMasterCommonBuffer64`, AND any of the following conditions are met:

1. **Alignment Requirements Not Met:**
   - The `NumberOfBytes` is not 4KB aligned, OR
   - The `HostAddress` (PhysicalAddress) is not 4KB aligned

2. **32-bit DMA Limitation:**
   - The operation is a 32-bit DMA operation (`EdkiiIoMmuOperationBusMasterRead` or
     `EdkiiIoMmuOperationBusMasterWrite`), AND
   - Any part of the DMA transfer range (`PhysicalAddress + NumberOfBytes`) exceeds 4GB

**Bounce Buffer Behavior:**

- When a bounce buffer is needed due to **alignment issues only** (with 64-bit operations), memory can be allocated
  anywhere in the address space
- When a bounce buffer is needed due to the **32-bit DMA limitation**, memory is allocated below 4GB using
  `AllocateMaxAddress` with `DmaMemoryTop` set to `SIZE_4GB - 1`
- The `DeviceAddress` returned points to the bounce buffer, not the original host address
- The `Mapping` handle stores information about both the original host address and the bounce buffer address

**CopyMem on Map (Host → Bounce Buffer):**

A `CopyMem` from the host buffer to the bounce buffer is performed during `IoMmuMap` when:

- A bounce buffer was allocated (NeedRemap is TRUE), AND
- The operation is a **read operation** from the Bus Master's perspective:
  - `EdkiiIoMmuOperationBusMasterRead`, OR
  - `EdkiiIoMmuOperationBusMasterRead64`

This copy ensures the Bus Master can read the correct data from the bounce buffer during the DMA operation.
For write operations, no copy is needed on Map since the Bus Master will write new data into the bounce buffer.

## DMA Unmapping with IoMmuLib and IoMmu Protocol

1. **PCI Driver Completes DMA**:
   - Calls PciIo->Unmap()
   - Provides mapping handle

2. **IoMmu Unmap**:

   ```c
   EFI_STATUS
   EFIAPI
   IoMmuUnmap (
     IN  VOID                  *Mapping
     );
   ```

   - Invalidates mapping in Page Table
   - Invalidates TLB entries

### Bounce Buffer Handling on Unmap

When unmapping a DMA operation that used a bounce buffer (i.e., `DeviceAddress != HostAddress`):

**CopyMem on Unmap (Bounce Buffer → Host):**

A `CopyMem` from the bounce buffer back to the host buffer is performed during `IoMmuUnmap` when:

- A bounce buffer was used (`DeviceAddress != HostAddress`), AND
- The operation is a **write operation** from the Bus Master's perspective:
  - `EdkiiIoMmuOperationBusMasterWrite`, OR
  - `EdkiiIoMmuOperationBusMasterWrite64`

This copy ensures the processor can access the data that the Bus Master wrote into the bounce buffer.
For read operations, no copy is needed on Unmap since the Bus Master only read data and did not modify it.

**Cleanup:**

- The bounce buffer pages are freed using `FreePages`
- The mapping information structure is freed

For direct mappings (no bounce buffer), only the mapping information structure is freed.

### DMA Access Attributes with IoMmuLib and IoMmu Protocol

1. Setting R/W permissions
   - After mapping an address with IoMmuMap()
   - Clearning R/W permissions before unmmapping an address with IoMmuUnmap()
   - Sets access permissions based on IoMmuAccess type:
      - EDKII_IOMMU_ACCESS_READ: READ only access
      - EDKII_IOMMU_ACCESS_WRITE: WRITE only access
      - EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE: READ/WRITE access

2. IoMmu SetAttribute

   ```c
   EFI_STATUS
   EFIAPI
   IoMmuSetAttribute (
      IN EFI_HANDLE            DeviceHandle,
      IN VOID                  *Mapping,
      IN UINT64                IoMmuAccess
   );
   ```

## SMMU Configuration

### 1. SMMUv3 Hardware Setup

The SMMU is configured in stage 2 translation mode with:

- Stream table for device ID mapping
- Command queue for SMMU operations, like TLB management
- Event queue for error handling
- 4KB translation granule

### 2. Page Table Structure

The IOMMU uses up to a 4-level page table structure for DMA address translation:
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

Depending on configuration from the SMMU registers, the starting level of translation is chosen.
Depending on the SMMU configuration found, also supports Concatenated Translation Tables for the
Translation Table Base.

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
   - TLB invalidation by VA for unmapped entries via the command queue

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
4. Updated IoMmu Protocol to optimize redundancies
5. Bounce Buffer Optimization
   - Remove the `NumberOfBytes` end address alignment check in `IoMmuMap`
   - Update individual drivers to allocate their DMA buffers by page (page-aligned and page-sized)
   - This eliminates one case for bounce buffering, improving performance by avoiding unnecessary
     buffer copies and allocations when only the end address is unaligned

## Configuration Options

Key SMMU settings controlled through the SMMU config HOB:

- IORT data: The complete IORT table data that the SMMU(s) will be configured with.

- SmmuDisabledList: Provides platform the ability to individually disable/bypass an SMMU if needed.
This list is a set of SMMU base addresses that the platform wants to disable/bypass.
By default, all SMMU's found are configured for Stage 2 Translation, otherwise set in the SmmuDisabledList,
in which case translation for that SMMU is disabled and it is set to global bypass mode.

## Platform Integration Instructions

Generic Platform Integration:

- The Platform will construct a SMMU config HOB and publish for SmmuDxe to consume:
- Append the IORT structure to this struct and update the fields accordingly.
- Append the SmmuDisabledList as a UINT64 array. SmmuDxe will parse this Offset and
interpret as a `(UINT64*)` and iterate on that array of SMMU base addresses based on the SmmuDisabledSize.
SmmuDxe will derive the number of SMMU's in the SmmuDisabledListOffset with
`SmmuDisabledListSize / sizeof(UINT64)`

  ```c
   // SMMU_CONFIG structure to pass the SMMU configuration data from the platform to the SMMU driver.
   // Platform will pass in the IORT structure through here.
   // Platform will configure SmmuDisabledList size and offset to the SMMU disabled list appropriatley
   // with the base address for any SMMU that needs be disabled in UEFI and set to bypass.
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
- If the platform needs to disable/bypass any SMMU, they can add the SMMU base address to the SmmuDisabledList in the HOB.
- This structure is consumed by SmmuDxe to configure the SMMU hardware

Integration with Qemu:

- SMMU is supported on Qemu but on v9.1.50+ <https://gitlab.com/qemu-project/qemu>

## Relevant Docs

- SMMUv3 specification <https://developer.arm.com/documentation/ihi0070/latest/>
- Useful ARM SMMU documentation - <https://developer.arm.com/documentation/109242/0100/Programming-the-SMMU>
- Arm AArch64 memory manegemnt guide - <https://developer.arm.com/documentation/101811/0104>
- ARM a_a-profile_architecture_reference_manual <https://developer.arm.com/documentation/102105/ka-07>
- Intel IOMMU for DMA protection in UEFI <https://www.intel.com/content/dam/develop/external/us/en/documents/intel-whitepaper-using-iommu-for-dma-protection-in-uefi.pdf>
- IORT documentation <https://developer.arm.com/documentation/den0049/latest/>
