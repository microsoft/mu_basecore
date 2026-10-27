/** @file

    Usb bus enumeration support.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbBus.h"

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
  )
{
  USB_ENDPOINT_DESC  *EpDesc;
  UINT8              Index;
  UINT8              NumEndpoints;

  NumEndpoints = UsbIf->IfSetting->Desc.NumEndpoints;

  for (Index = 0; Index < NumEndpoints; Index++) {
    EpDesc = UsbIf->IfSetting->Endpoints[Index];

    if (EpDesc->Desc.EndpointAddress == EpAddr) {
      return EpDesc;
    }
  }

  return NULL;
}

/**
  Free the resource used by USB interface.

  @param  UsbIf                 The USB interface to free.

  @retval EFI_ACCESS_DENIED     The interface is still occupied.
  @retval EFI_SUCCESS           The interface is freed.
**/
EFI_STATUS
UsbFreeInterface (
  IN USB_INTERFACE  *UsbIf
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  if (UsbIf->Device->AsyncCallbacks != 0) {
    return EFI_NOT_READY;
  }

  for (Index = 0; Index < ARRAY_SIZE (UsbIf->AsyncTransfers); Index++) {
    if (UsbIf->AsyncTransfers[Index] != NULL) {
      DEBUG ((DEBUG_ERROR, "UsbFreeInterface: asynchronous transfer is still queued\n"));
      return EFI_NOT_READY;
    }
  }

  UsbCloseHostProtoByChild (UsbIf->Device->Bus, UsbIf->Handle);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  UsbIf->Handle,
                  &gEfiDevicePathProtocolGuid,
                  UsbIf->DevicePath,
                  &gEfiUsbIoProtocolGuid,
                  &UsbIf->UsbIo,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    if (UsbIf->DevicePath != NULL) {
      FreePool (UsbIf->DevicePath);
    }

    FreePool (UsbIf);
  } else {
    UsbOpenHostProtoByChild (UsbIf->Device->Bus, UsbIf->Handle);
  }

  return Status;
}

/**
  Create an interface for the descriptor IfDesc. Each
  device's configuration can have several interfaces.

  @param  Device                The device has the interface descriptor.
  @param  IfDesc                The interface descriptor.

  @return The created USB interface for the descriptor, or NULL.

**/
USB_INTERFACE *
UsbCreateInterface (
  IN USB_DEVICE          *Device,
  IN USB_INTERFACE_DESC  *IfDesc
  )
{
  USB_DEVICE_PATH  UsbNode;
  USB_INTERFACE    *UsbIf;
  USB_INTERFACE    *HubIf;
  EFI_STATUS       Status;

  UsbIf = AllocateZeroPool (sizeof (USB_INTERFACE));

  if (UsbIf == NULL) {
    return NULL;
  }

  UsbIf->Signature = USB_INTERFACE_SIGNATURE;
  UsbIf->Device    = Device;
  UsbIf->IfDesc    = IfDesc;
  ASSERT (IfDesc->ActiveIndex < USB_MAX_INTERFACE_SETTING);
  UsbIf->IfSetting = IfDesc->Settings[IfDesc->ActiveIndex];

  CopyMem (
    &(UsbIf->UsbIo),
    &mUsbIoProtocol,
    sizeof (EFI_USB_IO_PROTOCOL)
    );

  //
  // Install protocols for USBIO and device path
  //
  UsbNode.Header.Type      = MESSAGING_DEVICE_PATH;
  UsbNode.Header.SubType   = MSG_USB_DP;
  UsbNode.ParentPortNumber = Device->ParentPort;
  UsbNode.InterfaceNumber  = UsbIf->IfSetting->Desc.InterfaceNumber;

  SetDevicePathNodeLength (&UsbNode.Header, sizeof (UsbNode));

  HubIf = Device->ParentIf;
  ASSERT (HubIf != NULL);

  UsbIf->DevicePath = AppendDevicePathNode (HubIf->DevicePath, &UsbNode.Header);

  if (UsbIf->DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "UsbCreateInterface: failed to create device path\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &UsbIf->Handle,
                  &gEfiDevicePathProtocolGuid,
                  UsbIf->DevicePath,
                  &gEfiUsbIoProtocolGuid,
                  &UsbIf->UsbIo,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbCreateInterface: failed to install UsbIo - %r\n", Status));
    goto ON_ERROR;
  }

  //
  // Open USB Host Controller Protocol by Child
  //
  Status = UsbOpenHostProtoByChild (Device->Bus, UsbIf->Handle);

  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           UsbIf->Handle,
           &gEfiDevicePathProtocolGuid,
           UsbIf->DevicePath,
           &gEfiUsbIoProtocolGuid,
           &UsbIf->UsbIo,
           NULL
           );

    DEBUG ((DEBUG_ERROR, "UsbCreateInterface: failed to open host for child - %r\n", Status));
    goto ON_ERROR;
  }

  DEBUG ((
    DEBUG_INFO,
    "UsbCreateInterface: published %04x:%04x interface %d at port %d@hub %d, handle %p\n",
    Device->DevDesc->Desc.IdVendor,
    Device->DevDesc->Desc.IdProduct,
    UsbIf->IfSetting->Desc.InterfaceNumber,
    Device->ParentPort,
    Device->ParentAddr,
    UsbIf->Handle
    ));

  return UsbIf;

ON_ERROR:
  if (UsbIf->DevicePath != NULL) {
    FreePool (UsbIf->DevicePath);
  }

  FreePool (UsbIf);
  return NULL;
}

/**
  Free the resource used by this USB device.

  @param  Device                The USB device to free.

**/
VOID
UsbFreeDevice (
  IN USB_DEVICE  *Device
  )
{
  ASSERT (Device->RetiredConfigs == NULL);

  if (Device->DevDesc != NULL) {
    UsbFreeDevDesc (Device->DevDesc);
  }

  gBS->FreePool (Device);
}

