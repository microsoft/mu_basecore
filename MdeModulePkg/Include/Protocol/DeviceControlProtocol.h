/** @file
  Defines protocol and associated types for boot loader device control in the
  UEFI environment.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DEVICE_CONTROL_PROTOCOL_H_
#define DEVICE_CONTROL_PROTOCOL_H_

#define DEVICE_CONTROL_PROTOCOL_GUID \
  { 0xf4c616df, 0xeb33, 0x4f34, { 0x8b, 0xd4, 0x5c, 0xb9, 0x9c, 0x80, 0x17, 0x89 } }

//
// Revision number of this protocol. Updates to the lower 32 bits of the
// revision number indicate backward-compatible (additive) changes.
//
#define DEVICE_CONTROL_PROTOCOL_REVISION  1

typedef struct _DEVICE_CONTROL_PROTOCOL DEVICE_CONTROL_PROTOCOL;

/**
  Relinquishes control of a PCI enumerable device for direct use by the
  boot loader. After this call returns successfully, the following
  conditions are guaranteed to be true:

    1. The device hardware is initialized and ready for use by the boot
       loader.

    2. All PciIo protocols associated with the device are available.

    3. There will be no further interaction with the device after this
       call from the firmware.

  @param[in] This      A pointer to the DEVICE_CONTROL_PROTOCOL.
  @param[in] Segment   The PCI segment number of the device to prepare.
  @param[in] Bus       The PCI bus number of the device to prepare.
  @param[in] Device    The PCI device number of the device to prepare.
  @param[in] Function  The PCI function number of the device to prepare.

  @retval EFI_SUCCESS            The device is prepared for direct control
                                 by the boot loader.
  @retval EFI_UNSUPPORTED        The provided device is not supported by
                                 this implementation.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_NOT_FOUND          The specified device could not be found.
  @retval EFI_DEVICE_ERROR       The device could not be prepared for
                                 direct control.
**/
typedef
EFI_STATUS
(EFIAPI *RELINQUISH_PCI_DEVICE_CONTROL)(
  IN DEVICE_CONTROL_PROTOCOL  *This,
  IN UINTN                    Segment,
  IN UINTN                    Bus,
  IN UINTN                    Device,
  IN UINTN                    Function
  );

struct _DEVICE_CONTROL_PROTOCOL {
  UINT64                           Revision;
  RELINQUISH_PCI_DEVICE_CONTROL    RelinquishPciDeviceControl;
};

extern EFI_GUID  gDeviceControlProtocolGuid;

#endif // DEVICE_CONTROL_PROTOCOL_H_
