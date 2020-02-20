//! UEFI System Integration
//!
//! This header defines the structures and types of the surrounding system of an UEFI application.
//! It contains the definitions of the system table, the runtime and boot services, as well as
//! common types.
//!
//! We do not document the behavior of each of these types and functions. They follow the UEFI
//! specification, which does a well-enough job of documenting each. This file just provides you
//! the rust definitions of each symbol and some limited hints on some pecularities.

//
// Time Management
//
// UEFI time management is modeled around the EFI_TIME structure, which represents any arbitrary
// timestamp. The runtime and boot services provide helper functions to query and set the system
// time.
//

pub const TIME_ADJUST_DAYLIGHT: u8 = 0x01u8;
pub const TIME_IN_DAYLIGHT: u8 = 0x02u8;

pub const UNSPECIFIED_TIMEZONE: i16 = 0x07ffi16;

#[repr(C)]
#[derive(Debug)]
pub struct Time {
    pub year: u16,
    pub month: u8,
    pub day: u8,
    pub hour: u8,
    pub minute: u8,
    pub second: u8,
    pub pad1: u8,
    pub nanosecond: u32,
    pub timezone: i16,
    pub daylight: u8,
    pub pad2: u8,
}

#[repr(C)]
#[derive(Debug)]
pub struct TimeCapabilities {
    pub resolution: u32,
    pub accuracy: u32,
    pub sets_to_zero: crate::base::Boolean,
}

//
// UEFI Variables
//
// UEFI systems provide a way to store global variables. These can be persistent or volatile. The
// variable store must be provided by the platform, but persistent storage might not be available.
//

pub const VARIABLE_NON_VOLATILE:                            u32 = 0x00000001u32;
pub const VARIABLE_BOOTSERVICE_ACCESS:                      u32 = 0x00000002u32;
pub const VARIABLE_RUNTIME_ACCESS:                          u32 = 0x00000004u32;
pub const VARIABLE_HARDWARE_ERROR_RECORD:                   u32 = 0x00000008u32;
pub const VARIABLE_AUTHENTICATED_WRITE_ACCESS:              u32 = 0x00000010u32;
pub const VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS:   u32 = 0x00000020u32;
pub const VARIABLE_APPEND_WRITE:                            u32 = 0x00000040u32;
pub const VARIABLE_ENHANCED_AUTHENTICATED_ACCESS:           u32 = 0x00000080u32;

pub const VARIABLE_AUTHENTICATION_3_CERT_ID_SHA256: u32 = 0x1u32;

#[repr(C)]
#[derive(Debug)]
pub struct VariableAuthentication3CertId {
    pub r#type: u8,
    pub id_size: u32,
    pub id: [u8],
}

#[repr(C)]
#[derive(Debug)]
pub struct VariableAuthentication {
    pub monotonic_count: u64,
    pub auth_info: [u8], // WIN_CERTIFICATE_UEFI_ID from PE/COFF
}

#[repr(C)]
#[derive(Debug)]
pub struct VariableAuthentication2 {
    pub timestamp: Time,
    pub auth_info: [u8], // WIN_CERTIFICATE_UEFI_ID from PE/COFF
}

pub const VARIABLE_AUTHENTICATION_3_TIMESTAMP_TYPE:     u32 = 0x1u32;
pub const VARIABLE_AUTHENTICATION_3_NONCE_TYPE:         u32 = 0x2u32;

#[repr(C)]
#[derive(Debug)]
pub struct VariableAuthentication3 {
    pub version: u8,
    pub r#type: u8,
    pub metadata_size: u32,
    pub flags: u32,
}

#[repr(C)]
#[derive(Debug)]
pub struct VariableAuthentication3Nonce {
    pub nonce_size: u32,
    pub nonce: [u8],
}

pub const HARDWARE_ERROR_VARIABLE_GUID: crate::base::Guid = crate::base::Guid::from_fields(
    0x414E6BDD, 0xE47B, 0x47cc, 0xB2, 0x44, &[0xBB, 0x61, 0x02, 0x0C, 0xF5, 0x16]
);

