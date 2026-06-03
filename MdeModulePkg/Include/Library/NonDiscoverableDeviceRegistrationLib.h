/** @file
  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __NON_DISCOVERABLE_DEVICE_REGISTRATION_LIB_H__
#define __NON_DISCOVERABLE_DEVICE_REGISTRATION_LIB_H__

#include <Protocol/NonDiscoverableDevice.h>

typedef enum {
  NonDiscoverableDeviceTypeAhci,
  NonDiscoverableDeviceTypeAmba,
  NonDiscoverableDeviceTypeEhci,
  NonDiscoverableDeviceTypeNvme,
  NonDiscoverableDeviceTypeOhci,
  NonDiscoverableDeviceTypeSdhci,
  NonDiscoverableDeviceTypeUfs,
  NonDiscoverableDeviceTypeUhci,
  NonDiscoverableDeviceTypeXhci,
  NonDiscoverableDeviceTypeMax,
} NON_DISCOVERABLE_DEVICE_TYPE;

// MU_CHANGE [BEGIN]: Add caller-supplied UniqueId parameter.

/**
  Register a non-discoverable MMIO device with a caller-supplied UniqueId.

  The platform must provide a UniqueId that NonDiscoverablePciDeviceDxe will
  use via EFI_PCI_IO_PROTOCOL.GetLocation(). Any value is accepted; the
  platform is responsible for keeping IDs unique across registrations.

  @param[in]      UniqueId            Platform-assigned UniqueId.
  @param[in]      Type                The type of non-discoverable device.
  @param[in]      DmaType             Whether the device is DMA coherent.
  @param[in]      InitFunc            Initialization routine to be invoked when
                                      the device is enabled.
  @param[in,out]  Handle              The handle onto which to install the
                                      non-discoverable device protocol.
                                      If Handle is NULL or *Handle is NULL, a
                                      new handle will be allocated.
  @param[in]      NumMmioResources    The number of UINTN base/size pairs that
                                      follow, each describing an MMIO region
                                      owned by the device.
  @param[in]  ...                     MmioResource base/size pairs.

  @retval EFI_SUCCESS                 The registration succeeded.
  @retval EFI_INVALID_PARAMETER       An invalid argument was given.
  @retval Other                       The registration failed.

**/
// MU_CHANGE [END]
EFI_STATUS
EFIAPI
RegisterNonDiscoverableMmioDevice (
  IN      UINTN                             UniqueId, // MU_CHANGE
  IN      NON_DISCOVERABLE_DEVICE_TYPE      Type,
  IN      NON_DISCOVERABLE_DEVICE_DMA_TYPE  DmaType,
  IN      NON_DISCOVERABLE_DEVICE_INIT      InitFunc,
  IN OUT  EFI_HANDLE                        *Handle OPTIONAL,
  IN      UINTN                             NumMmioResources,
  ...
  );

#endif