/**
  Create a device which is on the parent's ParentPort port.

  @param  ParentIf              The parent HUB interface.
  @param  ParentPort            The port on the HUB this device is connected to.

  @return Created USB device, Or NULL.

**/
USB_DEVICE *
UsbCreateDevice (
  IN USB_INTERFACE  *ParentIf,
  IN UINT8          ParentPort
  )
{
  USB_DEVICE  *Device;

  ASSERT (ParentIf != NULL);

  Device = AllocateZeroPool (sizeof (USB_DEVICE));

  if (Device == NULL) {
    return NULL;
  }

  Device->Bus        = ParentIf->Device->Bus;
  Device->MaxPacket0 = 8;
  Device->ParentAddr = ParentIf->Device->Address;
  Device->ParentIf   = ParentIf;
  Device->ParentPort = ParentPort;
  Device->Tier       = (UINT8)(ParentIf->Device->Tier + 1);
  Device->EnumScript = 0;
  Device->IsSSDev    = FALSE;
  // MU_CHANGE [BEGIN]
  Device->Connected = TRUE;
  DEBUG ((DEBUG_INFO, "UsbCreateDevice: ParentIf %p port %d Connected\n", ParentIf, ParentPort));
  // MU_CHANGE [END]
  return Device;
}

/**
  Connect the USB interface with its driver. EFI USB bus will
  create a USB interface for each separate interface descriptor.

  @param  UsbIf             The interface to connect driver to.

  @return EFI_SUCCESS       Interface is managed by some driver.
  @return Others            Failed to locate a driver for this interface.

**/
EFI_STATUS
UsbConnectDriver (
  IN USB_INTERFACE  *UsbIf
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  if (UsbIf->Device->ResetInProgress || (UsbIf->Device->BindingInProgress != 0)) {
    DEBUG ((DEBUG_INFO, "UsbConnectDriver: reset or binding is in progress\n"));
    return EFI_NOT_READY;
  }

  if (!UsbIf->Device->Connected || UsbIf->Disconnected) {
    DEBUG ((DEBUG_ERROR, "UsbConnectDriver: device %d is offline\n", UsbIf->Device->Address));
    return EFI_DEVICE_ERROR;
  }

  UsbIf->Device->BindingInProgress++;
  Status = EFI_SUCCESS;

  //
  // Hub is maintained by the USB bus driver. Otherwise try to
  // connect drivers with this interface
  //
  if (UsbIsHubInterface (UsbIf)) {
    DEBUG ((DEBUG_INFO, "UsbConnectDriver: found a hub device\n"));
    Status = mUsbHubApi.Init (UsbIf);
  } else {
    //
    // This function is called in both UsbIoControlTransfer and
    // the timer callback in hub enumeration. So, at least it is
    // called at TPL_CALLBACK. Some driver sitting on USB has
    // twisted TPL used. It should be no problem for us to connect
    // or disconnect at CALLBACK.
    //

    //
    // Only recursively wanted usb child device
    //
    if (UsbBusIsWantedUsbIO (UsbIf->Device->Bus, UsbIf)) {
      OldTpl = UsbGetCurrentTpl ();
      DEBUG ((DEBUG_INFO, "UsbConnectDriver: TPL before connect is %d, %p\n", (UINT32)OldTpl, UsbIf->Handle));

      if (OldTpl > TPL_CALLBACK) {
        gBS->RestoreTPL (TPL_CALLBACK);
      }

      Status           = gBS->ConnectController (UsbIf->Handle, NULL, NULL, TRUE);
      UsbIf->IsManaged = (BOOLEAN) !EFI_ERROR (Status);

      DEBUG ((DEBUG_INFO, "UsbConnectDriver: TPL after connect is %d\n", (UINT32)UsbGetCurrentTpl ()));
      ASSERT (UsbGetCurrentTpl () == ((OldTpl > TPL_CALLBACK) ? TPL_CALLBACK : OldTpl));

      if (OldTpl > TPL_CALLBACK) {
        gBS->RaiseTPL (OldTpl);
      }
    }
  }

  UsbIf->Device->BindingInProgress--;
  return Status;
}

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
  )
{
  USB_INTERFACE_SETTING  *Setting;
  UINTN                  Index;

  //
  // Locate the active alternate setting
  //
  Setting = NULL;

  for (Index = 0; Index < IfDesc->NumOfSetting; Index++) {
    ASSERT (Index < USB_MAX_INTERFACE_SETTING);
    Setting = IfDesc->Settings[Index];

    if (Setting->Desc.AlternateSetting == Alternate) {
      break;
    }
  }

  if ((Index == IfDesc->NumOfSetting) || (Setting == NULL)) {
    return EFI_NOT_FOUND;
  }

  IfDesc->ActiveIndex = Index;

  ASSERT (Setting != NULL);
  DEBUG ((
    DEBUG_INFO,
    "UsbSelectSetting: setting %d selected for interface %d\n",
    Alternate,
    Setting->Desc.InterfaceNumber
    ));

  //
  // Reset the endpoint toggle to zero
  //
  for (Index = 0; Index < Setting->Desc.NumEndpoints; Index++) {
    Setting->Endpoints[Index]->Toggle = 0;
  }

  return EFI_SUCCESS;
}

