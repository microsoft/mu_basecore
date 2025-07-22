/** @file IoMmuLib.h

  The IoMmuLib library class is made available for drivers that depend upon IOMMU services and need to suspend
  dispatch until those services are available. This simplifies the process of ensuring that IOMMU services are ready
  in the driver.

  Some background on overall IOMMU usage is provided below for reference.

  A silicon or platform-specific driver typically produces gEdkiiIoMmuProtocolGuid. This driver may have dependencies
  to do so. For example, IntelVtdDxe produces gEdkiiIoMmuProtocolGuid after parsing the DMAR ACPI table which it
  acquires via a protocol notification on gEfiAcpi10TableGuid and gEfiAcpi20TableGuid. The driver may also need PCI
  root bridge I/O to determine how to map its IOMMU engines to present PCI devices. It does so with the
  gEfiPciRootBridgeIoProtocolGuid protocol, produced by PciHostBridgeDxe. gEfiPciRootBridgeIoProtocolGuid also
  provides four functions that make use of the IOMMU protocol:

    - RootBridgeIoMap()
    - RootBridgeIoUnmap()
    - RootBridgeIoAllocateBuffer()
    - RootBridgeIoFreeBuffer()

  To avoid a circular dispatch dependency, PciHostBridgeDxe does not have gEdkiiIoMmuProtocolGuid in its DEPEX.
  However, a window of time exists where the gEfiPciRootBridgeIoProtocolGuid is produced, but the
  gEdkiiIoMmuProtocolGuid is not yet produced. This is referred to as the "IOMMU blackout window". During this time,
  the PciHostBridgeDxe driver needs to understand how to react when operations on gEfiPciRootBridgeIoProtocolGuid
  are called that may require an IOMMU. It understands how to do this for a given platform via the
  PcdRequireIommu feature PCD. If this PCD is TRUE, the driver will not perform any IOMMU operations until the
  gEdkiiIoMmuProtocolGuid is produced and return EFI_NOT_READY during the IOMMU blackout window. If the PCD is FALSE
  (default), the driver will not use the IOMMU protocol and will not return EFI_NOT_READY.

  The same IOMMU blackout window exists for the gEfiPciIoProtocolGuid, which is produced by PciBusDxe.

  Example of the blackout window on an Intel IOMMU enabled platforrm:

  PciHostBridgeDxe -> PciBusDxe -> IntelVtdDxe --> Anyone else can use IOMMU
  [ IOMMU BLACKOUT WINDOW                    ]     (IoMmuLib)

  Most drivers that need to perform IOMMU operations should use the non-NULL IoMmuLib instance. This library
  provides a set of functions that abstract underlying IOMMU dependency details and allow drivers to perform
  IOMMU operations when services are available. A NULL instance is provided for IoMmuLib that ignores IOMMU
  operations when that is desired.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef IOMMU_LIB_H_
#define IOMMU_LIB_H_

#include <Uefi.h>
#include <Protocol/IoMmu.h>

/**
  Map a host address to a device address using the Page Table.
  Currently, this function only supports identity mapping.

  @param [in]      Operation       The type of IOMMU operation.
  @param [in]      HostAddress     The host address to map.
  @param [in, out] NumberOfBytes   On input, the number of bytes to map. On output, the number of bytes mapped.
  @param [out]     DeviceAddress   The resulting device address.
  @param [out]     Mapping         A handle to the mapping. Used by IoMmuUnmap to unmap the address and IoMmuSetAttribute to set attributes.
                                   IoMmuMap allocates this memory, and it is be freed by IoMmuUnmap.

  @retval EFI_SUCCESS              Success.
  @retval EFI_NOT_READY            The IoMmu protocol is not ready.
  @retval Other                    Other errors as defined by the IoMmu protocol.

**/
EFI_STATUS
EFIAPI
IoMmuMap (
  IN     EDKII_IOMMU_OPERATION  Operation,
  IN     VOID                   *HostAddress,
  IN OUT UINTN                  *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID                   **Mapping
  );

/**
  Unmap a device address in the Page Table, also invaldidates the TLB by VA.

  @param [in]  Mapping   The mapping to unmap. This is the mapping that is returned from IoMmuMap.

  @retval EFI_SUCCESS            Success.
  @retval EFI_NOT_READY          The IoMmu protocol is not ready.
  @retval Other                  Other errors as defined by the IoMmu protocol.

**/
EFI_STATUS
EFIAPI
IoMmuUnmap (
  IN  VOID  *Mapping
  );

/**
  Free a buffer allocated by IoMmuAllocateBuffer.

  @param [in]  Pages         The number of pages to free.
  @param [in]  HostAddress   The host address to free.

  @retval EFI_SUCCESS            Success.
  @retval EFI_NOT_READY          The IoMmu protocol is not ready.
  @retval Other                  Other errors as defined by the IoMmu protocol.
**/
EFI_STATUS
EFIAPI
IoMmuFreeBuffer (
  IN  UINTN  Pages,
  IN  VOID   *HostAddress
  );

/**
  Allocate a buffer for DMA use with the IoMmu.

  @param [in]      Type          The type of allocation to perform.
  @param [in]      MemoryType    The type of memory to allocate.
  @param [in]      Pages         The number of pages to allocate.
  @param [in, out] HostAddress   On input, the desired host address. On output, the allocated host address.
  @param [in]      Attributes    The memory attributes to use for the allocation.

  @retval EFI_SUCCESS           The requested pages were allocated.
  @retval EFI_NOT_READY         The IoMmu protocol is not ready.
  @retval Other                 Other errors as defined by the IoMmu protocol.

**/
EFI_STATUS
EFIAPI
IoMmuAllocateBuffer (
  IN     EFI_ALLOCATE_TYPE  Type,
  IN     EFI_MEMORY_TYPE    MemoryType,
  IN     UINTN              Pages,
  IN OUT VOID               **HostAddress,
  IN     UINT64             Attributes
  );

/**
  Set the R/W access attributes for Mapping in the Page Table.

  @param [in]  DeviceHandle  The device handle to set attributes for.
  @param [in]  Mapping       The mapping to set attributes for. This is the mapping that is returned from IoMmuMap.
  @param [in]  IoMmuAccess   The IOMMU access attributes.

  @retval EFI_SUCCESS            Success.
  @retval EFI_NOT_READY          The IoMmu protocol is not ready.
  @retval Other                  Other errors as defined by the IoMmu protocol.

**/
EFI_STATUS
EFIAPI
IoMmuSetAttribute (
  IN EFI_HANDLE  DeviceHandle,
  IN VOID        *Mapping,
  IN UINT64      IoMmuAccess
  );

#endif // IOMMU_LIB_H_