//
// Virtual Mappings
//
// UEFI runs in an 1-to-1 mapping from virtual to physical addresses. But once you exit boot
// services, you can apply any address mapping you want, as long as you inform UEFI about it (or,
// alternatively, stop using the UEFI runtime services).
//

pub const OPTIONAL_POINTER: u32 = 0x00000001u32;

//
// System Reset
//
// UEFI provides access to firmware functions to reset the system. This includes a wide variety of
// different possible resets.
//

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub enum ResetType {
    ResetCold,
    ResetWarm,
    ResetShutdown,
    ResetPlatformSpecific,
}

//
// Update Capsules
//
// The process of firmware updates is generalized in UEFI. There are small blobs called capsules
// that you can push into the firmware to be run either immediately or on next reboot.
//

#[repr(C)]
pub union CapsuleBlockDescriptorUnion {
    pub data_block: crate::base::PhysicalAddress,
    pub continuation_pointer: crate::base::PhysicalAddress,
}

#[repr(C)]
pub struct CapsuleBlockDescriptor {
    pub length: u64,
    pub data: CapsuleBlockDescriptorUnion,
}

pub const CAPSULE_FLAGS_PERSIST_ACROSS_RESET:   u32 = 0x00010000u32;
pub const CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE:  u32 = 0x00020000u32;
pub const CAPSULE_FLAGS_INITIATE_RESET:         u32 = 0x00040000u32;

#[repr(C)]
#[derive(Debug)]
pub struct CapsuleHeader {
    pub capsule_guid: crate::base::Guid,
    pub header_size: u32,
    pub flags: u32,
    pub capsule_image_size: u32,
}

pub const OS_INDICATIONS_BOOT_TO_FW_UI:                     u64 = 0x0000000000000001u64;
pub const OS_INDICATIONS_TIMESTAMP_REVOCATION:              u64 = 0x0000000000000002u64;
pub const OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED:   u64 = 0x0000000000000004u64;
pub const OS_INDICATIONS_FMP_CAPSULE_SUPPORTED:             u64 = 0x0000000000000008u64;
pub const OS_INDICATIONS_CAPSULE_RESULT_VAR_SUPPORTED:      u64 = 0x0000000000000010u64;
pub const OS_INDICATIONS_START_OS_RECOVERY:                 u64 = 0x0000000000000020u64;
pub const OS_INDICATIONS_START_PLATFORM_RECOVERY:           u64 = 0x0000000000000040u64;

pub const CAPSULE_REPORT_GUID: crate::base::Guid = crate::base::Guid::from_fields(
    0x39b68c46, 0xf7fb, 0x441b, 0xb6, 0xec, &[0x16, 0xb0, 0xf6, 0x98, 0x21, 0xf3]
);

#[repr(C)]
#[derive(Debug)]
pub struct CapsuleResultVariableHeader {
    pub variable_total_size: u32,
    pub reserved: u32,
    pub capsule_guid: crate::base::Guid,
    pub capsule_processed: Time,
    pub capsule_status: crate::base::Status,
}

#[repr(C)]
#[derive(Debug)]
pub struct CapsuleResultVariableFMP {
    pub version: u16,
    pub payload_index: u8,
    pub update_image_index: u8,
    pub update_image_type_id: crate::base::Guid,
    pub capsule_file_name_and_target: [crate::base::Char16],
}

//
// Tasks
//
// UEFI uses a simplified task model, and only ever runs on a single CPU. Usually, there is only
// one single task running on the system, which is the current execution. No interrupts are
// supported, other than timer interrupts. That is, all device management must be reliant on
// polling.
//
// You can, however, register callbacks to be run by the UEFI core. That is, either when execution
// is returned to the UEFI core, or when a timer interrupt fires, the scheduler will run the
// highest priority task next, interrupting the current task. You can use simple
// task-priority-levels (TPL) to adjust the priority of your callbacks and current task.
//

pub const EVT_TIMER:                            u32 = 0x80000000u32;
pub const EVT_RUNTIME:                          u32 = 0x40000000u32;
pub const EVT_NOTIFY_WAIT:                      u32 = 0x00000100u32;
pub const EVT_NOTIFY_SIGNAL:                    u32 = 0x00000200u32;
pub const EVT_SIGNAL_EXIT_BOOT_SERVICES:        u32 = 0x00000201u32;
pub const EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE:    u32 = 0x60000202u32;