/**
  Select a new configuration for the device. Each
  device may support several configurations.

  @param  Device                The device to select configuration.
  @param  ConfigValue           The index of the configuration ( != 0).
  @param  ConnectDrivers        Whether to connect function drivers immediately.

  @retval EFI_NOT_FOUND         There is no configuration with the index.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource.
  @retval EFI_SUCCESS           The configuration is selected.

**/
EFI_STATUS
UsbSelectConfig (
  IN USB_DEVICE  *Device,
  IN UINT8       ConfigValue,
  IN BOOLEAN     ConnectDrivers
  )
{
  USB_DEVICE_DESC     *DevDesc;
  USB_CONFIG_DESC     *ConfigDesc;
  USB_INTERFACE_DESC  *IfDesc;
  USB_INTERFACE       *UsbIf;
  EFI_STATUS          Status;
  UINT8               Index;

  //
  // Locate the active config, then set the device's pointer
  //
  DevDesc    = Device->DevDesc;
  ConfigDesc = NULL;

  for (Index = 0; Index < DevDesc->Desc.NumConfigurations; Index++) {
    ConfigDesc = DevDesc->Configs[Index];

    if (ConfigDesc->Desc.ConfigurationValue == ConfigValue) {
      break;
    }
  }

  if ((Index == DevDesc->Desc.NumConfigurations) || (ConfigDesc == NULL)) {
    return EFI_NOT_FOUND;
  }

  if (ConfigDesc->Desc.NumInterfaces > USB_MAX_INTERFACE) {
    DEBUG ((DEBUG_ERROR, "UsbSelectConfig: too many interfaces\n"));
    return EFI_UNSUPPORTED;
  }

  Device->ActiveConfig = ConfigDesc;

  DEBUG ((
    DEBUG_INFO,
    "UsbSelectConfig: config %d selected for device %d\n",
    ConfigValue,
    Device->Address
    ));

  //
  // Create interfaces for each USB interface descriptor.
  //
  for (Index = 0; Index < Device->ActiveConfig->Desc.NumInterfaces; Index++) {
    //
    // A driver's Start() may reset the device and replace its descriptors.
    //
    ConfigDesc = Device->ActiveConfig;

    //
    // First select the default interface setting, and reset
    // the endpoint toggles to zero for its endpoints.
    //
    IfDesc = ConfigDesc->Interfaces[Index];
    Status = UsbSelectSetting (IfDesc, 0);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UsbSelectConfig: no default setting for interface %d - %r\n", Index, Status));
      return Status;
    }

    //
    // Create a USB_INTERFACE and install USB_IO and other protocols
    //
    UsbIf = Device->Interfaces[Index];
    if (UsbIf == NULL) {
      UsbIf = UsbCreateInterface (Device, ConfigDesc->Interfaces[Index]);
      if (UsbIf == NULL) {
        if (ConnectDrivers) {
          Device->NumOfInterface = Index;
        }

        return EFI_OUT_OF_RESOURCES;
      }

      Device->Interfaces[Index] = UsbIf;
    }

    ASSERT (Index < USB_MAX_INTERFACE);
    if (ConnectDrivers) {
      Device->NumOfInterface = (UINT8)(Index + 1);
    }

    if (!ConnectDrivers) {
      continue;
    }

    //
    // Connect the device to drivers, if it failed, ignore
    // the error. Don't let the unsupported interfaces to block
    // the supported interfaces.
    //
    Status = UsbConnectDriver (UsbIf);

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "UsbSelectConfig: failed to connect driver - %r, ignored\n",
        Status
        ));
    }

    if (!Device->Connected) {
      DEBUG ((DEBUG_ERROR, "UsbSelectConfig: device went offline during driver binding\n"));
      return EFI_DEVICE_ERROR;
    }

    if (Device->ActiveConfig != ConfigDesc) {
      //
      // Reset already published the new configuration without driver binding.
      //
      return EFI_SUCCESS;
    }
  }

  Device->NumOfInterface = Index;

  return EFI_SUCCESS;
}

/**
  Disconnect the USB interface with its driver.

  @param  UsbIf                 The interface to disconnect driver from.
  @param  Force                 Disconnect drivers even if not connected by the bus.

**/
EFI_STATUS
UsbDisconnectDriver (
  IN USB_INTERFACE  *UsbIf,
  IN BOOLEAN        Force
  )
{
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  //
  // Release the hub if it's a hub controller, otherwise
  // disconnect the driver if it is managed by other drivers.
  //
  Status = EFI_SUCCESS;
  if (UsbIf->IsHub) {
    Status = UsbIf->HubApi->Release (UsbIf);
  } else if (UsbIf->IsManaged || Force) {
    //
    // This function is called in both UsbIoControlTransfer and
    // the timer callback in hub enumeration. So, at least it is
    // called at TPL_CALLBACK. Some driver sitting on USB has
    // twisted TPL used. It should be no problem for us to connect
    // or disconnect at CALLBACK.
    //
    OldTpl = UsbGetCurrentTpl ();
    DEBUG ((DEBUG_INFO, "UsbDisconnectDriver: old TPL is %d, %p\n", (UINT32)OldTpl, UsbIf->Handle));

    gBS->RestoreTPL (TPL_CALLBACK);

    Status = gBS->DisconnectController (UsbIf->Handle, NULL, NULL);
    if (!EFI_ERROR (Status)) {
      UsbIf->IsManaged = FALSE;
    }

    DEBUG ((DEBUG_INFO, "UsbDisconnectDriver: TPL after disconnect is %d, %d\n", (UINT32)UsbGetCurrentTpl (), Status));
    ASSERT (UsbGetCurrentTpl () == TPL_CALLBACK);

    gBS->RaiseTPL (OldTpl);
  }

  return Status;
}

