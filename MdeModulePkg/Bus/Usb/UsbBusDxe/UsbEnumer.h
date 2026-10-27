/** @file

    USB bus enumeration interface.

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

//
// Advance the byte and bit to the next bit, adjust byte accordingly.
//
#define USB_NEXT_BIT(Byte, Bit)   \
          do {                \
            (Bit)++;          \
            if ((Bit) > 7) {  \
              (Byte)++;       \
              (Bit) = 0;      \
            }                 \
          } while (0)

//
// Common interface used by usb bus enumeration process.
// This interface is defined to mask the difference between
// the root hub and normal hub. So, bus enumeration code
// can be shared by both root hub and normal hub
//
typedef
EFI_STATUS
(*USB_HUB_INIT) (
  IN USB_INTERFACE  *UsbIf
  );

//
// Get the port status. This function is required to
// ACK the port change bits although it will return
// the port changes in PortState. Bus enumeration code
// doesn't need to ACK the port change bits.
//
typedef
EFI_STATUS
(*USB_HUB_GET_PORT_STATUS) (
  IN  USB_INTERFACE        *UsbIf,
  IN  UINT8                Port,
  OUT EFI_USB_PORT_STATUS  *PortState
  );

typedef
VOID
(*USB_HUB_CLEAR_PORT_CHANGE) (
  IN USB_INTERFACE  *HubIf,
  IN UINT8          Port
  );

typedef
EFI_STATUS
(*USB_HUB_SET_PORT_FEATURE) (
  IN USB_INTERFACE         *UsbIf,
  IN UINT8                 Port,
  IN EFI_USB_PORT_FEATURE  Feature
  );

typedef
EFI_STATUS
(*USB_HUB_CLEAR_PORT_FEATURE) (
  IN USB_INTERFACE         *UsbIf,
  IN UINT8                 Port,
  IN EFI_USB_PORT_FEATURE  Feature
  );

typedef
EFI_STATUS
(*USB_HUB_RESET_PORT) (
  IN USB_INTERFACE  *UsbIf,
  IN UINT8          Port
  );

typedef
EFI_STATUS
(*USB_HUB_RELEASE) (
  IN USB_INTERFACE  *UsbIf
  );

/**
  Return the endpoint descriptor in this interface.

  @param  UsbIf                 The interface to search in.
  @param  EpAddr                The address of the endpoint to return.

  @return The endpoint descriptor or NULL.

**/
USB_ENDPOINT_DESC *
UsbGetEndpointDesc (
  IN USB_INTERFACE  *UsbIf,
  IN UINT8          EpAddr
  );

/**
  Select an alternate setting for the interface.
  Each interface can have several mutually exclusive
  settings. Only one setting is active. It will
  also reset its endpoints' toggle to zero.

  @param  IfDesc                The interface descriptor to set.
  @param  Alternate             The alternate setting number to locate.

  @retval EFI_NOT_FOUND         There is no setting with this alternate index.
  @retval EFI_SUCCESS           The interface is set to Alternate setting.

**/
EFI_STATUS
UsbSelectSetting (
  IN USB_INTERFACE_DESC  *IfDesc,
  IN UINT8               Alternate
  );

/**
  Select a new configuration for the device. Each
  device may support several configurations.

  @param  Device                The device to select configuration.
  @param  ConfigIndex           The index of the configuration ( != 0).
  @param  ConnectDrivers        Whether to connect function drivers immediately.

  @retval EFI_NOT_FOUND         There is no configuration with the index.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource.
  @retval EFI_SUCCESS           The configuration is selected.

**/
EFI_STATUS
UsbSelectConfig (
  IN USB_DEVICE  *Device,
  IN UINT8       ConfigIndex,
  IN BOOLEAN     ConnectDrivers
  );

/**
  Connect an interface while pinning its device across driver binding.

  @param UsbIf                  The interface to connect.

  @retval EFI_NOT_READY         Reset or recursive binding blocks connection.
  @retval Others                The result of connecting the interface.
**/
EFI_STATUS
UsbConnectDriver (
  IN USB_INTERFACE  *UsbIf
  );

/**
  Check that software-requested replacement will not stop an active consumer.

  @param Device                 The old device, whose objects remain installed.

  @retval EFI_SUCCESS           No driver ownership or asynchronous work remains.
  @retval Others                The device cannot yet be safely retired.
**/
EFI_STATUS
UsbDeviceIsQuiesced (
  IN USB_DEVICE  *Device
  );

/**
  Clean up disconnected interfaces and their retained descriptor trees.

  @param Device                 The device retaining old interface generations.
  @param DisconnectDrivers      Whether normal teardown may stop their drivers.

  @retval EFI_SUCCESS           All retired configurations were removed.
  @retval Others                Active consumers or teardown failure retained state.
**/
EFI_STATUS
UsbRemoveRetiredConfigs (
  IN USB_DEVICE  *Device,
  IN BOOLEAN     DisconnectDrivers
  );

/**
  Check that an interface's consumers have released it before retirement.

  @param UsbIf                  The interface to retire.
  @retval EFI_SUCCESS           No owner or asynchronous work remains.
  @retval Others                The interface cannot yet be safely retired.
**/
EFI_STATUS
UsbInterfaceIsQuiesced (
  IN USB_INTERFACE  *UsbIf
  );

/**
  Free an interface after its consumers and transfers have released it.

  @param UsbIf                  The interface to free.
  @retval EFI_SUCCESS           The interface was uninstalled and freed.
  @retval Others                The interface remains installed.
**/
EFI_STATUS
UsbFreeInterface (
  IN USB_INTERFACE  *UsbIf
  );

/**
  Remove the current device configuration.

  @param  Device                The USB device to remove configuration from.

  @retval EFI_SUCCESS           All interfaces were retired.
  @retval Others                Active work or teardown failure retained old state.

**/
EFI_STATUS
UsbRemoveConfig (
  IN USB_DEVICE  *Device
  );

/**
  Remove the device and all its children from the bus.

  @param  Device                The device to remove.

  @retval EFI_SUCCESS           The device is removed.
  @retval EFI_NOT_READY         A reset, binding, or asynchronous callback is active.

**/
EFI_STATUS
UsbRemoveDevice (
  IN USB_DEVICE  *Device
  );

/**
  Enumerate all the changed hub ports.

  @param  Event                 The event that is triggered.
  @param  Context               The context to the event.

  @return None.

**/
VOID
EFIAPI
UsbHubEnumeration (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Enumerate all the changed hub ports.

  @param  Event                 The event that is triggered.
  @param  Context               The context to the event.

  @return None.

**/
VOID
EFIAPI
UsbRootHubEnumeration (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );
