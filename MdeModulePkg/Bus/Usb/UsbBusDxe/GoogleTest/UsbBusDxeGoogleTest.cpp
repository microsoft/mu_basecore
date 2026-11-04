/** @file
  Exercise unchanged, runtime-to-DFU, and DFU-to-runtime USB resets.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

extern "C" {
  #include "../UsbBus.h"
}

namespace {
#pragma pack(push, 1)
/// USB DFU 1.1 functional descriptor, section 4.2.3.
struct DfuFunctionalDescriptor {
  UINT8     Length;
  UINT8     DescriptorType;
  UINT8     Attributes;
  UINT16    DetachTimeout;
  UINT16    TransferSize;
  UINT16    DfuVersion;
};

#pragma pack(pop)

static_assert (sizeof (DfuFunctionalDescriptor) == 9, "DFU functional descriptor must be nine bytes");

struct PublishedInterface {
  UINTN                       Id;
  EFI_DEVICE_PATH_PROTOCOL    *Path;
  EFI_USB_IO_PROTOCOL         *UsbIo;
};

class UsbDfuResetTest : public testing::Test {
protected:
  static UsbDfuResetTest *Active;
  EFI_BOOT_SERVICES Services { };
  EFI_BOOT_SERVICES *SavedServices { };
  EFI_USB2_HC_PROTOCOL Host { };
  USB_HUB_API HubApi { };
  USB_BUS Bus { };
  USB_DEVICE Parent { };
  USB_INTERFACE Hub { };
  USB_DEVICE *Device { };
  EFI_DEVICE_PATH_PROTOCOL ParentPath {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { END_DEVICE_PATH_LENGTH, 0 }
  };
  std::vector<std::unique_ptr<PublishedInterface> > Published;
  std::vector<UINTN> Removed;
  std::vector<UINT8> Configuration;
  EFI_USB_DEVICE_DESCRIPTOR Descriptor { };
  EFI_TPL CurrentTpl = TPL_APPLICATION;
  UINTN InstallCount = 0;
  bool NextDfu       = false;

  /**
    Install fake Boot Services and initialize the USB device and parent hub.
  **/
  void
  SetUp (
    ) override
  {
    Active                                       = this;
    SavedServices                                = gBS;
    Services.RaiseTPL                            = RaiseTpl;
    Services.RestoreTPL                          = RestoreTpl;
    Services.Stall                               = Stall;
    Services.FreePool                            = ReleasePool;
    Services.InstallMultipleProtocolInterfaces   = Install;
    Services.UninstallMultipleProtocolInterfaces = Uninstall;
    Services.OpenProtocol                        = Open;
    Services.CloseProtocol                       = Close;
    Services.ConnectController                   = Connect;
    Services.DisconnectController                = Disconnect;
    gBS                                          = &Services;
    Host.ControlTransfer                         = ControlTransfer;
    HubApi.ResetPort                             = ResetPort;
    HubApi.ClearPortChange                       = ClearPortChange;
    Bus.Usb2Hc                                   = &Host;
    Bus.Signature                                = USB_BUS_SIGNATURE;
    Bus.MaxDevices                               = USB_MAX_DEVICES;
    InitializeListHead (&Bus.WantedUsbIoDPList);
    Parent.Bus     = &Bus;
    Hub.Device     = &Parent;
    Hub.DevicePath = &ParentPath;
    Hub.HubApi     = &HubApi;
    Device         = static_cast<USB_DEVICE *> (AllocateZeroPool (sizeof (USB_DEVICE)));
    ASSERT_NE (Device, nullptr);
    Device->Bus        = &Bus;
    Device->ParentIf   = &Hub;
    Device->ParentPort = 2;
    Device->Connected  = TRUE;
    Device->MaxPacket0 = 64;
    Device->Address    = 1;
    Device->Speed      = EFI_USB_SPEED_HIGH;
    Bus.Devices[1]     = Device;
  }

  /**
    Remove the test device, verify its protocols are gone, and restore Boot Services.
  **/
  void
  TearDown (
    ) override
  {
    if (Device != nullptr) {
      EXPECT_EQ (UsbRemoveDevice (Device), EFI_SUCCESS);
    }

    EXPECT_TRUE (Published.empty ());
    EXPECT_EQ (UsbBusFreeUsbDPList (&Bus.WantedUsbIoDPList), EFI_SUCCESS);
    gBS    = SavedServices;
    Active = nullptr;
  }

  /**
    Build the descriptors returned by the emulated host controller.

    @param[in] Dfu  True for one DFU interface; false for four runtime interfaces.
  **/
  void
  SetInterfaceLayout (
    bool  Dfu
    )
  {
    ZeroMem (&Descriptor, sizeof (Descriptor));
    Descriptor.Length            = sizeof (Descriptor);
    Descriptor.DescriptorType    = USB_DESC_TYPE_DEVICE;
    Descriptor.BcdUSB            = 0x200;
    Descriptor.MaxPacketSize0    = 64;
    Descriptor.IdVendor          = 0x13D3;
    Descriptor.IdProduct         = Dfu ? 0x5800 : 0x5D48;
    Descriptor.NumConfigurations = 1;
    EFI_USB_CONFIG_DESCRIPTOR  Config { };

    Config.Length             = sizeof (Config);
    Config.DescriptorType     = USB_DESC_TYPE_CONFIG;
    Config.NumInterfaces      = Dfu ? 1 : 4;
    Config.ConfigurationValue = 1;
    Config.Attributes         = 0x80;
    Config.MaxPower           = 50;
    Config.TotalLength        = static_cast<UINT16> (
                                                     sizeof (Config) +
                                                     Config.NumInterfaces * sizeof (EFI_USB_INTERFACE_DESCRIPTOR) +
                                                     (Dfu ? sizeof (DfuFunctionalDescriptor) : 0)
                                                     );
    Configuration.resize (Config.TotalLength);
    CopyMem (Configuration.data (), &Config, sizeof (Config));
    UINTN  Offset = sizeof (Config);

    for (UINT8 Index = 0; Index < Config.NumInterfaces; ++Index) {
      EFI_USB_INTERFACE_DESCRIPTOR  Function { };
      Function.Length            = sizeof (Function);
      Function.DescriptorType    = USB_DESC_TYPE_INTERFACE;
      Function.InterfaceNumber   = Index;
      Function.InterfaceClass    = Dfu ? 0xFE : 0x0E;
      Function.InterfaceSubClass = 1;
      Function.InterfaceProtocol = Dfu ? 2 : 0;
      CopyMem (Configuration.data () + Offset, &Function, sizeof (Function));
      Offset += sizeof (Function);
    }

    if (Dfu) {
      DfuFunctionalDescriptor  Functional { };
      Functional.Length         = sizeof (Functional);
      Functional.DescriptorType = 0x21;
      Functional.Attributes     = 0x0B;
      Functional.DetachTimeout  = 0x00FF;
      Functional.TransferSize   = 0x0400;
      Functional.DfuVersion     = 0x0110;
      CopyMem (Configuration.data () + Offset, &Functional, sizeof (Functional));
    }
  }

  /**
    Read descriptors and publish the initial interfaces without connecting drivers.

    @param[in] Dfu  Whether the device starts in DFU mode.
  **/
  void
  StartDevice (
    bool  Dfu
    )
  {
    SetInterfaceLayout (Dfu);
    ASSERT_EQ (UsbBuildDescTable (Device), EFI_SUCCESS);
    ASSERT_EQ (UsbSetConfig (Device, 1), EFI_SUCCESS);
    ASSERT_EQ (UsbSelectConfig (Device, 1, FALSE), EFI_SUCCESS);
    ExpectInterfaceLayout (Dfu);
  }

  /**
    Select the next interface layout and reset through an installed USB I/O protocol.

    @param[in] Dfu  Whether the device enters DFU mode on this reset.

    @return The status returned by UsbPortReset().
  **/
  EFI_STATUS
  Reset (
    bool  Dfu
    )
  {
    NextDfu = Dfu;
    EFI_USB_IO_PROTOCOL  *UsbIo = Published.front ()->UsbIo;

    return UsbIo->UsbPortReset (UsbIo);
  }

  /**
    Snapshot installation IDs without retaining pointers that reset may invalidate.

    @return The IDs of all currently published interfaces, in publication order.
  **/
  std::vector<UINTN>
  PublishedIds (
    )
  {
    std::vector<UINTN>  Ids;

    for (const auto &Interface : Published) {
      Ids.push_back (Interface->Id);
    }

    return Ids;
  }

  /**
    Verify the installed USB I/O protocols expose the expected interface layout.

    @param[in] Dfu  True to expect one DFU interface; false to expect four runtime interfaces.
  **/
  void
  ExpectInterfaceLayout (
    bool  Dfu
    )
  {
    const UINT8  Count = Dfu ? 1 : 4;

    ASSERT_EQ (Published.size (), Count);
    EXPECT_EQ (Device->NumOfInterface, Count);
    EXPECT_TRUE (Device->Connected);
    for (UINT8 Index = 0; Index < Count; ++Index) {
      EFI_USB_IO_PROTOCOL  *UsbIo = Published[Index]->UsbIo;
      ASSERT_NE (UsbIo, nullptr);
      EFI_USB_DEVICE_DESCRIPTOR     DeviceDescriptor { };
      EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor { };
      EFI_USB_CONFIG_DESCRIPTOR     ConfigDescriptor { };
      ASSERT_EQ (UsbIo->UsbGetDeviceDescriptor (UsbIo, &DeviceDescriptor), EFI_SUCCESS);
      ASSERT_EQ (UsbIo->UsbGetInterfaceDescriptor (UsbIo, &InterfaceDescriptor), EFI_SUCCESS);
      ASSERT_EQ (UsbIo->UsbGetConfigDescriptor (UsbIo, &ConfigDescriptor), EFI_SUCCESS);
      EXPECT_EQ (DeviceDescriptor.IdProduct, Dfu ? 0x5800 : 0x5D48);
      EXPECT_EQ (ConfigDescriptor.NumInterfaces, Count);
      EXPECT_EQ (InterfaceDescriptor.InterfaceNumber, Index);
      EXPECT_EQ (InterfaceDescriptor.InterfaceClass, Dfu ? 0xFE : 0x0E);
      EXPECT_EQ (InterfaceDescriptor.InterfaceProtocol, Dfu ? 2 : 0);
    }
  }

  /**
    Update the fake current TPL without dispatching firmware events.

    @param[in] Tpl  The new TPL.

    @return The previous TPL.
  **/
  static EFI_TPL EFIAPI
  RaiseTpl (
    EFI_TPL  Tpl
    )
  {
    const EFI_TPL  Previous = Active->CurrentTpl;

    Active->CurrentTpl = Tpl;
    return Previous;
  }

  /**
    Restore the fake current TPL without dispatching firmware events.

    @param[in] Tpl  The TPL to restore.
  **/
  static VOID EFIAPI
  RestoreTpl (
    EFI_TPL  Tpl
    )
  {
    Active->CurrentTpl = Tpl;
  }

  /**
    Accept a firmware delay without delaying the host test.

    @param[in] Microseconds  The requested delay, ignored.

    @retval EFI_SUCCESS  The delay was skipped.
  **/
  static EFI_STATUS EFIAPI
  Stall (
    UINTN  Microseconds
    )
  {
    (void)Microseconds;
    return EFI_SUCCESS;
  }

  /**
    Release a pool allocation using the host memory-allocation library.

    @param[in] Buffer  The allocation to release.

    @retval EFI_SUCCESS  The allocation was released.
  **/
  static EFI_STATUS EFIAPI
  ReleasePool (
    VOID  *Buffer
    )
  {
    FreePool (Buffer);
    return EFI_SUCCESS;
  }

  /**
    Record a Device Path and USB I/O installation, rejecting duplicate paths.

    @param[out] Handle  Receives the fake handle for the installed protocols.
    @param[in]  ...     Device Path and USB I/O GUID/interface pairs, followed by NULL.

    @retval EFI_SUCCESS          The interface was recorded.
    @retval EFI_ALREADY_STARTED  The device path is already installed.
  **/
  static EFI_STATUS EFIAPI
  Install (
    EFI_HANDLE  *Handle,
    ...
    )
  {
    VA_LIST  Args;

    VA_START (Args, Handle);
    EXPECT_EQ (VA_ARG (Args, EFI_GUID *), &gEfiDevicePathProtocolGuid);
    auto  *Path = VA_ARG (Args, EFI_DEVICE_PATH_PROTOCOL *);

    EXPECT_EQ (VA_ARG (Args, EFI_GUID *), &gEfiUsbIoProtocolGuid);
    auto  *UsbIo = VA_ARG (Args, EFI_USB_IO_PROTOCOL *);

    EXPECT_EQ (VA_ARG (Args, VOID *), nullptr);
    VA_END (Args);
    for (const auto &Existing : Active->Published) {
      if ((GetDevicePathSize (Existing->Path) == GetDevicePathSize (Path)) &&
          (CompareMem (Existing->Path, Path, GetDevicePathSize (Path)) == 0))
      {
        return EFI_ALREADY_STARTED;
      }
    }

    auto  Record = std::make_unique<PublishedInterface> ();

    Record->Id    = ++Active->InstallCount;
    Record->Path  = Path;
    Record->UsbIo = UsbIo;
    *Handle = Record.get ();
    Active->Published.push_back (std::move (Record));
    return EFI_SUCCESS;
  }

  /**
    Remove an installed interface and record its ID for replacement assertions.

    @param[in] Handle  The fake handle to remove.
    @param[in] ...     Protocol GUID/interface pairs, ignored by this fake.

    @retval EFI_SUCCESS    The interface was removed.
    @retval EFI_NOT_FOUND  The handle is unknown; a test failure is also recorded.
  **/
  static EFI_STATUS EFIAPI
  Uninstall (
    EFI_HANDLE  Handle,
    ...
    )
  {
    const auto  Found = std::find_if (
                               Active->Published.begin (),
                               Active->Published.end (),
                               [Handle](const std::unique_ptr<PublishedInterface> &Item) {
        return Item.get () == Handle;
      }
                               );

    if (Found == Active->Published.end ()) {
      ADD_FAILURE () << "Uninstalling an unknown handle";
      return EFI_NOT_FOUND;
    }

    Active->Removed.push_back ((*Found)->Id);
    Active->Published.erase (Found);
    return EFI_SUCCESS;
  }

  /**
    Supply the fake host controller for a USB interface's child-controller open.

    @param[in]  Handle      The controller handle, ignored.
    @param[in]  Protocol    Expected to identify the USB 2.0 host controller protocol.
    @param[out] Interface   Receives the fake host controller.
    @param[in]  Agent       The opening driver, ignored.
    @param[in]  Controller  The child controller, ignored.
    @param[in]  Attributes  Expected to request a child-controller open.

    @retval EFI_SUCCESS  The fake host controller was returned.
  **/
  static EFI_STATUS EFIAPI
  Open (
    EFI_HANDLE  Handle,
    EFI_GUID    *Protocol,
    VOID        **Interface,
    EFI_HANDLE  Agent,
    EFI_HANDLE  Controller,
    UINT32      Attributes
    )
  {
    (void)Handle;
    (void)Agent;
    (void)Controller;
    EXPECT_EQ (Protocol, &gEfiUsb2HcProtocolGuid);
    EXPECT_EQ (Attributes, static_cast<UINT32> (EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER));
    *Interface = &Active->Host;
    return EFI_SUCCESS;
  }

  /**
    Accept closure of a host-controller protocol relationship without tracking it.

    @param[in] Handle      The controller handle, ignored.
    @param[in] Protocol    The protocol GUID, ignored.
    @param[in] Agent       The opening driver, ignored.
    @param[in] Controller  The child controller, ignored.

    @retval EFI_SUCCESS  The close request was accepted.
  **/
  static EFI_STATUS EFIAPI
  Close (
    EFI_HANDLE  Handle,
    EFI_GUID    *Protocol,
    EFI_HANDLE  Agent,
    EFI_HANDLE  Controller
    )
  {
    (void)Handle;
    (void)Protocol;
    (void)Agent;
    (void)Controller;
    return EFI_SUCCESS;
  }

  /**
    Fail the test if reset attempts to connect function drivers.

    @param[in] Handle     The controller handle, ignored.
    @param[in] Drivers    The driver list, ignored.
    @param[in] Path       The remaining device path, ignored.
    @param[in] Recursive  The recursive connection flag, ignored.

    @retval EFI_UNSUPPORTED  Connection is forbidden by these tests.
  **/
  static EFI_STATUS EFIAPI
  Connect (
    EFI_HANDLE                Handle,
    EFI_HANDLE                *Drivers,
    EFI_DEVICE_PATH_PROTOCOL  *Path,
    BOOLEAN                   Recursive
    )
  {
    (void)Handle;
    (void)Drivers;
    (void)Path;
    (void)Recursive;
    ADD_FAILURE () << "Reset must not connect replacement drivers";
    return EFI_UNSUPPORTED;
  }

  /**
    Allow interface teardown without simulating function drivers or their children.

    @param[in] Handle  The controller handle, ignored.
    @param[in] Driver  The driver handle, ignored.
    @param[in] Child   The child handle, ignored.

    @retval EFI_SUCCESS  The disconnect request was accepted.
  **/
  static EFI_STATUS EFIAPI
  Disconnect (
    EFI_HANDLE  Handle,
    EFI_HANDLE  Driver,
    EFI_HANDLE  Child
    )
  {
    (void)Handle;
    (void)Driver;
    (void)Child;
    return EFI_SUCCESS;
  }

  /**
    Apply the selected interface layout when production code resets the parent port.

    @param[in] Hub   The parent hub interface, ignored.
    @param[in] Port  Expected to identify the test device's port.

    @retval EFI_SUCCESS  The selected descriptors are ready for enumeration.
  **/
  static EFI_STATUS
  ResetPort (
    USB_INTERFACE  *Hub,
    UINT8          Port
    )
  {
    (void)Hub;
    EXPECT_EQ (Port, 2);
    Active->SetInterfaceLayout (Active->NextDfu);
    return EFI_SUCCESS;
  }

  /**
    Accept port-change acknowledgement without tracking hub change bits.

    @param[in] Hub   The parent hub interface, ignored.
    @param[in] Port  The port number, ignored.
  **/
  static VOID
  ClearPortChange (
    USB_INTERFACE  *Hub,
    UINT8          Port
    )
  {
    (void)Hub;
    (void)Port;
  }

  /**
    Serve descriptor reads and accept address and configuration requests.

    @param[in]     This        The host controller protocol, ignored.
    @param[in]     Address     Expected to be zero for address assignment.
    @param[in]     Speed       The device speed, ignored.
    @param[in]     MaxPacket   The endpoint-zero packet size, ignored.
    @param[in]     Request     The USB setup request.
    @param[in]     Direction   The transfer direction, ignored.
    @param[out]    Data        Receives descriptor bytes for descriptor reads.
    @param[in,out] Length      Buffer capacity on entry; bytes copied for descriptor reads.
    @param[in]     Timeout     The transfer timeout, ignored.
    @param[in]     Translator  The transaction translator, ignored.
    @param[out]    Result      Receives the USB transfer result.

    @retval EFI_SUCCESS       A supported request completed.
    @retval EFI_DEVICE_ERROR  An unavailable descriptor was requested; USB status is STALL.
    @retval EFI_UNSUPPORTED   An unexpected request also records a test failure.
  **/
  static EFI_STATUS EFIAPI
  ControlTransfer (
    EFI_USB2_HC_PROTOCOL                *This,
    UINT8                               Address,
    UINT8                               Speed,
    UINTN                               MaxPacket,
    EFI_USB_DEVICE_REQUEST              *Request,
    EFI_USB_DATA_DIRECTION              Direction,
    VOID                                *Data,
    UINTN                               *Length,
    UINTN                               Timeout,
    EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    UINT32                              *Result
    )
  {
    (void)This;
    (void)Speed;
    (void)MaxPacket;
    (void)Direction;
    (void)Timeout;
    (void)Translator;
    *Result = EFI_USB_NOERROR;
    if (Request->Request == USB_REQ_SET_ADDRESS) {
      EXPECT_EQ (Address, 0);
      EXPECT_EQ (Request->Value, 1);
      return EFI_SUCCESS;
    }

    if (Request->Request == USB_REQ_SET_CONFIG) {
      EXPECT_EQ (Request->Value, 1);
      return EFI_SUCCESS;
    }

    if (Request->Request == USB_REQ_GET_DESCRIPTOR) {
      const UINT8  Type    = static_cast<UINT8> (Request->Value >> 8);
      const VOID   *Source = nullptr;
      UINTN        Size    = 0;
      if (Type == USB_DESC_TYPE_DEVICE) {
        Source = &Active->Descriptor;
        Size   = sizeof (Active->Descriptor);
      } else if (Type == USB_DESC_TYPE_CONFIG) {
        Source = Active->Configuration.data ();
        Size   = Active->Configuration.size ();
      } else {
        // No strings are advertised by this device.
        *Result = EFI_USB_ERR_STALL;
        return EFI_DEVICE_ERROR;
      }

      *Length = std::min (*Length, Size);
      CopyMem (Data, Source, *Length);
      return EFI_SUCCESS;
    }

    ADD_FAILURE () << "Unexpected USB request " << static_cast<unsigned> (Request->Request);
    return EFI_UNSUPPORTED;
  }
};

UsbDfuResetTest  *UsbDfuResetTest::Active = nullptr;

/**
  Reset a device that exposes four runtime interfaces without changing its mode.
  Expect the same four handles and USB I/O pointers, with no protocol removals
  or new installations.
**/
TEST_F (UsbDfuResetTest, ResetPreservesFourInterfaces) {
  ASSERT_NO_FATAL_FAILURE (StartDevice (false));
  const auto                          OriginalIds = PublishedIds ();
  std::vector<EFI_HANDLE>             Handles;
  std::vector<EFI_USB_IO_PROTOCOL *>  Protocols;

  for (const auto &Interface : Published) {
    Handles.push_back (Interface.get ());
    Protocols.push_back (Interface->UsbIo);
  }

  ASSERT_EQ (Reset (false), EFI_SUCCESS);
  ASSERT_NO_FATAL_FAILURE (ExpectInterfaceLayout (false));
  EXPECT_EQ (PublishedIds (), OriginalIds);
  EXPECT_EQ (InstallCount, 4U);
  EXPECT_TRUE (Removed.empty ());
  for (UINTN Index = 0; Index < Published.size (); ++Index) {
    EXPECT_EQ (Published[Index].get (), Handles[Index]);
    EXPECT_EQ (Published[Index]->UsbIo, Protocols[Index]);
  }
}

/**
  Reset a device from four runtime interfaces into single-interface DFU mode.
  Expect all four original installations to be removed and exactly one new
  USB I/O interface exposing DFU descriptors to be available.
**/
TEST_F (UsbDfuResetTest, ResetEntersDfu) {
  ASSERT_NO_FATAL_FAILURE (StartDevice (false));
  const auto  OriginalIds = PublishedIds ();

  ASSERT_EQ (Reset (true), EFI_SUCCESS);
  ASSERT_NO_FATAL_FAILURE (ExpectInterfaceLayout (true));
  EXPECT_EQ (Removed, OriginalIds);
  EXPECT_EQ (InstallCount, 5U);
  EXPECT_GT (Published[0]->Id, OriginalIds.back ());
}

/**
  Start with one DFU interface and reset the device into runtime mode.
  Expect the original DFU installation to be removed and four new USB I/O
  interfaces exposing runtime descriptors to be available.
**/
TEST_F (UsbDfuResetTest, ResetExitsDfu) {
  ASSERT_NO_FATAL_FAILURE (StartDevice (true));
  const auto  OriginalIds = PublishedIds ();

  ASSERT_EQ (Reset (false), EFI_SUCCESS);
  ASSERT_NO_FATAL_FAILURE (ExpectInterfaceLayout (false));
  EXPECT_EQ (Removed, OriginalIds);
  EXPECT_EQ (InstallCount, 5U);
  for (const auto &Interface : Published) {
    EXPECT_GT (Interface->Id, OriginalIds.back ());
  }
}
} // namespace

/**
  Initialize Google Test and run the USB reset scenarios.

  @param[in] Argc  The command-line argument count.
  @param[in] Argv  The command-line arguments.

  @return Zero if all tests pass; nonzero otherwise.
**/
int
main (
  int   Argc,
  char  *Argv[]
  )
{
  testing::InitGoogleTest (&Argc, Argv);
  return RUN_ALL_TESTS ();
}