/**
  Require consumers to release an interface before software-driven replacement.
**/
EFI_STATUS
UsbInterfaceIsQuiesced (
  IN USB_INTERFACE  *UsbIf
  )
{
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *Entries;
  EFI_GUID                             *Protocols[2];
  USB_DEVICE                           *Device;
  EFI_STATUS                           Status;
  UINTN                                ProtocolIndex;
  UINTN                                EntryIndex;
  UINTN                                EntryCount;
  UINTN                                TransferIndex;

  Device = UsbIf->Device;
  if ((Device->BindingInProgress != 0) || (Device->AsyncCallbacks != 0)) {
    DEBUG ((DEBUG_INFO, "UsbInterfaceIsQuiesced: device %d has an active binding or callback\n", Device->Address));
    return EFI_NOT_READY;
  }

  if (UsbIf->IsHub) {
    DEBUG ((DEBUG_ERROR, "UsbInterfaceIsQuiesced: cannot replace a hub interface\n"));
    return EFI_ACCESS_DENIED;
  }

  for (TransferIndex = 0; TransferIndex < ARRAY_SIZE (UsbIf->AsyncTransfers); TransferIndex++) {
    if (UsbIf->AsyncTransfers[TransferIndex] != NULL) {
      DEBUG ((DEBUG_INFO, "UsbInterfaceIsQuiesced: device %d still has asynchronous transfers\n", Device->Address));
      return EFI_NOT_READY;
    }
  }

  Protocols[0] = &gEfiUsbIoProtocolGuid;
  Protocols[1] = &gEfiDevicePathProtocolGuid;
  for (ProtocolIndex = 0; ProtocolIndex < ARRAY_SIZE (Protocols); ProtocolIndex++) {
    Status = gBS->OpenProtocolInformation (UsbIf->Handle, Protocols[ProtocolIndex], &Entries, &EntryCount);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UsbInterfaceIsQuiesced: failed to query interface ownership - %r\n", Status));
      return Status;
    }

    Status = EFI_SUCCESS;
    for (EntryIndex = 0; EntryIndex < EntryCount; EntryIndex++) {
      if ((Entries[EntryIndex].Attributes &
           (EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER | EFI_OPEN_PROTOCOL_EXCLUSIVE)) != 0)
      {
        Status = EFI_ACCESS_DENIED;
        break;
      }
    }

    if (Entries != NULL) {
      FreePool (Entries);
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "UsbInterfaceIsQuiesced: device %d interface %d still has an owner\n", Device->Address, UsbIf->IfSetting->Desc.InterfaceNumber));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Check all remaining interfaces before deferred whole-device recovery.
**/
EFI_STATUS
UsbDeviceIsQuiesced (
  IN USB_DEVICE  *Device
  )
{
  EFI_STATUS          Status;
  UINTN               Index;
  USB_RETIRED_CONFIG  *Retired;

  if ((Device->BindingInProgress != 0) || (Device->AsyncCallbacks != 0)) {
    return EFI_NOT_READY;
  }

  for (Index = 0; Index < Device->NumOfInterface; Index++) {
    if (Device->Interfaces[Index] == NULL) {
      continue;
    }

    Status = UsbInterfaceIsQuiesced (Device->Interfaces[Index]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  for (Retired = Device->RetiredConfigs; Retired != NULL; Retired = Retired->Next) {
    for (Index = 0; Index < Retired->NumOfInterface; Index++) {
      if (Retired->Interfaces[Index] == NULL) {
        continue;
      }

      Status = UsbInterfaceIsQuiesced (Retired->Interfaces[Index]);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Remove retained interfaces after their consumers release them, or as part
  of normal configuration teardown when DisconnectDrivers is TRUE.
**/
EFI_STATUS
UsbRemoveRetiredConfigs (
  IN USB_DEVICE  *Device,
  IN BOOLEAN     DisconnectDrivers
  )
{
  USB_RETIRED_CONFIG  **Link;
  USB_RETIRED_CONFIG  *Retired;
  USB_INTERFACE       *UsbIf;
  EFI_STATUS          Status;
  EFI_STATUS          ReturnStatus;
  BOOLEAN             Empty;
  UINTN               Index;

  if (Device->ResetInProgress || (Device->BindingInProgress != 0) || (Device->AsyncCallbacks != 0)) {
    return EFI_NOT_READY;
  }

  ReturnStatus = EFI_SUCCESS;
  Link         = &Device->RetiredConfigs;
  while (*Link != NULL) {
    Retired = *Link;
    Empty   = TRUE;
    for (Index = 0; Index < Retired->NumOfInterface; Index++) {
      UsbIf = Retired->Interfaces[Index];
      if (UsbIf == NULL) {
        continue;
      }

      Status = DisconnectDrivers ? EFI_SUCCESS : UsbInterfaceIsQuiesced (UsbIf);
      if (!EFI_ERROR (Status)) {
        //
        // Stop() and protocol removal can dispatch notifications.
        //
        Device->BindingInProgress++;
        Status = UsbDisconnectDriver (UsbIf, DisconnectDrivers);
        if (!EFI_ERROR (Status)) {
          Status = UsbFreeInterface (UsbIf);
        }

        Device->BindingInProgress--;
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "UsbRemoveRetiredConfigs: interface teardown failed - %r\n", Status));
        }
      }

      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
        Empty        = FALSE;
      } else {
        Retired->Interfaces[Index] = NULL;
      }
    }

    if (Empty) {
      *Link = Retired->Next;
      UsbFreeDevDesc (Retired->DevDesc);
      FreePool (Retired);
    } else {
      Link = &Retired->Next;
    }
  }

  return ReturnStatus;
}

/**
  Remove the current device configuration.

  @param  Device                The USB device to remove configuration from.

**/
EFI_STATUS
UsbRemoveConfig (
  IN USB_DEVICE  *Device
  )
{
  USB_INTERFACE  *UsbIf;
  UINTN          Index;
  EFI_STATUS     Status;
  EFI_STATUS     ReturnStatus;

  if (Device->ResetInProgress || (Device->BindingInProgress != 0) || (Device->AsyncCallbacks != 0)) {
    DEBUG ((DEBUG_ERROR, "UsbRemoveConfig: binding or callback is still executing\n"));
    return EFI_NOT_READY;
  }

  //
  // Remove each interface of the device
  //
  ReturnStatus = EFI_SUCCESS;
  for (Index = 0; Index < Device->NumOfInterface; Index++) {
    ASSERT (Index < USB_MAX_INTERFACE);
    UsbIf = Device->Interfaces[Index];

    if (UsbIf == NULL) {
      continue;
    }

    Status = UsbRemoveRetiredConfigs (Device, TRUE);
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }

    Status = UsbDisconnectDriver (UsbIf, (BOOLEAN) !Device->Connected);
    if (!EFI_ERROR (Status)) {
      Status = UsbFreeInterface (UsbIf);
      if (EFI_ERROR (Status) && Device->Connected && !UsbIf->Disconnected) {
        UsbConnectDriver (UsbIf);
      }
    }

    if (!EFI_ERROR (Status)) {
      Device->Interfaces[Index] = NULL;
    } else {
      ReturnStatus = Status;
    }
  }

  if (!EFI_ERROR (ReturnStatus)) {
    Device->ActiveConfig = NULL;
  }

  return ReturnStatus;
}

