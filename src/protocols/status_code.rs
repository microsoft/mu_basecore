//! Status Code Protocol
//!
//! Provides the service required to report a status code to the platform firmware.
//!
//! See <https://uefi.org/specs/PI/1.8A/V2_DXE_Runtime_Protocols.html#efi-status-code-protocol>
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

use r_efi::efi;

pub const PROTOCOL_GUID: efi::Guid =
    efi::Guid::from_fields(0xD2B2B828, 0x0826, 0x48A7, 0xB3, 0xDF, &[0x98, 0x3C, 0x00, 0x60, 0x24, 0xF0]);

///
/// Definition of code types. All other values masked by
/// EFI_STATUS_CODE_TYPE_MASK are reserved for use by
/// this specification.
///
pub const EFI_PROGRESS_CODE: u32 = 0x00000001;
pub const EFI_ERROR_CODE: u32 = 0x00000002;
pub const EFI_DEBUG_CODE: u32 = 0x00000003;

///
/// Class definitions.
/// Values of 4-127 are reserved for future use by this specification.
/// Values in the range 127-255 are reserved for OEM use.
///
pub const EFI_COMPUTING_UNIT: u32 = 0x00000000;
pub const EFI_PERIPHERAL: u32 = 0x01000000;
pub const EFI_IO_BUS: u32 = 0x02000000;
pub const EFI_SOFTWARE: u32 = 0x03000000;

///
/// Software Subclass definitions.
/// Values of 14-127 are reserved for future use by this specification.
/// Values of 128-255 are reserved for OEM use.
///
pub const EFI_SOFTWARE_UNSPECIFIED: u32 = EFI_SOFTWARE;
pub const EFI_SOFTWARE_SEC: u32 = EFI_SOFTWARE | 0x00010000;
pub const EFI_SOFTWARE_PEI_CORE: u32 = EFI_SOFTWARE | 0x00020000;
pub const EFI_SOFTWARE_PEI_MODULE: u32 = EFI_SOFTWARE | 0x00030000;
pub const EFI_SOFTWARE_DXE_CORE: u32 = EFI_SOFTWARE | 0x00040000;
pub const EFI_SOFTWARE_DXE_BS_DRIVER: u32 = EFI_SOFTWARE | 0x00050000;
pub const EFI_SOFTWARE_DXE_RT_DRIVER: u32 = EFI_SOFTWARE | 0x00060000;
pub const EFI_SOFTWARE_SMM_DRIVER: u32 = EFI_SOFTWARE | 0x00070000;
pub const EFI_SOFTWARE_EFI_APPLICATION: u32 = EFI_SOFTWARE | 0x00080000;
pub const EFI_SOFTWARE_EFI_OS_LOADER: u32 = EFI_SOFTWARE | 0x00090000;
pub const EFI_SOFTWARE_RT: u32 = EFI_SOFTWARE | 0x000A0000;
pub const EFI_SOFTWARE_AL: u32 = EFI_SOFTWARE | 0x000B0000;
pub const EFI_SOFTWARE_EBC_EXCEPTION: u32 = EFI_SOFTWARE | 0x000C0000;
pub const EFI_SOFTWARE_IA32_EXCEPTION: u32 = EFI_SOFTWARE | 0x000D0000;
pub const EFI_SOFTWARE_IPF_EXCEPTION: u32 = EFI_SOFTWARE | 0x000E0000;
pub const EFI_SOFTWARE_PEI_SERVICE: u32 = EFI_SOFTWARE | 0x000F0000;
pub const EFI_SOFTWARE_EFI_BOOT_SERVICE: u32 = EFI_SOFTWARE | 0x00100000;
pub const EFI_SOFTWARE_EFI_RUNTIME_SERVICE: u32 = EFI_SOFTWARE | 0x00110000;
pub const EFI_SOFTWARE_EFI_DXE_SERVICE: u32 = EFI_SOFTWARE | 0x00120000;
pub const EFI_SOFTWARE_X64_EXCEPTION: u32 = EFI_SOFTWARE | 0x00130000;
pub const EFI_SOFTWARE_ARM_EXCEPTION: u32 = EFI_SOFTWARE | 0x00140000;

///
/// Debug Code definitions for all classes and subclass.
/// Only one debug code is defined at this point and should
/// be used for anything that is sent to the debug stream.
///
pub const EFI_DC_UNSPECIFIED: u32 = 0x0;

///
/// General partitioning scheme for Progress and Error Codes are:
///   - 0x0000-0x0FFF    Shared by all sub-classes in a given class.
///   - 0x1000-0x7FFF    Subclass Specific.
///   - 0x8000-0xFFFF    OEM specific.
pub const EFI_SUBCLASS_SPECIFIC: u32 = 0x1000;
pub const EFI_OEM_SPECIFIC: u32 = 0x8000;

