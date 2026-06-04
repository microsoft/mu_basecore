/** @file IoMmuLib.c

    This file contains all the IoMmu library functions.
    Wraps the IoMmu protocol functions to provide a generic interface for mapping host memory to device memory.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoMmuLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/IoMmu.h>

EFI_EVENT             mIoMmuEvent;
VOID                  *mIoMmuRegistration;
EDKII_IOMMU_PROTOCOL  *mIoMmuProtocol = NULL;

// TRUE only when the located IoMmu producer is at a revision that includes
// the SetAttributeById entry point and actually populates it.
BOOLEAN  mSetAttributeByIdSupported = FALSE;

/**
  Returns True if the IoMmu protocol is available, otherwise returns False.

  @retval BOOLEAN    TRUE if the IoMmu protocol is available, FALSE otherwise.
**/
BOOLEAN
EFIAPI
IoMmuIsPresent (
  VOID
  )
{
  return (mIoMmuProtocol != NULL);
}

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
  )
{
  if (mIoMmuProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: IoMmuProtocol is NULL\n", __func__));
    ASSERT (mIoMmuProtocol != NULL);
    return EFI_NOT_READY;
  }

  return mIoMmuProtocol->Map (mIoMmuProtocol, Operation, HostAddress, NumberOfBytes, DeviceAddress, Mapping);
}

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
  )
{
  if (mIoMmuProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: IoMmuProtocol is NULL\n", __func__));
    ASSERT (mIoMmuProtocol != NULL);
    return EFI_NOT_READY;
  }

  return mIoMmuProtocol->Unmap (mIoMmuProtocol, Mapping);
}

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
  )
{
  if (mIoMmuProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: IoMmuProtocol is NULL\n", __func__));
    ASSERT (mIoMmuProtocol != NULL);
    return EFI_NOT_READY;
  }

  return mIoMmuProtocol->FreeBuffer (mIoMmuProtocol, Pages, HostAddress);
}

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
  )
{
  if (mIoMmuProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: IoMmuProtocol is NULL\n", __func__));
    ASSERT (mIoMmuProtocol != NULL);
    return EFI_NOT_READY;
  }

  return mIoMmuProtocol->AllocateBuffer (mIoMmuProtocol, Type, MemoryType, Pages, HostAddress, Attributes);
}

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
  )
{
  if (mIoMmuProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: IoMmuProtocol is NULL\n", __func__));
    ASSERT (mIoMmuProtocol != NULL);
    return EFI_NOT_READY;
  }

  return mIoMmuProtocol->SetAttribute (mIoMmuProtocol, DeviceHandle, Mapping, IoMmuAccess);
}

/**
  Set the R/W access attributes for Mapping in the Page Table for a caller
  that explicitly specifies (IommuBase, DmaId) instead of an EFI_HANDLE.

  @param [in]  IommuBase     Base MMIO address of the IOMMU that owns DmaId.
  @param [in]  DmaId         DMA identifier emitted by the calling DMA agent (e.g. StreamID on Arm SMMU, RequesterID on VT-d).
  @param [in]  Mapping       The mapping to set attributes for. Returned from IoMmuMap.
  @param [in]  IoMmuAccess   The IOMMU access attributes.

  @retval EFI_SUCCESS        Success
  @retval EFI_NOT_READY      The IoMmu protocol is not ready.
  @retval Other              Other errors as defined by the IoMmu protocol.
**/
EFI_STATUS
EFIAPI
IoMmuSetAttributeById (
  IN UINT64  IommuBase,
  IN UINT32  DmaId,
  IN VOID    *Mapping,
  IN UINT64  IoMmuAccess
  )
{
  if (mIoMmuProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: IoMmuProtocol is NULL\n", __func__));
    ASSERT (mIoMmuProtocol != NULL);
    return EFI_NOT_READY;
  }

  if (!mSetAttributeByIdSupported) {
    DEBUG ((DEBUG_WARN, "%a: SetAttributeById not implemented by IoMmu producer.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  return mIoMmuProtocol->SetAttributeById (mIoMmuProtocol, IommuBase, DmaId, Mapping, IoMmuAccess);
}

/**
  Event notification that is fired when IOMMU protocol is installed.

  @param [in] Event               The Event that is being processed.
  @param [in] Context             Event Context.

**/
VOID
IoMmuProtocolCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEdkiiIoMmuProtocolGuid, NULL, (VOID **)&mIoMmuProtocol);
  if (!EFI_ERROR (Status)) {
    mSetAttributeByIdSupported = ((mIoMmuProtocol != NULL) &&
                                  (mIoMmuProtocol->Revision >= EDKII_IOMMU_PROTOCOL_REVISION) &&
                                  (mIoMmuProtocol->SetAttributeById != NULL)
                                  );
    gBS->CloseEvent (mIoMmuEvent);
  }
}

/**
 * IoMmuLibInit Constructor
 * Locates the IoMmu protocol and initializes the library.
 * If the IoMmu protocol is not found, it sets up a notification event to be called when the protocol is installed.
 *
 * @retval EFI_SUCCESS           A protocol instance matching Protocol was found and returned in
 *                               Interface.
 * @retval EFI_NOT_FOUND         No protocol instances were found that match Protocol and
 *                               Registration.
 * @retval EFI_INVALID_PARAMETER Interface is NULL.
 *                               Protocol is NULL.
 **/
EFI_STATUS
EFIAPI
IoMmuLibInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEdkiiIoMmuProtocolGuid, NULL, (VOID **)&mIoMmuProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: IoMmuProtocol not found, setting IoMmuProtocolCallback\n", __func__));

    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    (EFI_TPL)TPL_CALLBACK,
                    (EFI_EVENT_NOTIFY)IoMmuProtocolCallback,
                    NULL,
                    &mIoMmuEvent
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to create IoMmuProtocolCallback event\n", __func__));
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // Register for gEdkiiIoMmuProtocolGuid notifications on this event
    Status = gBS->RegisterProtocolNotify (
                    &gEdkiiIoMmuProtocolGuid,
                    mIoMmuEvent,
                    &mIoMmuRegistration
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to register IoMmuProtocol notify\n", __func__));
      ASSERT_EFI_ERROR (Status);
      gBS->CloseEvent (mIoMmuEvent);
    }

    return EFI_SUCCESS;
  }

  // The library remains fully functional with older IoMmu revisions.
  // If revision < EDKII_IOMMU_PROTOCOL_REVISION, then
  // IoMmuSetAttributeById will return EFI_UNSUPPORTED
  if ((mIoMmuProtocol != NULL) && (mIoMmuProtocol->Revision >= EDKII_IOMMU_PROTOCOL_REVISION) && (mIoMmuProtocol->SetAttributeById != NULL)) {
    mSetAttributeByIdSupported = TRUE;
  } else {
    DEBUG ((
      DEBUG_WARN,
      "%a: IoMmuProtocol revision 0x%llx does not support SetAttributeById. IoMmuSetAttributeById will return EFI_UNSUPPORTED.\n",
      __func__,
      mIoMmuProtocol->Revision
      ));
  }

  return Status;
}