/**
  Remove the device and all its children from the bus.

  @param  Device                The device to remove.

  @retval EFI_SUCCESS           The device is removed.
  @retval EFI_NOT_READY         A reset, binding, or asynchronous callback is active.

**/
EFI_STATUS
UsbRemoveDevice (
  IN USB_DEVICE  *Device
  )
{
  USB_BUS     *Bus;
  USB_DEVICE  *Child;
  EFI_STATUS  Status;
  EFI_STATUS  ReturnStatus;
  UINTN       Index;

  if (Device->ResetInProgress || (Device->BindingInProgress != 0) || (Device->AsyncCallbacks != 0)) {
    DEBUG ((DEBUG_ERROR, "UsbRemoveDevice: reset, binding, or callback in progress for device %d\n", Device->Address));
    return EFI_NOT_READY;
  }

  Bus = Device->Bus;

  //
  // Remove all the devices on its downstream ports. Search from devices[1].
  // Devices[0] is the root hub.
  //
  ReturnStatus = EFI_SUCCESS;
  for (Index = 1; Index < Bus->MaxDevices; Index++) {
    Child = Bus->Devices[Index];

    if ((Child == NULL) || (Child->ParentAddr != Device->Address)) {
      continue;
    }

    Status = UsbRemoveDevice (Child);

    if (!EFI_ERROR (Status)) {
      Bus->Devices[Index] = NULL;
    } else {
      Bus->Devices[Index]->DisconnectFail = TRUE;
      ReturnStatus                        = Status;
      DEBUG ((DEBUG_INFO, "UsbRemoveDevice: failed to remove child %p at parent %p\n", Child, Device));
    }
  }

  if (EFI_ERROR (ReturnStatus)) {
    return ReturnStatus;
  }

  // MU_CHANGE [BEGIN]
  Device->Connected = FALSE;
  DEBUG ((DEBUG_INFO, "UsbRemoveDevice: ParentIf %p port %d Disonnected\n", Device->ParentIf, Device->ParentPort));
  // MU_CHANGE [END]
  Status = UsbRemoveConfig (Device);

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UsbRemoveDevice: device %d removed\n", Device->Address));

    ASSERT (Device->Address < Bus->MaxDevices);
    Bus->Devices[Device->Address] = NULL;
    UsbFreeDevice (Device);
  } else {
    Bus->Devices[Device->Address]->DisconnectFail = TRUE;
  }

  return Status;
}

/**
  Find the child device on the hub's port.

  @param  HubIf                 The hub interface.
  @param  Port                  The port of the hub this child is connected to.

  @return The device on the hub's port, or NULL if there is none.

**/
USB_DEVICE *
UsbFindChild (
  IN USB_INTERFACE  *HubIf,
  IN UINT8          Port
  )
{
  USB_DEVICE  *Device;
  USB_BUS     *Bus;
  UINTN       Index;

  Bus = HubIf->Device->Bus;

  //
  // Start checking from device 1, device 0 is the root hub
  //
  for (Index = 1; Index < Bus->MaxDevices; Index++) {
    Device = Bus->Devices[Index];

    if ((Device != NULL) && (Device->ParentAddr == HubIf->Device->Address) &&
        (Device->ParentPort == Port))
    {
      return Device;
    }
  }

  return NULL;
}