pub const EFI_SW_BS_PC_RAISE_TPL: u32 = EFI_SUBCLASS_SPECIFIC;
pub const EFI_SW_BS_PC_RESTORE_TPL: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000001;
pub const EFI_SW_BS_PC_ALLOCATE_PAGES: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000002;
pub const EFI_SW_BS_PC_FREE_PAGES: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000003;
pub const EFI_SW_BS_PC_GET_MEMORY_MAP: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000004;
pub const EFI_SW_BS_PC_ALLOCATE_POOL: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000005;
pub const EFI_SW_BS_PC_FREE_POOL: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000006;
pub const EFI_SW_BS_PC_CREATE_EVENT: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000007;
pub const EFI_SW_BS_PC_SET_TIMER: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000008;
pub const EFI_SW_BS_PC_WAIT_FOR_EVENT: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000009;
pub const EFI_SW_BS_PC_SIGNAL_EVENT: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000000A;
pub const EFI_SW_BS_PC_CLOSE_EVENT: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000000B;
pub const EFI_SW_BS_PC_CHECK_EVENT: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000000C;
pub const EFI_SW_BS_PC_INSTALL_PROTOCOL_INTERFACE: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000000D;
pub const EFI_SW_BS_PC_REINSTALL_PROTOCOL_INTERFACE: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000000E;
pub const EFI_SW_BS_PC_UNINSTALL_PROTOCOL_INTERFACE: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000000F;
pub const EFI_SW_BS_PC_HANDLE_PROTOCOL: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000010;
pub const EFI_SW_BS_PC_PC_HANDLE_PROTOCOL: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000011;
pub const EFI_SW_BS_PC_REGISTER_PROTOCOL_NOTIFY: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000012;
pub const EFI_SW_BS_PC_LOCATE_HANDLE: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000013;
pub const EFI_SW_BS_PC_INSTALL_CONFIGURATION_TABLE: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000014;
pub const EFI_SW_BS_PC_LOAD_IMAGE: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000015;
pub const EFI_SW_BS_PC_START_IMAGE: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000016;
pub const EFI_SW_BS_PC_EXIT: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000017;
pub const EFI_SW_BS_PC_UNLOAD_IMAGE: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000018;
pub const EFI_SW_BS_PC_EXIT_BOOT_SERVICES: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000019;
pub const EFI_SW_BS_PC_GET_NEXT_MONOTONIC_COUNT: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000001A;
pub const EFI_SW_BS_PC_STALL: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000001B;
pub const EFI_SW_BS_PC_SET_WATCHDOG_TIMER: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000001C;
pub const EFI_SW_BS_PC_CONNECT_CONTROLLER: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000001D;
pub const EFI_SW_BS_PC_DISCONNECT_CONTROLLER: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000001E;
pub const EFI_SW_BS_PC_OPEN_PROTOCOL: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000001F;
pub const EFI_SW_BS_PC_CLOSE_PROTOCOL: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000020;
pub const EFI_SW_BS_PC_OPEN_PROTOCOL_INFORMATION: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000021;
pub const EFI_SW_BS_PC_PROTOCOLS_PER_HANDLE: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000022;
pub const EFI_SW_BS_PC_LOCATE_HANDLE_BUFFER: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000023;
pub const EFI_SW_BS_PC_LOCATE_PROTOCOL: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000024;
pub const EFI_SW_BS_PC_INSTALL_MULTIPLE_INTERFACES: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000025;
pub const EFI_SW_BS_PC_UNINSTALL_MULTIPLE_INTERFACES: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000026;
pub const EFI_SW_BS_PC_CALCULATE_CRC_32: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000027;
pub const EFI_SW_BS_PC_COPY_MEM: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000028;
pub const EFI_SW_BS_PC_SET_MEM: u32 = EFI_SUBCLASS_SPECIFIC | 0x00000029;
pub const EFI_SW_BS_PC_CREATE_EVENT_EX: u32 = EFI_SUBCLASS_SPECIFIC | 0x0000002A;

/// The definition of the status code extended data header. The data will follow HeaderSize bytes from the
/// beginning of the structure and is Size bytes long.
///
/// # Documentation
/// UEFI Platform Initialization Specification, Release 1.8, Section III-6.6.2.1
#[repr(C)]
pub struct EfiStatusCodeData {
    pub header_size: u16,
    pub size: u16,
    pub r#type: efi::Guid,
}

/// Provides an interface that a software module can call to report a status code.
///
/// # Documentation
/// UEFI Platform Initialization Specification, Release 1.8, Section II-14.2.1
pub type ReportStatusCode =
    extern "efiapi" fn(u32, u32, u32, *const efi::Guid, *const EfiStatusCodeData) -> efi::Status;

/// Provides the service required to report a status code to the platform firmware.
/// This protocol must be produced by a runtime DXE driver.
///
/// # Documentation
/// UEFI Platform Initialization Specification, Release 1.8, Section II-14.2.1
#[repr(C)]
pub struct Protocol {
    pub report_status_code: ReportStatusCode,
}