pub type EventNotify = eficall!{fn(crate::base::Event, *mut core::ffi::c_void)};

pub const EVENT_GROUP_EXIT_BOOT_SERVICES: crate::base::Guid = crate::base::Guid::from_fields(
    0x27abf055, 0xb1b8, 0x4c26, 0x80, 0x48, &[0x74, 0x8f, 0x37, 0xba, 0xa2, 0xdf]
);
pub const EVENT_GROUP_VIRTUAL_ADDRESS_CHANGE: crate::base::Guid = crate::base::Guid::from_fields(
    0x13fa7698, 0xc831, 0x49c7, 0x87, 0xea, &[0x8f, 0x43, 0xfc, 0xc2, 0x51, 0x96]
);
pub const EVENT_GROUP_MEMORY_MAP_CHANGE: crate::base::Guid = crate::base::Guid::from_fields(
    0x78bee926, 0x692f, 0x48fd, 0x9e, 0xdb, &[0x1, 0x42, 0x2e, 0xf0, 0xd7, 0xab]
);
pub const EVENT_GROUP_READY_TO_BOOT: crate::base::Guid = crate::base::Guid::from_fields(
    0x7ce88fb3, 0x4bd7, 0x4679, 0x87, 0xa8, &[0xa8, 0xd8, 0xde, 0xe5, 0x0d, 0x2b]
);
pub const EVENT_GROUP_RESET_SYSTEM: crate::base::Guid = crate::base::Guid::from_fields(
    0x62da6a56, 0x13fb, 0x485a, 0xa8, 0xda, &[0xa3, 0xdd, 0x79, 0x12, 0xcb, 0x6b]
);

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub enum TimerDelay {
    TimerCancel,
    TimerPeriodic,
    TimerRelative,
}

pub const TPL_APPLICATION:      crate::base::Tpl = 4;
pub const TPL_CALLBACK:         crate::base::Tpl = 8;
pub const TPL_NOTIFY:           crate::base::Tpl = 16;
pub const TPL_HIGH_LEVEL:       crate::base::Tpl = 31;