/**
  Enumerate and configure the new device on the port of this HUB interface.

  @param  HubIf                 The HUB that has the device connected.
  @param  Port                  The port index of the hub (started with zero).
  @param  ResetIsNeeded         The boolean to control whether skip the reset of the port.

  @retval EFI_SUCCESS           The device is enumerated (added or removed).
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the device.
  @retval Others                Failed to enumerate the device.

**/
EFI_STATUS
UsbEnumerateNewDev (
  IN USB_INTERFACE  *HubIf,
  IN UINT8          Port,
  IN BOOLEAN        ResetIsNeeded
  )
{
  USB_BUS              *Bus;
  USB_HUB_API          *HubApi;
  USB_DEVICE           *Child;
  USB_DEVICE           *Parent;
  EFI_USB_PORT_STATUS  PortState;
  UINTN                Address;
  UINT8                Config;
  EFI_STATUS           Status;
  EFI_STATUS           OriginalStatus;
  UINT8                RetryCount;

  Parent  = HubIf->Device;
  Bus     = Parent->Bus;
  HubApi  = HubIf->HubApi;
  Address = Bus->MaxDevices;

  gBS->Stall (USB_WAIT_PORT_STABLE_STALL);

  //
  // Hub resets the device for at least 10 milliseconds.
  // Host learns device speed. If device is of low/full speed
  // and the hub is a EHCI root hub, ResetPort will release
  // the device to its companion UHCI and return an error.
  //
  if (ResetIsNeeded) {
    Status = HubApi->ResetPort (HubIf, Port);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to reset port %d - %r\n", Port, Status));

      return Status;
    }

    DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: hub port %d is reset\n", Port));
  } else {
    DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: hub port %d reset is skipped\n", Port));
  }

  Child = UsbCreateDevice (HubIf, Port);

  if (Child == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  RetryCount = 3;

DeviceRetry:
  //
  // OK, now identify the device speed. After reset, hub
  // fully knows the actual device speed.
  //
  Status = HubApi->GetPortStatus (HubIf, Port, &PortState);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to get speed of port %d\n", Port));
    goto ON_ERROR;
  }

  //
  // RetryCount is used to execute the different enum scripts.
  // Current the diffrence is only for getting Configuration Descriptor.
  // +--------------------+---+----------------------------------------------------+
  // | UsbEnumScriptWin   | 3 | UsbGetOneConfig get the descriptor with 255 bytes  |
  // +--------------------+---+----------------------------------------------------+
  // | UsbEnumScriptLinux | 2 | UsbGetOneConfig get the descriptor with            |
  // |                    |   | ConfigurationDescriptor Length.                    |
  // +--------------------+---+----------------------------------------------------+
  // | UsbEnumScriptRsrv  | 1 | UsbGetOneConfig get the descriptor with 8 bytes    |
  // |                    |   | and then get the total length. (Same as Edk2 flow) |
  // +--------------------+---+----------------------------------------------------+
  // | UsbEnumScriptEdk2  | 0 | UsbGetOneConfig get the descriptor with 8 bytes    |
  // |                    |   | and then get the total length.                     |
  // +--------------------+--------------------------------------------------------+
  //
  Child->EnumScript = RetryCount;

  if (!USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_CONNECTION)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: No device present at port %d\n", Port));
    Status     = EFI_NOT_FOUND;
    RetryCount = 0;
    goto ON_ERROR;
  } else if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_SUPER_SPEED)) {
    Child->Speed      = EFI_USB_SPEED_SUPER;
    Child->MaxPacket0 = 512;
  } else if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_HIGH_SPEED)) {
    Child->Speed      = EFI_USB_SPEED_HIGH;
    Child->MaxPacket0 = 64;
  } else if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_LOW_SPEED)) {
    Child->Speed      = EFI_USB_SPEED_LOW;
    Child->MaxPacket0 = 8;
  } else {
    Child->Speed      = EFI_USB_SPEED_FULL;
    Child->MaxPacket0 = 8;
  }

  DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: device is of %d speed\n", Child->Speed));

  if (((Child->Speed == EFI_USB_SPEED_LOW) || (Child->Speed == EFI_USB_SPEED_FULL)) &&
      (Parent->Speed == EFI_USB_SPEED_HIGH))
  {
    //
    // If the child is a low or full speed device, it is necessary to
    // set the transaction translator. Port TT is 1-based.
    // This is quite simple:
    //  1. if parent is of high speed, then parent is our translator
    //  2. otherwise use parent's translator.
    //
    Child->Translator.TranslatorHubAddress = Parent->Address;
    Child->Translator.TranslatorPortNumber = (UINT8)(Port + 1);
  } else {
    Child->Translator = Parent->Translator;
  }

  DEBUG ((
    DEBUG_INFO,
    "UsbEnumerateNewDev: device uses translator (%d, %d)\n",
    Child->Translator.TranslatorHubAddress,
    Child->Translator.TranslatorPortNumber
    ));

  //
  // After port is reset, hub establishes a signal path between
  // the device and host (DEFAULT state). Device's registers are
  // reset, use default address 0 (host enumerates one device at
  // a time) , and ready to respond to control transfer at EP 0.
  //

  //
  // Host assigns an address to the device. Device completes the
  // status stage with default address, then switches to new address.
  // ADDRESS state. Address zero is reserved for root hub.
  //
  ASSERT (Bus->MaxDevices <= 256);
  if (Child->Address == 0) {
    for (Address = 1; Address < Bus->MaxDevices; Address++) {
      if (Bus->Devices[Address] == NULL) {
        break;
      }
    }

    if (Address >= Bus->MaxDevices) {
      DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: address pool is full for port %d\n", Port));

      Status = EFI_ACCESS_DENIED;
      goto ON_ERROR;
    }
  } else {
    Address               = Child->Address;
    Child->Address        = 0;
    Bus->Devices[Address] = NULL;
  }

  Status                = UsbSetAddress (Child, (UINT8)Address);
  Child->Address        = (UINT8)Address;
  Bus->Devices[Address] = Child;

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to set device address - %r\n", Status));
    goto ON_ERROR;
  }

  gBS->Stall (USB_SET_DEVICE_ADDRESS_STALL);

  DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: device is now ADDRESSED at %d\n", Address));

  //
  // Host sends a Get_Descriptor request to learn the max packet
  // size of default pipe (only part of the device's descriptor).
  //
  Status = UsbGetMaxPacketSize0 (Child);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to get max packet for EP 0 - %r\n", Status));
    goto ON_ERROR;
  }

  DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: max packet size for EP 0 is %d\n", Child->MaxPacket0));

  //
  // Host learns about the device's abilities by requesting device's
  // entire descriptions.
  //
  Status = UsbBuildDescTable (Child);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to build descriptor table - %r\n", Status));
    goto ON_ERROR;
  }

  // Below code is ensuring the device can be executed with SS.
  // Some device FW might execute with SS later but it would produce failure if the device is already enumerated with HS.
  if (  (RetryCount > 0)
     && (Bus->Usb2Hc != NULL)
     && (Bus->Usb2Hc->MajorRevision >= 0x3)
     && (Child->IsSSDev)
     && (Child->Speed < EFI_USB_SPEED_SUPER)
     && (Child->DevDesc->Desc.DeviceClass != USB_HUB_CLASS_CODE))
  {
    goto ON_ERROR;
  }

  //
  // Select a default configuration: UEFI must set the configuration
  // before the driver can connect to the device.
  //
  Config = Child->DevDesc->Configs[0]->Desc.ConfigurationValue;
  Status = UsbSetConfig (Child, Config);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to set configure %d - %r\n", Config, Status));
    goto ON_ERROR;
  }

  DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: device %d is now in CONFIGED state\n", Address));

  //
  // Host assigns and loads a device driver.
  //
  Status = UsbSelectConfig (Child, Config, TRUE);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to create interfaces - %r\n", Status));
    goto ON_ERROR;
  }

  //
  // Report Status Code to indicate USB device has been detected by hotplug
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_USB | EFI_IOB_PC_HOTPLUG),
    Bus->DevicePath
    );
  return EFI_SUCCESS;

ON_ERROR:
  //
  // Do the error handling with retry counter.
  //
  OriginalStatus = Status;
  if ((Child != NULL) && (Child->NumOfInterface != 0)) {
    //
    // Installed interfaces still own the descriptor tree. Do not retry in place.
    //
    return OriginalStatus;
  }

  Status = HubApi->GetPortStatus (HubIf, Port, &PortState);
  if (EFI_ERROR (Status) || (!USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_CONNECTION))) {
    DEBUG ((DEBUG_ERROR, "Device is gone. Don't reset the port\n"));
    Status = EFI_NOT_FOUND;
  }

  if (!EFI_ERROR (Status) && (RetryCount > 0)) {
    RetryCount--;
    DEBUG ((DEBUG_INFO, "Reset the port due to Error\n"));
    if ((Child != NULL) && (Child->DevDesc != NULL)) {
      UsbFreeDevDesc (Child->DevDesc);
      Child->DevDesc = NULL;
    }

    Status = HubApi->ResetPort (HubIf, Port);
    if (!EFI_ERROR (Status)) {
      gBS->Stall (USB_WAIT_PORT_STABLE_STALL);
      goto DeviceRetry;
    }
  }

  //
  // If reach here, it means the enumeration process on a given port is interrupted due to error.
  // The s/w resources, including the assigned address(Address) and the allocated usb device data
  // structure(Bus->Devices[Address]), will NOT be freed here. These resources will be freed when
  // the device is unplugged from the port or DriverBindingStop() is invoked.
  //
  // This way is used to co-work with the lower layer EDKII UHCI/EHCI/XHCI host controller driver.
  // It's mainly because to keep UEFI spec unchanged EDKII XHCI driver have to maintain a state machine
  // to keep track of the mapping between actual address and request address. If the request address
  // (Address) is freed here, the Address value will be used by next enumerated device. Then EDKII XHCI
  // host controller driver will have wrong information, which will cause further transaction error.
  //
  // EDKII UHCI/EHCI doesn't get impacted as it's make sense to reserve s/w resource till it gets unplugged.
  //
  return OriginalStatus;
}

/**
  Process the events on the port, including software-requested reset recovery.
  Recovery does not require hardware change bits and always resets the port.

  @param  HubIf                 The HUB that has the device connected.
  @param  Port                  The port index of the hub (started with zero).

  @retval EFI_SUCCESS           The device is enumerated (added or removed).
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the device.
  @retval Others                Failed to enumerate the device.

**/
EFI_STATUS
UsbEnumeratePort (
  IN USB_INTERFACE  *HubIf,
  IN UINT8          Port
  )
{
  USB_HUB_API          *HubApi;
  USB_DEVICE           *Child;
  EFI_USB_PORT_STATUS  PortState;
  EFI_STATUS           Status;
  EFI_STATUS           RemoveStatus;
  USB_BUS              *Bus;
  BOOLEAN              Recover;
  BOOLEAN              RecoveryInProgress;
  UINTN                Index;

  Bus     = HubIf->Device->Bus;
  Child   = UsbFindChild (HubIf, Port);
  HubApi  = HubIf->HubApi;
  Recover = (BOOLEAN)((Child != NULL) && Child->Reenumerate);

  if ((Child != NULL) &&
      (Child->ResetInProgress || (Child->BindingInProgress != 0) || (Child->AsyncCallbacks != 0)))
  {
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: device operation in progress on port %d\n", Port));
    return EFI_NOT_READY;
  }

  // Zero out PortState in case GetPortStatus does not set
  // it and we continue on the EFI_DEVICE_ERROR path
  PortState.PortStatus       = 0;
  PortState.PortChangeStatus = 0;

  //
  // Host learns of the new device by polling the hub for port changes.
  //
  Status = HubApi->GetPortStatus (HubIf, Port, &PortState);

  if (EFI_ERROR (Status) && (Recover || (Status != EFI_DEVICE_ERROR))) {
    DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: failed to get state of port %d\n", Port));
    return Status;
  }

  if (Recover && USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_CONNECTION)) {
    RemoveStatus = UsbDeviceIsQuiesced (Child);
    if (EFI_ERROR (RemoveStatus)) {
      return RemoveStatus;
    }

    for (Index = 0; Index < Child->NumOfInterface; Index++) {
      if (Child->Interfaces[Index] != NULL) {
        Child->Interfaces[Index]->IsManaged = FALSE;
      }
    }
  }

  //
  // Only handle connection/enable/overcurrent/reset change.
  // Usb super speed hub may report other changes, such as warm reset change. Ignore them.
  //
  if (!Recover &&
      ((PortState.PortChangeStatus & (USB_PORT_STAT_C_CONNECTION | USB_PORT_STAT_C_ENABLE | USB_PORT_STAT_C_OVERCURRENT | USB_PORT_STAT_C_RESET)) == 0))
  {
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_INFO,
    "UsbEnumeratePort: port %d state - %02x, change - %02x on %p\n",
    Port,
    PortState.PortStatus,
    PortState.PortChangeStatus,
    HubIf
    ));

  //
  // This driver only process two kinds of events now: over current and
  // connect/disconnect. Other three events are: ENABLE, SUSPEND, RESET.
  // ENABLE/RESET is used to reset port. SUSPEND isn't supported.
  //

  if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_OVERCURRENT)) {
    if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_OVERCURRENT)) {
      //
      // Case1:
      //   Both OverCurrent and OverCurrentChange set, means over current occurs,
      //   which probably is caused by short circuit. It has to wait system hardware
      //   to perform recovery.
      //
      DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: Critical Over Current (port %d)\n", Port));
      return EFI_DEVICE_ERROR;
    }

    //
    // Case2:
    //   Only OverCurrentChange set, means system has been recoveried from
    //   over current. As a result, all ports are nearly power-off, so
    //   it's necessary to detach and enumerate all ports again.
    //
    DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: 2.0 device Recovery Over Current (port %d)\n", Port));
  }

  if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_ENABLE)) {
    //
    // Case3:
    //   1.1 roothub port reg doesn't reflect over-current state, while its counterpart
    //   on 2.0 roothub does. When over-current has influence on 1.1 device, the port
    //   would be disabled, so it's also necessary to detach and enumerate again.
    //
    DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: 1.1 device Recovery Over Current (port %d)\n", Port));
  }

  if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_CONNECTION)) {
    //
    // Case4:
    //   Device connected or disconnected normally.
    //
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: Device Connect/Disconnect Normally (port %d)\n", Port));
  }

  //
  // Following as the above cases, it's safety to remove and create again.
  //
  Child = UsbFindChild (HubIf, Port);

  if (Recover) {
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: starting deferred recovery on port %d@hub %d\n", Port, HubIf->Device->Address));
  }

  if (Child != NULL) {
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: device at port %d removed from root hub %p\n", Port, HubIf));
    RemoveStatus = UsbRemoveDevice (Child);
    if (EFI_ERROR (RemoveStatus)) {
      DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: failed to remove device on port %d - %r\n", Port, RemoveStatus));
      return RemoveStatus;
    }
  }

  if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_CONNECTION)) {
    //
    // Now, new device connected, enumerate and configure the device
    //
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: new device connected at port %d\n", Port));
    RecoveryInProgress      = Bus->RecoveryInProgress;
    Bus->RecoveryInProgress = (BOOLEAN)(RecoveryInProgress || Recover);
    if (!Recover &&
        USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_RESET) &&
        (Status != EFI_DEVICE_ERROR))
    {
      Status = UsbEnumerateNewDev (HubIf, Port, FALSE);
    } else {
      Status = UsbEnumerateNewDev (HubIf, Port, TRUE);
    }

    Bus->RecoveryInProgress = RecoveryInProgress;
  } else {
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: device disconnected event on port %d\n", Port));
  }

  if (Recover) {
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: deferred recovery on port %d@hub %d finished - %r\n", Port, HubIf->Device->Address, Status));
  }

  HubApi->ClearPortChange (HubIf, Port);
  return Status;
}