//
// Memory management
//
// The UEFI boot services provide you pool-allocation helpers to reserve memory. The region for
// each allocation can be selected by the caller, allowing to reserve memory that even survives
// beyond boot services. However, dynamic allocations can only performed via boot services, so no
// dynamic modifications can be done once you exit boot services.
//

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub enum AllocateType {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub enum MemoryType {
    ReservedMemoryType,
    LoaderCode,
    LoaderData,
    BootServicesCode,
    BootServicesData,
    RuntimeServicesCode,
    RuntimeServicesData,
    ConventionalMemory,
    UnusableMemory,
    AcpiReclaimMemory,
    AcpiMemoryNvs,
    MemoryMappedIO,
    MemoryMappedIOPortSpace,
    PalCode,
    PersistentMemory,
}

pub const MEMORY_UC:                u64 = 0x0000000000000001u64;
pub const MEMORY_WC:                u64 = 0x0000000000000002u64;
pub const MEMORY_WT:                u64 = 0x0000000000000004u64;
pub const MEMORY_WB:                u64 = 0x0000000000000008u64;
pub const MEMORY_UCE:               u64 = 0x0000000000000010u64;
pub const MEMORY_WP:                u64 = 0x0000000000001000u64;
pub const MEMORY_RP:                u64 = 0x0000000000002000u64;
pub const MEMORY_XP:                u64 = 0x0000000000004000u64;
pub const MEMORY_NV:                u64 = 0x0000000000008000u64;
pub const MEMORY_MORE_RELIABLE:     u64 = 0x0000000000010000u64;
pub const MEMORY_RO:                u64 = 0x0000000000020000u64;
pub const MEMORY_RUNTIME:           u64 = 0x8000000000000000u64;

pub const MEMORY_DESCRIPTOR_VERSION: u32 = 0x00000001u32;

#[repr(C)]
#[derive(Debug)]
pub struct MemoryDescriptor {
    pub r#type: u32,
    pub physical_start: crate::base::PhysicalAddress,
    pub virtual_start: crate::base::VirtualAddress,
    pub number_of_pages: u64,
    pub attribute: u64,
}

//
// Protocol Management
//
// The UEFI driver model provides ways to have bus-drivers, device-drivers, and applications as
// separate, independent entities. They use protocols to communicate, and handles to refer to
// common state. Drivers and devices can be registered dynamically at runtime, and can support
// hotplugging.
//

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub enum InterfaceType {
    NativeInterface,
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub enum LocateSearchType {
    AllHandles,
    ByRegisterNotify,
    ByProtocol,
}

pub const OPEN_PROTOCOL_BY_HANDLE_PROTOCOL:     u32 = 0x00000001u32;
pub const OPEN_PROTOCOL_GET_PROTOCOL:           u32 = 0x00000002u32;
pub const OPEN_PROTOCOL_TEST_PROTOCOL:          u32 = 0x00000004u32;
pub const OPEN_PROTOCOL_BY_CHILD_CONTROLLER:    u32 = 0x00000008u32;
pub const OPEN_PROTOCOL_BY_DRIVER:              u32 = 0x00000010u32;
pub const OPEN_PROTOCOL_EXCLUSIVE:              u32 = 0x00000020u32;

#[repr(C)]
#[derive(Debug)]
pub struct OpenProtocolInformationEntry {
    pub agent_handle: crate::base::Handle,
    pub controller_handle: crate::base::Handle,
    pub attributes: u32,
    pub open_count: u32,
}

//
// Configuration Tables
//
// The system table contains an array of auxiliary tables, indexed by their GUID, called
// configuration tables. Each table uses the generic ConfigurationTable structure as header.
//

#[repr(C)]
#[derive(Debug)]
pub struct ConfigurationTable {
    pub vendor_guid: crate::base::Guid,
    pub vendor_table: *mut core::ffi::c_void,
}

pub const PROPERTIES_TABLE_GUID: crate::base::Guid = crate::base::Guid::from_fields(
    0x880aaca3, 0x4adc, 0x4a04, 0x90, 0x79, &[0xb7, 0x47, 0x34, 0x8, 0x25, 0xe5]
);

pub const PROPERTIES_TABLE_VERSION: u32 = 0x00010000u32;

pub const PROPERTIES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA: u64 = 0x1u64;

#[repr(C)]
#[derive(Debug)]
pub struct PropertiesTable {
    pub version: u32,
    pub length: u32,
    pub memory_protection_attribute: u64,
}

pub const MEMORY_ATTRIBUTES_TABLE_GUID: crate::base::Guid = crate::base::Guid::from_fields(
    0xdcfa911d, 0x26eb, 0x469f, 0xa2, 0x20, &[ 0x38, 0xb7, 0xdc, 0x46, 0x12, 0x20]
);

pub const MEMORY_ATTRIBUTES_TABLE_VERSION: u32 = 0x00000001u32;

#[repr(C)]
#[derive(Debug)]
pub struct MemoryAttributesTable {
    pub version: u32,
    pub number_of_entries: u32,
    pub descriptor_size: u32,
    pub reserved: u32,
    pub entry: [MemoryDescriptor],
}

//
// Global Tables
//
// UEFI uses no global state, so all access to UEFI internal state is done through vtables you get
// passed to your entry-point. The global entry is the system-table, which encorporates several
// sub-tables, including the runtime and boot service tables, and configuration tables (including
// vendor extensions).
//

pub const SPECIFICATION_REVISION: u32 = SYSTEM_TABLE_REVISION;

#[repr(C)]
#[derive(Debug)]
pub struct TableHeader {
    pub signature: u64,
    pub revision: u32,
    pub header_size: u32,
    pub crc32: u32,
    pub reserved: u32,
}

pub const RUNTIME_SERVICES_SIGNATURE: u64 = 0x56524553544e5552u64; // "RUNTSERV"
pub const RUNTIME_SERVICES_REVISION: u32 = SPECIFICATION_REVISION;

#[repr(C)]
pub struct RuntimeServices {
    pub hdr: TableHeader,

    pub get_time: eficall!{fn(
        *mut Time,
        *mut TimeCapabilities,
    ) -> crate::base::Status},
    pub set_time: eficall!{fn(
        *mut Time,
    ) -> crate::base::Status},
    pub get_wakeup_time: eficall!{fn(
        *mut crate::base::Boolean,
        *mut crate::base::Boolean,
        *mut Time,
    ) -> crate::base::Status},
    pub set_wakeup_time: eficall!{fn(
        crate::base::Boolean,
        *mut Time,
    ) -> crate::base::Status},

    pub set_virtual_address_map: eficall!{fn(
        usize,
        usize,
        u32,
        *mut MemoryDescriptor,
    ) -> crate::base::Status},
    pub convert_pointer: eficall!{fn(
        usize,
        *mut *mut core::ffi::c_void,
    ) -> crate::base::Status},

    pub get_variable: eficall!{fn(
        *mut crate::base::Char16,
        *mut crate::base::Guid,
        *mut u32,
        *mut usize,
        *mut core::ffi::c_void,
    ) -> crate::base::Status},
    pub get_next_variable_name: eficall!{fn(
        *mut usize,
        *mut crate::base::Char16,
        *mut crate::base::Guid,
    ) -> crate::base::Status},
    pub set_variable: eficall!{fn(
        *mut crate::base::Char16,
        *mut crate::base::Guid,
        u32,
        usize,
        *mut core::ffi::c_void,
    ) -> crate::base::Status},

    pub get_next_high_mono_count: eficall!{fn(
        *mut u32,
    ) -> crate::base::Status},
    pub reset_system: eficall!{fn(
        ResetType,
        crate::base::Status,
        usize,
        *mut core::ffi::c_void,
    )},

    pub update_capsule: eficall!{fn(
        *mut *mut CapsuleHeader,
        usize,
        crate::base::PhysicalAddress,
    ) -> crate::base::Status},
    pub query_capsule_capabilities: eficall!{fn(
        *mut *mut CapsuleHeader,
        usize,
        *mut u64,
        *mut ResetType,
    ) -> crate::base::Status},
    pub query_variable_info: eficall!{fn(
        u32,
        *mut u64,
        *mut u64,
        *mut u64,
    ) -> crate::base::Status},
}

pub const BOOT_SERVICES_SIGNATURE: u64 = 0x56524553544f4f42u64; // "BOOTSERV"
pub const BOOT_SERVICES_REVISION: u32 = SPECIFICATION_REVISION;

#[repr(C)]
pub struct BootServices {
    pub hdr: TableHeader,

    pub raise_tpl: eficall!{fn(
        crate::base::Tpl,
    ) -> crate::base::Tpl},
    pub restore_tpl: eficall!{fn(
        crate::base::Tpl,
    )},

    pub allocate_pages: eficall!{fn(
        AllocateType,
        MemoryType,
        usize,
        *mut crate::base::PhysicalAddress,
    ) -> crate::base::Status},
    pub free_pages: eficall!{fn(
        crate::base::PhysicalAddress,
        usize,
    ) -> crate::base::Status},
    pub get_memory_map: eficall!{fn(
        *mut usize,
        *mut MemoryDescriptor,
        *mut usize,
        *mut usize,
        *mut u32,
    ) -> crate::base::Status},
    pub allocate_pool: eficall!{fn(
        MemoryType,
        usize,
        *mut *mut core::ffi::c_void,
    ) -> crate::base::Status},
    pub free_pool: eficall!{fn(
        *mut core::ffi::c_void,
    ) -> crate::base::Status},

    pub create_event: eficall!{fn(
        u32,
        crate::base::Tpl,
        EventNotify,
        *mut core::ffi::c_void,
        *mut crate::base::Event,
    ) -> crate::base::Status},
    pub set_timer: eficall!{fn(
        crate::base::Event,
        TimerDelay,
        u64,
    ) -> crate::base::Status},
    pub wait_for_event: eficall!{fn(
        usize,
        *mut crate::base::Event,
        *mut usize,
    ) -> crate::base::Status},
    pub signal_event: eficall!{fn(
        crate::base::Event,
    ) -> crate::base::Status},
    pub close_event: eficall!{fn(
        crate::base::Event,
    ) -> crate::base::Status},
    pub check_event: eficall!{fn(
        crate::base::Event,
    ) -> crate::base::Status},

    pub install_protocol_interface: eficall!{fn(
        *mut crate::base::Handle,
        *mut crate::base::Guid,
        InterfaceType,
        *mut core::ffi::c_void,
    ) -> crate::base::Status},
    pub reinstall_protocol_interface: eficall!{fn(
        crate::base::Handle,
        *mut crate::base::Guid,
        *mut core::ffi::c_void,
        *mut core::ffi::c_void,
    ) -> crate::base::Status},
    pub uninstall_protocol_interface: eficall!{fn(
        crate::base::Handle,
        *mut crate::base::Guid,
        *mut core::ffi::c_void,
    ) -> crate::base::Status},
    pub handle_protocol: eficall!{fn(
        crate::base::Handle,
        *mut crate::base::Guid,
        *mut *mut core::ffi::c_void,
    ) -> crate::base::Status},
    pub reserved: *mut core::ffi::c_void,
    pub register_protocol_notify: eficall!{fn(
        *mut crate::base::Guid,
        crate::base::Event,
        *mut *mut core::ffi::c_void,
    ) -> crate::base::Status},
    pub locate_handle: eficall!{fn(
        LocateSearchType,
        *mut crate::base::Guid,
        *mut core::ffi::c_void,
        *mut usize,
        *mut crate::base::Handle,
    ) -> crate::base::Status},
    pub locate_device_path: eficall!{fn(
        *mut crate::base::Guid,
        *mut *mut core::ffi::c_void, // XXX
    ) -> crate::base::Status},

    pub install_configuration_table: eficall!{fn(
        *mut crate::base::Guid,
        *mut core::ffi::c_void,
    ) -> crate::base::Status},

    pub load_image: eficall!{fn(
        crate::base::Boolean,
        crate::base::Handle,
        *mut core::ffi::c_void, // XXX
        *mut core::ffi::c_void,
        usize,
        *mut crate::base::Handle,
    ) -> crate::base::Status},
    pub start_image: eficall!{fn(
        crate::base::Handle,
        *mut usize,
        *mut *mut crate::base::Char16,
    ) -> crate::base::Status},
    pub exit: eficall!{fn(
        crate::base::Handle,
        crate::base::Status,
        usize,
        *mut crate::base::Char16,
    ) -> crate::base::Status},
    pub unload_image: eficall!{fn(
        crate::base::Handle,
    ) -> crate::base::Status},
    pub exit_boot_services: eficall!{fn(
        crate::base::Handle,
        usize,
    ) -> crate::base::Status},

    pub get_next_monotonic_count: eficall!{fn(
        *mut u64,
    ) -> crate::base::Status},
    pub stall: eficall!{fn(
        usize,
    ) -> crate::base::Status},
    pub set_watchdog_timer: eficall!{fn(
        usize,
        u64,
        usize,
        *mut crate::base::Char16,
    ) -> crate::base::Status},

    // 1.1+

    pub connect_controller: eficall!{fn(
        crate::base::Handle,
        *mut crate::base::Handle,
        *mut core::ffi::c_void, // XXX
        crate::base::Boolean,
    ) -> crate::base::Status},
    pub disconnect_controller: eficall!{fn(
        crate::base::Handle,
        crate::base::Handle,
        crate::base::Handle,
    ) -> crate::base::Status},

    pub open_protocol: eficall!{fn(
        crate::base::Handle,
        *mut crate::base::Guid,
        *mut *mut core::ffi::c_void,
        crate::base::Handle,
        crate::base::Handle,
        u32,
    ) -> crate::base::Status},
    pub close_protocol: eficall!{fn(
        crate::base::Handle,
        *mut crate::base::Guid,
        crate::base::Handle,
        crate::base::Handle,
    ) -> crate::base::Status},
    pub open_protocol_information: eficall!{fn(
        crate::base::Handle,
        *mut crate::base::Guid,
        *mut *mut OpenProtocolInformationEntry,
        *mut usize,
    ) -> crate::base::Status},

    pub protocols_per_handle: eficall!{fn(
        crate::base::Handle,
        *mut *mut *mut crate::base::Guid,
        *mut usize,
    ) -> crate::base::Status},
    pub locate_handle_buffer: eficall!{fn(
        LocateSearchType,
        *mut crate::base::Guid,
        *mut core::ffi::c_void,
        *mut usize,
        *mut *mut crate::base::Handle,
    ) -> crate::base::Status},
    pub locate_protocol: eficall!{fn(
        *mut crate::base::Guid,
        *mut core::ffi::c_void,
        *mut *mut core::ffi::c_void,
    ) -> crate::base::Status},
    pub install_multiple_protocol_interfaces: eficall!{fn(
        *mut crate::base::Handle,
        // XXX: Actual definition is variadic. See eficall!{} for details.
        *mut core::ffi::c_void,
        *mut core::ffi::c_void,
    ) -> crate::base::Status},
    pub uninstall_multiple_protocol_interfaces: eficall!{fn(
        *mut crate::base::Handle,
        // XXX: Actual definition is variadic. See eficall!{} for details.
        *mut core::ffi::c_void,
        *mut core::ffi::c_void,
    ) -> crate::base::Status},

    pub calculate_crc32: eficall!{fn(
        *mut core::ffi::c_void,
        usize,
        *mut u32,
    ) -> crate::base::Status},

    pub copy_mem: eficall!{fn(
        *mut core::ffi::c_void,
        *mut core::ffi::c_void,
        usize,
    )},
    pub set_mem: eficall!{fn(
        *mut core::ffi::c_void,
        usize,
        u8,
    )},

    // 2.0+

    pub create_event_ex: eficall!{fn(
        u32,
        crate::base::Tpl,
        EventNotify,
        *const core::ffi::c_void,
        *const crate::base::Guid,
        *mut crate::base::Event,
    ) -> crate::base::Status},
}

pub const SYSTEM_TABLE_REVISION_2_70: u32 = (2 << 16) | (70);
pub const SYSTEM_TABLE_REVISION_2_60: u32 = (2 << 16) | (60);
pub const SYSTEM_TABLE_REVISION_2_50: u32 = (2 << 16) | (50);
pub const SYSTEM_TABLE_REVISION_2_40: u32 = (2 << 16) | (40);
pub const SYSTEM_TABLE_REVISION_2_31: u32 = (2 << 16) | (31);
pub const SYSTEM_TABLE_REVISION_2_30: u32 = (2 << 16) | (30);
pub const SYSTEM_TABLE_REVISION_2_20: u32 = (2 << 16) | (20);
pub const SYSTEM_TABLE_REVISION_2_10: u32 = (2 << 16) | (10);
pub const SYSTEM_TABLE_REVISION_2_00: u32 = (2 << 16) | ( 0);
pub const SYSTEM_TABLE_REVISION_1_10: u32 = (1 << 16) | (10);
pub const SYSTEM_TABLE_REVISION_1_02: u32 = (1 << 16) | ( 2);

pub const SYSTEM_TABLE_SIGNATURE: u64 = 0x5453595320494249u64; // "IBI SYST"
pub const SYSTEM_TABLE_REVISION: u32 = SYSTEM_TABLE_REVISION_2_70;

#[repr(C)]
pub struct SystemTable {
    pub hdr: TableHeader,
    pub firmware_vendor: *mut crate::base::Char16,
    pub firmware_revision: u32,

    pub console_in_handle: crate::base::Handle,
    pub con_in: *mut crate::protocols::simple_text_input::Protocol,
    pub console_out_handle: crate::base::Handle,
    pub con_out: *mut crate::protocols::simple_text_output::Protocol,
    pub standard_error_handle: crate::base::Handle,
    pub std_err: *mut crate::protocols::simple_text_output::Protocol,

    pub runtime_services: *mut RuntimeServices,
    pub boot_services: *mut BootServices,

    pub number_of_table_entries: usize,
    pub configuration_table: *mut ConfigurationTable,
}