/**
  Enumerate all the changed hub ports.

  @param  Event                 The event that is triggered.
  @param  Context               The context to the event.

**/
VOID
EFIAPI
UsbHubEnumeration (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  USB_INTERFACE  *HubIf;
  UINT8          Byte;
  UINT8          Bit;
  UINT8          Index;
  USB_DEVICE     *Child;

  ASSERT (Context != NULL);

  HubIf = (USB_INTERFACE *)Context;

  for (Index = 0; Index < HubIf->NumOfPort; Index++) {
    Child = UsbFindChild (HubIf, Index);
    if ((Child != NULL) && (Child->DisconnectFail == TRUE) && !Child->Reenumerate) {
      DEBUG ((DEBUG_INFO, "UsbEnumeratePort: The device disconnect fails at port %d from hub %p, try again\n", Index, HubIf));
      UsbRemoveDevice (Child);
    }
  }

  if (HubIf->ChangeMap == NULL) {
    return;
  }

  //
  // HUB starts its port index with 1.
  //
  Byte = 0;
  Bit  = 1;

  for (Index = 0; Index < HubIf->NumOfPort; Index++) {
    if (USB_BIT_IS_SET (HubIf->ChangeMap[Byte], USB_BIT (Bit))) {
      UsbEnumeratePort (HubIf, Index);
    }

    USB_NEXT_BIT (Byte, Bit);
  }

  UsbHubAckHubStatus (HubIf->Device);

  gBS->FreePool (HubIf->ChangeMap);
  HubIf->ChangeMap = NULL;
  return;
}

/**
  Enumerate all the changed hub ports.

  @param  Event                 The event that is triggered.
  @param  Context               The context to the event.

**/
VOID
EFIAPI
UsbRootHubEnumeration (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  USB_INTERFACE  *RootHub;
  UINT8          Index;
  USB_DEVICE     *Child;
  USB_BUS        *Bus;
  UINTN          Address;
  EFI_STATUS     Status;

  RootHub = (USB_INTERFACE *)Context;
  Bus     = RootHub->Device->Bus;

  // MU_CHANGE [BEGIN]
  // MU_CHANGE Implement Enumeration delay
  if (RootHub->PollCount < USB_ENUM_POLL_MAXIMUM_ATTEMPTS) {
    RootHub->PollCount++;
  }

  // MU_CHANGE [END]

  //
  // The root timer also services devices behind external hubs, even when their
  // interrupt endpoint reports no port changes. No separate event owns a child.
  //
  for (Address = 1; Address < Bus->MaxDevices; Address++) {
    Child = Bus->Devices[Address];
    if ((Child != NULL) && Child->Reenumerate) {
      Status = UsbEnumeratePort (Child->ParentIf, Child->ParentPort);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "UsbRootHubEnumeration: deferred recovery failed - %r\n", Status));
      }
    }
  }

  for (Address = 1; Address < Bus->MaxDevices; Address++) {
    Child = Bus->Devices[Address];
    if ((Child == NULL) || (Child->RetiredConfigs == NULL)) {
      continue;
    }

    Status = UsbRemoveRetiredConfigs (Child, FALSE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "UsbRootHubEnumeration: retained interfaces await cleanup - %r\n", Status));
    }
  }

  for (Index = 0; Index < RootHub->NumOfPort; Index++) {
    Child = UsbFindChild (RootHub, Index);
    if ((Child != NULL) && Child->Reenumerate) {
      continue;
    }

    if ((Child != NULL) && (Child->DisconnectFail == TRUE)) {
      DEBUG ((DEBUG_INFO, "UsbEnumeratePort: The device disconnect fails at port %d from root hub %p, try again\n", Index, RootHub));
      UsbRemoveDevice (Child);
    }

    UsbEnumeratePort (RootHub, Index);
  }
}
