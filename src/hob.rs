//! Hand Off Block (HOB)
//!
//! Contains protocols defined in UEFI's Platform Initialization (PI) Specification.
//! See <https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Pi/PiHob.h>
//!
//! ## Example
//! ```
//! use mu_pi::{hob, hob::Hob, hob::HobList};
//! use core::mem::size_of;
//!
//! // Generate HOBs to initialize a new HOB list
//! fn gen_capsule() -> hob::Capsule {
//!   let header = hob::header::Hob { r#type: hob::UEFI_CAPSULE, length: size_of::<hob::Capsule>() as u16, reserved: 0 };
//!
//!   hob::Capsule { header, base_address: 0, length: 0x12 }
//! }
//!
//! fn gen_firmware_volume2() -> hob::FirmwareVolume2 {
//!   let header = hob::header::Hob { r#type: hob::FV2, length: size_of::<hob::FirmwareVolume2>() as u16, reserved: 0 };
//!
//!   hob::FirmwareVolume2 {
//!     header,
//!     base_address: 0,
//!     length: 0x0123456789abcdef,
//!     fv_name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
//!     file_name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
//!   }
//! }
//!
//! fn gen_end_of_hoblist() -> hob::PhaseHandoffInformationTable {
//!   let header = hob::header::Hob {
//!     r#type: hob::END_OF_HOB_LIST,
//!     length: size_of::<hob::PhaseHandoffInformationTable>() as u16,
//!     reserved: 0,
//!   };
//!
//!   hob::PhaseHandoffInformationTable {
//!     header,
//!     version: 0x00010000,
//!     boot_mode: 0,
//!     memory_top: 0xdeadbeef,
//!     memory_bottom: 0xdeadc0de,
//!     free_memory_top: 104,
//!     free_memory_bottom: 255,
//!     end_of_hob_list: 0xdeaddeadc0dec0de,
//!   }
//! }
//!
//! // Generate some example HOBs
//! let capsule = gen_capsule();
//! let firmware_volume2 = gen_firmware_volume2();
//! let end_of_hob_list = gen_end_of_hoblist();
//!
//! // Create a new empty HOB list
//! let mut hoblist = HobList::new();
//!
//! // Push the example HOBs onto the HOB list
//! hoblist.push(Hob::Capsule(&capsule));
//! hoblist.push(Hob::FirmwareVolume2(&firmware_volume2));
//! hoblist.push(Hob::Handoff(&end_of_hob_list));
//! ```
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

use crate::address_helper::{align_down, align_up};
use core::{
    ffi::c_void,
    fmt,
    marker::PhantomData,
    mem::{self, size_of},
};
use indoc::indoc;

// Expectation is someone will provide alloc
extern crate alloc;
use alloc::vec::Vec;

// If the target is x86_64, then EfiPhysicalAddress is u64
#[cfg(target_arch = "x86_64")]
pub type EfiPhysicalAddress = u64;

// If the target is aarch64, then EfiPhysicalAddress is u64
#[cfg(target_arch = "aarch64")]
pub type EfiPhysicalAddress = u64;

// if the target is x86, then EfiPhysicalAddress is u32
#[cfg(target_arch = "x86")]
pub type EfiPhysicalAddress = u32;

// if the target is not x86, x86_64, or aarch64, then alert the user
#[cfg(not(any(target_arch = "x86", target_arch = "x86_64", target_arch = "aarch64")))]
compile_error!("This crate only (currently) supports x86, x86_64, and aarch64 architectures");

// All targets assume (currently) that EfiBootMode is u32
pub type EfiBootMode = u32;

// HOB type field is a UINT16
pub const HANDOFF: u16 = 0x0001;
pub const MEMORY_ALLOCATION: u16 = 0x0002;
pub const RESOURCE_DESCRIPTOR: u16 = 0x0003;
pub const GUID_EXTENSION: u16 = 0x0004;
pub const FV: u16 = 0x0005;
pub const CPU: u16 = 0x0006;
pub const MEMORY_POOL: u16 = 0x0007;
pub const FV2: u16 = 0x0009;
pub const LOAD_PEIM_UNUSED: u16 = 0x000A;
pub const UEFI_CAPSULE: u16 = 0x000B;
pub const FV3: u16 = 0x000C;
pub const UNUSED: u16 = 0xFFFE;
pub const END_OF_HOB_LIST: u16 = 0xFFFF;

pub mod header {
    use crate::hob::EfiPhysicalAddress;
    use r_efi::system::MemoryType;

    /// Describes the format and size of the data inside the HOB.
    /// All HOBs must contain this generic HOB header (EFI_HOB_GENERIC_HEADER).
    ///
    #[repr(C)]
    #[derive(Copy, Clone, Debug)]
    pub struct Hob {
        // EFI_HOB_GENERIC_HEADER
        /// Identifies the HOB data structure type.
        ///
        pub r#type: u16,

        /// The length in bytes of the HOB.
        ///
        pub length: u16,

        /// This field must always be set to zero.
        ///
        pub reserved: u32,
    }

    /// MemoryAllocation (EFI_HOB_MEMORY_ALLOCATION_HEADER) describes the
    /// various attributes of the logical memory allocation. The type field will be used for
    /// subsequent inclusion in the UEFI memory map.
    ///
    #[repr(C)]
    #[derive(Copy, Clone, Debug)]
    pub struct MemoryAllocation {
        // EFI_HOB_MEMORY_ALLOCATION_HEADER
        /// A GUID that defines the memory allocation region's type and purpose, as well as
        /// other fields within the memory allocation HOB. This GUID is used to define the
        /// additional data within the HOB that may be present for the memory allocation HOB.
        /// Type EFI_GUID is defined in InstallProtocolInterface() in the UEFI 2.0
        /// specification.
        ///
        pub name: r_efi::base::Guid,

        /// The base address of memory allocated by this HOB. Type
        /// EfiPhysicalAddress is defined in AllocatePages() in the UEFI 2.0
        /// specification.
        ///
        pub memory_base_address: EfiPhysicalAddress,

        /// The length in bytes of memory allocated by this HOB.
        ///
        pub memory_length: u64,

        /// Defines the type of memory allocated by this HOB. The memory type definition
        /// follows the EFI_MEMORY_TYPE definition. Type EFI_MEMORY_TYPE is defined
        /// in AllocatePages() in the UEFI 2.0 specification.
        ///
        pub memory_type: MemoryType,

        /// This field will always be set to zero.
        ///
        pub reserved: [u8; 4],
    }
}

/// Describes pool memory allocations.
///
/// The HOB generic header. Header.HobType = EFI_HOB_TYPE_MEMORY_POOL.
///
pub type MemoryPool = header::Hob;

/// Contains general state information used by the HOB producer phase.
/// This HOB must be the first one in the HOB list.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct PhaseHandoffInformationTable {
    /// The HOB generic header. Header.HobType = EFI_HOB_TYPE_HANDOFF.
    ///
    pub header: header::Hob, // EFI_HOB_GENERIC_HEADER

    /// The version number pertaining to the PHIT HOB definition.
    /// This value is four bytes in length to provide an 8-byte aligned entry
    /// when it is combined with the 4-byte BootMode.
    ///
    pub version: u32,

    /// The system boot mode as determined during the HOB producer phase.
    ///
    pub boot_mode: EfiBootMode,

    /// The highest address location of memory that is allocated for use by the HOB producer
    /// phase. This address must be 4-KB aligned to meet page restrictions of UEFI.
    ///
    pub memory_top: EfiPhysicalAddress,

    /// The lowest address location of memory that is allocated for use by the HOB producer phase.
    ///
    pub memory_bottom: EfiPhysicalAddress,

    /// The highest address location of free memory that is currently available
    /// for use by the HOB producer phase.
    ///
    pub free_memory_top: EfiPhysicalAddress,

    /// The lowest address location of free memory that is available for use by the HOB producer phase.
    ///
    pub free_memory_bottom: EfiPhysicalAddress,

    /// The end of the HOB list.
    ///
    pub end_of_hob_list: EfiPhysicalAddress,
}

/// Describes all memory ranges used during the HOB producer
/// phase that exist outside the HOB list. This HOB type
/// describes how memory is used, not the physical attributes of memory.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct MemoryAllocation {
    // EFI_HOB_MEMORY_ALLOCATION
    /// The HOB generic header. Header.HobType = EFI_HOB_TYPE_MEMORY_ALLOCATION.
    ///
    pub header: header::Hob,

    /// An instance of the EFI_HOB_MEMORY_ALLOCATION_HEADER that describes the
    /// various attributes of the logical memory allocation.
    ///
    pub alloc_descriptor: header::MemoryAllocation,
    // Additional data pertaining to the "Name" Guid memory
    // may go here.
    //
}

// EFI_HOB_MEMORY_ALLOCATION_STACK
/// Describes the memory stack that is produced by the HOB producer
/// phase and upon which all post-memory-installed executable
/// content in the HOB producer phase is executing.
///
pub type MemoryAllocationStack = MemoryAllocation;

// EFI_HOB_MEMORY_ALLOCATION_BSP_STORE
/// Defines the location of the boot-strap
/// processor (BSP) BSPStore ("Backing Store Pointer Store").
/// This HOB is valid for the Itanium processor family only
/// register overflow store.
///
pub type MemoryAllocationBspStore = MemoryAllocation;

/// Defines the location and entry point of the HOB consumer phase.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct MemoryAllocationModule {
    /// The HOB generic header. Header.HobType = EFI_HOB_TYPE_MEMORY_ALLOCATION.
    ///
    pub header: header::Hob,

    /// An instance of the EFI_HOB_MEMORY_ALLOCATION_HEADER that describes the
    /// various attributes of the logical memory allocation.
    ///
    pub alloc_descriptor: header::MemoryAllocation,

    /// The GUID specifying the values of the firmware file system name
    /// that contains the HOB consumer phase component.
    ///
    pub module_name: r_efi::base::Guid, // EFI_GUID

    /// The address of the memory-mapped firmware volume
    /// that contains the HOB consumer phase firmware file.
    ///
    pub entry_point: u64, // EFI_PHYSICAL_ADDRESS
}

//
// Value of ResourceType in EFI_HOB_RESOURCE_DESCRIPTOR.
//
pub const EFI_RESOURCE_SYSTEM_MEMORY: u32 = 0x00000000;
pub const EFI_RESOURCE_MEMORY_MAPPED_IO: u32 = 0x00000001;
pub const EFI_RESOURCE_IO: u32 = 0x00000002;
pub const EFI_RESOURCE_FIRMWARE_DEVICE: u32 = 0x00000003;
pub const EFI_RESOURCE_MEMORY_MAPPED_IO_PORT: u32 = 0x00000004;
pub const EFI_RESOURCE_MEMORY_RESERVED: u32 = 0x00000005;
pub const EFI_RESOURCE_IO_RESERVED: u32 = 0x00000006;

//
// BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED is defined for unaccepted memory.
// But this definition has not been officially in the PI spec. Base
// on the code-first we define BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED at
// MdeModulePkg/Include/Pi/PrePiHob.h and update EFI_RESOURCE_MAX_MEMORY_TYPE
// to 8. After BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED is officially published
// in PI spec, we will re-visit here.
//
// #define BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED      0x00000007
pub const EFI_RESOURCE_MAX_MEMORY_TYPE: u32 = 0x00000007;

//
// These types can be ORed together as needed.
//
// The following attributes are used to describe settings
//
pub const EFI_RESOURCE_ATTRIBUTE_PRESENT: u32 = 0x00000001;
pub const EFI_RESOURCE_ATTRIBUTE_INITIALIZED: u32 = 0x00000002;
pub const EFI_RESOURCE_ATTRIBUTE_TESTED: u32 = 0x00000004;
pub const EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED: u32 = 0x00000080;

//
// This is typically used as memory cacheability attribute today.
// NOTE: Since PI spec 1.4, please use EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED
// as Physical write protected attribute, and EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED
// means Memory cacheability attribute: The memory supports being programmed with
// a writeprotected cacheable attribute.
//
pub const EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED: u32 = 0x00000100;
pub const EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED: u32 = 0x00000200;
pub const EFI_RESOURCE_ATTRIBUTE_PERSISTENT: u32 = 0x00800000;

//
// Physical memory relative reliability attribute. This
// memory provides higher reliability relative to other
// memory in the system. If all memory has the same
// reliability, then this bit is not used.
//
pub const EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE: u32 = 0x02000000;

//
// The rest of the attributes are used to describe capabilities
//
pub const EFI_RESOURCE_ATTRIBUTE_SINGLE_BIT_ECC: u32 = 0x00000008;
pub const EFI_RESOURCE_ATTRIBUTE_MULTIPLE_BIT_ECC: u32 = 0x00000010;
pub const EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_1: u32 = 0x00000020;
pub const EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_2: u32 = 0x00000040;
pub const EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE: u32 = 0x00000400;
pub const EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE: u32 = 0x00000800;
pub const EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE: u32 = 0x00001000;
pub const EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE: u32 = 0x00002000;
pub const EFI_RESOURCE_ATTRIBUTE_16_BIT_IO: u32 = 0x00004000;
pub const EFI_RESOURCE_ATTRIBUTE_32_BIT_IO: u32 = 0x00008000;
pub const EFI_RESOURCE_ATTRIBUTE_64_BIT_IO: u32 = 0x00010000;
pub const EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED: u32 = 0x00020000;
pub const EFI_RESOURCE_ATTRIBUTE_READ_PROTECTABLE: u32 = 0x00100000;

//
// This is typically used as memory cacheability attribute today.
// NOTE: Since PI spec 1.4, please use EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE
// as Memory capability attribute: The memory supports being protected from processor
// writes, and EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE TABLE means Memory cacheability attribute:
// The memory supports being programmed with a writeprotected cacheable attribute.
//
pub const EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE: u32 = 0x00200000;
pub const EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE: u32 = 0x00400000;
pub const EFI_RESOURCE_ATTRIBUTE_PERSISTABLE: u32 = 0x01000000;

pub const EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED: u32 = 0x00040000;
pub const EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE: u32 = 0x00080000;

pub const MEMORY_ATTRIBUTE_MASK: u32 = EFI_RESOURCE_ATTRIBUTE_PRESENT
    | EFI_RESOURCE_ATTRIBUTE_INITIALIZED
    | EFI_RESOURCE_ATTRIBUTE_TESTED
    | EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED
    | EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED
    | EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED
    | EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED
    | EFI_RESOURCE_ATTRIBUTE_16_BIT_IO
    | EFI_RESOURCE_ATTRIBUTE_32_BIT_IO
    | EFI_RESOURCE_ATTRIBUTE_64_BIT_IO
    | EFI_RESOURCE_ATTRIBUTE_PERSISTENT;

pub const TESTED_MEMORY_ATTRIBUTES: u32 =
    EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED;

pub const INITIALIZED_MEMORY_ATTRIBUTES: u32 = EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED;

pub const PRESENT_MEMORY_ATTRIBUTES: u32 = EFI_RESOURCE_ATTRIBUTE_PRESENT;

/// Attributes for reserved memory before it is promoted to system memory
pub const EFI_MEMORY_PRESENT: u64 = 0x0100_0000_0000_0000;
pub const EFI_MEMORY_INITIALIZED: u64 = 0x0200_0000_0000_0000;
pub const EFI_MEMORY_TESTED: u64 = 0x0400_0000_0000_0000;

///
/// Physical memory persistence attribute.
/// The memory region supports byte-addressable non-volatility.
///
pub const EFI_MEMORY_NV: u64 = 0x0000_0000_0000_8000;
///
/// The memory region provides higher reliability relative to other memory in the system.
/// If all memory has the same reliability, then this bit is not used.
///
pub const EFI_MEMORY_MORE_RELIABLE: u64 = 0x0000_0000_0001_0000;

/// Describes the resource properties of all fixed,
/// nonrelocatable resource ranges found on the processor
/// host bus during the HOB producer phase.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct ResourceDescriptor {
    // EFI_HOB_RESOURCE_DESCRIPTOR
    /// The HOB generic header. Header.HobType = EFI_HOB_TYPE_RESOURCE_DESCRIPTOR.
    ///
    pub header: header::Hob,

    /// A GUID representing the owner of the resource. This GUID is used by HOB
    /// consumer phase components to correlate device ownership of a resource.
    ///
    pub owner: r_efi::base::Guid,

    /// The resource type enumeration as defined by EFI_RESOURCE_TYPE.
    ///
    pub resource_type: u32,

    /// Resource attributes as defined by EFI_RESOURCE_ATTRIBUTE_TYPE.
    ///
    pub resource_attribute: u32,

    /// The physical start address of the resource region.
    ///
    pub physical_start: EfiPhysicalAddress,

    /// The number of bytes of the resource region.
    ///
    pub resource_length: u64,
}

impl ResourceDescriptor {
    pub fn attributes_valid(&self) -> bool {
        (self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED == 0
            || self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE != 0)
            && (self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED == 0
                || self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE != 0)
            && (self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED == 0
                || self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE != 0)
            && (self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED == 0
                || self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE != 0)
            && (self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_PERSISTENT == 0
                || self.resource_attribute & EFI_RESOURCE_ATTRIBUTE_PERSISTABLE != 0)
    }
}

/// Allows writers of executable content in the HOB producer phase to
/// maintain and manage HOBs with specific GUID.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct GuidHob {
    // EFI_HOB_GUID_TYPE
    /// The HOB generic header. Header.HobType = EFI_HOB_TYPE_GUID_EXTENSION.
    ///
    pub header: header::Hob,

    /// A GUID that defines the contents of this HOB.
    ///
    pub name: r_efi::base::Guid,
    // Guid specific data goes here
    //
}

/// Details the location of firmware volumes that contain firmware files.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct FirmwareVolume {
    // EFI_HOB_FIRMWARE_VOLUME
    /// The HOB generic header. Header.HobType = EFI_HOB_TYPE_FV.
    ///
    pub header: header::Hob,

    /// The physical memory-mapped base address of the firmware volume.
    ///
    pub base_address: EfiPhysicalAddress,

    /// The length in bytes of the firmware volume.
    ///
    pub length: u64,
}

/// Details the location of a firmware volume that was extracted
/// from a file within another firmware volume.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct FirmwareVolume2 {
    // EFI_HOB_FIRMWARE_VOLUME2
    /// The HOB generic header. Header.HobType = EFI_HOB_TYPE_FV2.
    ///
    pub header: header::Hob,

    /// The physical memory-mapped base address of the firmware volume.
    ///
    pub base_address: EfiPhysicalAddress,

    /// The length in bytes of the firmware volume.
    ///
    pub length: u64,

    /// The name of the firmware volume.
    ///
    pub fv_name: r_efi::base::Guid,

    /// The name of the firmware file that contained this firmware volume.
    ///
    pub file_name: r_efi::base::Guid,
}

/// Details the location of a firmware volume that was extracted
/// from a file within another firmware volume.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct FirmwareVolume3 {
    // EFI_HOB_FIRMWARE_VOLUME3
    /// The HOB generic header. Header.HobType = EFI_HOB_TYPE_FV3.
    ///
    pub header: header::Hob,

    /// The physical memory-mapped base address of the firmware volume.
    ///
    pub base_address: EfiPhysicalAddress,

    /// The length in bytes of the firmware volume.
    ///
    pub length: u64,

    /// The authentication status.
    ///
    pub authentication_status: u32,

    /// TRUE if the FV was extracted as a file within another firmware volume.
    /// FALSE otherwise.
    ///
    pub extracted_fv: r_efi::efi::Boolean,

    /// The name of the firmware volume.
    /// Valid only if IsExtractedFv is TRUE.
    ///
    pub fv_name: r_efi::base::Guid,

    /// The name of the firmware file that contained this firmware volume.
    /// Valid only if IsExtractedFv is TRUE.
    ///
    pub file_name: r_efi::base::Guid,
}

/// Describes processor information, such as address space and I/O space capabilities.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct Cpu {
    // EFI_HOB_CPU
    /// The HOB generic header. Header.HobType = EFI_HOB_TYPE_CPU.
    ///
    pub header: header::Hob,

    /// Identifies the maximum physical memory addressability of the processor.
    ///
    pub size_of_memory_space: u8,

    /// Identifies the maximum physical I/O addressability of the processor.
    ///
    pub size_of_io_space: u8,

    /// This field will always be set to zero.
    ///
    pub reserved: [u8; 6],
}

/// Each UEFI capsule HOB details the location of a UEFI capsule. It includes a base address and length
/// which is based upon memory blocks with a EFI_CAPSULE_HEADER and the associated
/// CapsuleImageSize-based payloads. These HOB's shall be created by the PEI PI firmware
/// sometime after the UEFI UpdateCapsule service invocation with the
/// CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flag set in the EFI_CAPSULE_HEADER.
///
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct Capsule {
    // EFI_HOB_CAPSULE
    /// The HOB generic header where Header.HobType = EFI_HOB_TYPE_UEFI_CAPSULE.
    ///
    pub header: header::Hob,

    /// The physical memory-mapped base address of an UEFI capsule. This value is set to
    /// point to the base of the contiguous memory of the UEFI capsule.
    /// The length of the contiguous memory in bytes.
    ///
    pub base_address: u8,
    pub length: u8,
}

/// Represents a HOB list.
///
pub struct HobList<'a>(Vec<Hob<'a>>);

impl Default for HobList<'_> {
    fn default() -> Self {
        HobList::new()
    }
}

/// Union of all the possible HOB Types.
///
#[derive(Clone, Debug)]
pub enum Hob<'a> {
    Handoff(&'a PhaseHandoffInformationTable),
    MemoryAllocation(&'a MemoryAllocation),
    MemoryAllocationModule(&'a MemoryAllocationModule),
    Capsule(&'a Capsule),
    ResourceDescriptor(&'a ResourceDescriptor),
    GuidHob(&'a GuidHob),
    FirmwareVolume(&'a FirmwareVolume),
    FirmwareVolume2(&'a FirmwareVolume2),
    FirmwareVolume3(&'a FirmwareVolume3),
    Cpu(&'a Cpu),
    Misc(u16),
}

pub trait HobTrait {
    fn size(&self) -> usize;
    fn as_ptr<T>(&self) -> *const T;
}

// HOB Trait implementation.
impl HobTrait for Hob<'_> {
    /// Returns the size of the HOB.
    fn size(&self) -> usize {
        match self {
            Hob::Handoff(_) => size_of::<PhaseHandoffInformationTable>(),
            Hob::MemoryAllocation(_) => size_of::<MemoryAllocation>(),
            Hob::MemoryAllocationModule(_) => size_of::<MemoryAllocationModule>(),
            Hob::Capsule(_) => size_of::<Capsule>(),
            Hob::ResourceDescriptor(_) => size_of::<ResourceDescriptor>(),
            Hob::GuidHob(_) => size_of::<GuidHob>(),
            Hob::FirmwareVolume(_) => size_of::<FirmwareVolume>(),
            Hob::FirmwareVolume2(_) => size_of::<FirmwareVolume2>(),
            Hob::FirmwareVolume3(_) => size_of::<FirmwareVolume3>(),
            Hob::Cpu(_) => size_of::<Cpu>(),
            Hob::Misc(_) => size_of::<u16>(),
        }
    }

    /// Returns a pointer to the HOB.
    fn as_ptr<T>(&self) -> *const T {
        match self {
            Hob::Handoff(hob) => *hob as *const PhaseHandoffInformationTable as *const _,
            Hob::MemoryAllocation(hob) => *hob as *const MemoryAllocation as *const _,
            Hob::MemoryAllocationModule(hob) => *hob as *const MemoryAllocationModule as *const _,
            Hob::Capsule(hob) => *hob as *const Capsule as *const _,
            Hob::ResourceDescriptor(hob) => *hob as *const ResourceDescriptor as *const _,
            Hob::GuidHob(hob) => *hob as *const GuidHob as *const _,
            Hob::FirmwareVolume(hob) => *hob as *const FirmwareVolume as *const _,
            Hob::FirmwareVolume2(hob) => *hob as *const FirmwareVolume2 as *const _,
            Hob::FirmwareVolume3(hob) => *hob as *const FirmwareVolume3 as *const _,
            Hob::Cpu(hob) => *hob as *const Cpu as *const _,
            Hob::Misc(hob) => *hob as *const u16 as *const _,
        }
    }
}

impl<'a> HobList<'a> {
    /// Instantiates a Hoblist.
    pub fn new() -> Self {
        HobList(Vec::new())
    }

    /// Implements iter for Hoblist.
    ///
    /// # Example(s)
    ///
    /// ```no_run
    /// use core::ffi::c_void;
    /// use mu_pi::hob::HobList;
    ///
    /// fn example(hob_list: *const c_void) {
    ///     // example discovering and adding hobs to a hob list
    ///     let mut the_hob_list = HobList::default();
    ///     the_hob_list.discover_hobs(hob_list);
    ///
    ///     for hob in the_hob_list.iter() {
    ///         // ... do something with the hob(s)
    ///     }
    /// }
    /// ```
    pub fn iter(&self) -> impl Iterator<Item = &Hob> {
        self.0.iter()
    }

    /// Returns the size of the Hoblist in bytes.
    ///
    /// # Example(s)
    ///
    /// ```no_run
    /// use core::ffi::c_void;
    /// use mu_pi::hob::HobList;
    ///
    /// fn example(hob_list: *const c_void) {
    ///     // example discovering and adding hobs to a hob list
    ///     let mut the_hob_list = HobList::default();
    ///     the_hob_list.discover_hobs(hob_list);
    ///
    ///     let length = the_hob_list.size();
    ///     println!("size_of_hobs: {:?}", length);
    /// }
    pub fn size(&self) -> usize {
        let mut size_of_hobs = 0;

        for hob in self.iter() {
            size_of_hobs += hob.size()
        }

        size_of_hobs
    }

    /// Implements len for Hoblist.
    /// Returns the number of hobs in the list.
    ///
    /// # Example(s)
    /// ```no_run
    /// use core::ffi::c_void;
    /// use mu_pi::hob::HobList;
    ///
    /// fn example(hob_list: *const c_void) {
    ///    // example discovering and adding hobs to a hob list
    ///    let mut the_hob_list = HobList::default();
    ///    the_hob_list.discover_hobs(hob_list);
    ///
    ///    let length = the_hob_list.len();
    ///    println!("length_of_hobs: {:?}", length);
    /// }
    /// ```
    pub fn len(&self) -> usize {
        self.0.len()
    }

    /// Implements is_empty for Hoblist.
    /// Returns true if the list is empty.
    ///
    /// # Example(s)
    /// ```no_run
    /// use core::ffi::c_void;
    /// use mu_pi::hob::HobList;
    ///
    /// fn example(hob_list: *const c_void) {
    ///    // example discovering and adding hobs to a hob list
    ///    let mut the_hob_list = HobList::default();
    ///    the_hob_list.discover_hobs(hob_list);
    ///
    ///    let is_empty = the_hob_list.is_empty();
    ///    println!("is_empty: {:?}", is_empty);
    /// }
    /// ```
    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }

    /// Implements push for Hoblist.
    ///
    /// Parameters:
    /// * hob: Hob<'a> - the hob to add to the list
    ///
    /// # Example(s)
    /// ```no_run
    /// use core::{ffi::c_void, mem::size_of};
    /// use mu_pi::hob::{HobList, Hob, header, FirmwareVolume, FV};
    ///
    /// fn example(hob_list: *const c_void) {
    ///   // example discovering and adding hobs to a hob list
    ///   let mut the_hob_list = HobList::default();
    ///   the_hob_list.discover_hobs(hob_list);
    ///
    ///   // example pushing a hob onto the list
    ///   let header = header::Hob {
    ///       r#type: FV,
    ///       length: size_of::<FirmwareVolume>() as u16,
    ///       reserved: 0,
    ///   };
    ///
    ///   let firmware_volume = FirmwareVolume {
    ///       header,
    ///       base_address: 0,
    ///       length: 0x0123456789abcdef,
    ///   };
    ///
    ///   let hob = Hob::FirmwareVolume(&firmware_volume);
    ///   the_hob_list.push(hob);
    /// }
    /// ```
    pub fn push(&mut self, hob: Hob<'a>) {
        let cloned_hob = hob.clone();
        self.0.push(cloned_hob);
    }

    /// Discovers hobs from a C style void* and adds them to a rust structure.
    ///
    /// # Example(s)
    ///
    /// ```no_run
    /// use core::ffi::c_void;
    /// use mu_pi::hob::HobList;
    ///
    /// fn example(hob_list: *const c_void) {
    ///     // example discovering and adding hobs to a hob list
    ///     let mut the_hob_list = HobList::default();
    ///     the_hob_list.discover_hobs(hob_list);
    /// }
    /// ```
    pub fn discover_hobs(&mut self, hob_list: *const c_void) {
        const NOT_NULL: &str = "Ptr should not be NULL";
        fn assert_hob_size<T>(hob: &header::Hob) {
            let hob_len = hob.length as usize;
            let hob_size = mem::size_of::<T>();
            assert_eq!(hob_len, hob_size, "Trying to cast hob of length {hob_len} into a pointer of size {hob_size}");
        }

        let mut hob_header: *const header::Hob = hob_list as *const header::Hob;

        loop {
            let current_header = unsafe { hob_header.cast::<header::Hob>().as_ref().expect(NOT_NULL) };
            match current_header.r#type {
                HANDOFF => {
                    assert_hob_size::<PhaseHandoffInformationTable>(current_header);
                    let phit_hob =
                        unsafe { hob_header.cast::<PhaseHandoffInformationTable>().as_ref().expect(NOT_NULL) };
                    self.0.push(Hob::Handoff(phit_hob));
                }
                MEMORY_ALLOCATION => {
                    if current_header.length == mem::size_of::<MemoryAllocationModule>() as u16 {
                        let mem_alloc_hob =
                            unsafe { hob_header.cast::<MemoryAllocationModule>().as_ref().expect(NOT_NULL) };
                        self.0.push(Hob::MemoryAllocationModule(mem_alloc_hob));
                    } else {
                        assert_hob_size::<MemoryAllocation>(current_header);
                        let mem_alloc_hob = unsafe { hob_header.cast::<MemoryAllocation>().as_ref().expect(NOT_NULL) };
                        self.0.push(Hob::MemoryAllocation(mem_alloc_hob));
                    }
                }
                RESOURCE_DESCRIPTOR => {
                    assert_hob_size::<ResourceDescriptor>(current_header);
                    let resource_desc_hob =
                        unsafe { hob_header.cast::<ResourceDescriptor>().as_ref().expect(NOT_NULL) };
                    self.0.push(Hob::ResourceDescriptor(resource_desc_hob));
                }
                GUID_EXTENSION => {
                    let guid_hob = unsafe { hob_header.cast::<GuidHob>().as_ref().expect(NOT_NULL) };
                    self.0.push(Hob::GuidHob(guid_hob));
                }
                FV => {
                    assert_hob_size::<FirmwareVolume>(current_header);
                    let fv_hob = unsafe { hob_header.cast::<FirmwareVolume>().as_ref().expect(NOT_NULL) };
                    self.0.push(Hob::FirmwareVolume(fv_hob));
                }
                FV2 => {
                    assert_hob_size::<FirmwareVolume2>(current_header);
                    let fv2_hob = unsafe { hob_header.cast::<FirmwareVolume2>().as_ref().expect(NOT_NULL) };
                    self.0.push(Hob::FirmwareVolume2(fv2_hob));
                }
                FV3 => {
                    assert_hob_size::<FirmwareVolume3>(current_header);
                    let fv3_hob = unsafe { hob_header.cast::<FirmwareVolume3>().as_ref().expect(NOT_NULL) };
                    self.0.push(Hob::FirmwareVolume3(fv3_hob));
                }
                CPU => {
                    assert_hob_size::<Cpu>(current_header);
                    let cpu_hob = unsafe { hob_header.cast::<Cpu>().as_ref().expect(NOT_NULL) };
                    self.0.push(Hob::Cpu(cpu_hob));
                }
                UEFI_CAPSULE => {
                    assert_hob_size::<Capsule>(current_header);
                    let capsule_hob = unsafe { hob_header.cast::<Capsule>().as_ref().expect(NOT_NULL) };
                    self.0.push(Hob::Capsule(capsule_hob));
                }
                END_OF_HOB_LIST => {
                    break;
                }
                _ => {
                    self.0.push(Hob::Misc(current_header.r#type));
                }
            }
            let next_hob = hob_header as usize + current_header.length as usize;
            hob_header = next_hob as *const header::Hob;
        }
    }
}

/// Implements IntoIterator for HobList.
///
/// Defines how it will be converted to an iterator.
impl<'a> IntoIterator for HobList<'a> {
    type Item = Hob<'a>;
    type IntoIter = <Vec<Hob<'a>> as IntoIterator>::IntoIter;

    fn into_iter(self) -> Self::IntoIter {
        self.0.into_iter()
    }
}

/// Implements Debug for Hoblist.
///
/// Writes Hoblist debug information to stdio
///
impl fmt::Debug for HobList<'_> {
    #[cfg_attr(feature = "nightly", feature(no_coverage))]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for hob in self.0.clone().into_iter() {
            match hob {
                Hob::Handoff(hob) => {
                    write!(
                        f,
                        indoc! {"
                        PHASE HANDOFF INFORMATION TABLE (PHIT) HOB
                          HOB Length: 0x{:x}
                          Version: 0x{:x}
                          Boot Mode: 0x{:x}
                          Memory Bottom: 0x{:x}
                          Memory Top: 0x{:x}
                          Free Memory Bottom: 0x{:x}
                          Free Memory Top: 0x{:x}
                          End of HOB List: 0x{:x}\n"},
                        hob.header.length,
                        hob.version,
                        hob.boot_mode,
                        align_up(hob.memory_bottom, 0x1000),
                        align_down(hob.memory_top, 0x1000),
                        align_up(hob.free_memory_bottom, 0x1000),
                        align_down(hob.free_memory_top, 0x1000),
                        hob.end_of_hob_list
                    )?;
                }
                Hob::MemoryAllocation(hob) => {
                    write!(
                        f,
                        indoc! {"
                        MEMORY ALLOCATION HOB
                          HOB Length: 0x{:x}
                          Memory Base Address: 0x{:x}
                          Memory Length: 0x{:x}
                          Memory Type: {:?}\n"},
                        hob.header.length,
                        hob.alloc_descriptor.memory_base_address,
                        hob.alloc_descriptor.memory_length,
                        hob.alloc_descriptor.memory_type
                    )?;
                }
                Hob::ResourceDescriptor(hob) => {
                    write!(
                        f,
                        indoc! {"
                        RESOURCE DESCRIPTOR HOB
                          HOB Length: 0x{:x}
                          Resource Type: 0x{:x}
                          Resource Attribute Type: 0x{:x}
                          Resource Start Address: 0x{:x}
                          Resource Length: 0x{:x}\n"},
                        hob.header.length,
                        hob.resource_type,
                        hob.resource_attribute,
                        hob.physical_start,
                        hob.resource_length
                    )?;
                }
                Hob::GuidHob(hob) => {
                    write!(
                        f,
                        indoc! {"
                        GUID HOB
                          HOB Length: 0x{:x}\n"},
                        hob.header.length
                    )?;
                }
                Hob::FirmwareVolume2(hob) => {
                    write!(
                        f,
                        indoc! {"
                        FIRMWARE VOLUME 2 (FV2) HOB
                          Base Address: 0x{:x}
                          Length: 0x{:x}\n"},
                        hob.base_address, hob.length
                    )?;
                }
                Hob::FirmwareVolume3(hob) => {
                    write!(
                        f,
                        indoc! {"
                        FIRMWARE VOLUME 3 (FV3) HOB
                          Base Address: 0x{:x}
                          Length: 0x{:x}\n"},
                        hob.base_address, hob.length
                    )?;
                }
                Hob::Cpu(hob) => {
                    write!(
                        f,
                        indoc! {"
                        CPU HOB
                          Memory Space Size: 0x{:x}
                          IO Space Size: 0x{:x}\n"},
                        hob.size_of_memory_space, hob.size_of_io_space
                    )?;
                }
                Hob::Capsule(hob) => {
                    write!(
                        f,
                        indoc! {"
                        CAPSULE HOB
                          Base Address: 0x{:x}
                          Length: 0x{:x}\n"},
                        hob.base_address, hob.length
                    )?;
                }
                _ => (),
            }
        }
        write!(f, "Parsed HOBs")
    }
}

impl Hob<'_> {
    pub fn header(&self) -> header::Hob {
        match self {
            Hob::Handoff(hob) => hob.header,
            Hob::MemoryAllocation(hob) => hob.header,
            Hob::MemoryAllocationModule(hob) => hob.header,
            Hob::Capsule(hob) => hob.header,
            Hob::ResourceDescriptor(hob) => hob.header,
            Hob::GuidHob(hob) => hob.header,
            Hob::FirmwareVolume(hob) => hob.header,
            Hob::FirmwareVolume2(hob) => hob.header,
            Hob::FirmwareVolume3(hob) => hob.header,
            Hob::Cpu(hob) => hob.header,
            Hob::Misc(hob_type) => {
                header::Hob { r#type: *hob_type, length: mem::size_of::<header::Hob>() as u16, reserved: 0 }
            }
        }
    }
}

/// A HOB iterator.
///
pub struct HobIter<'a> {
    hob_ptr: *const header::Hob,
    _a: PhantomData<&'a ()>,
}

impl<'a> IntoIterator for &Hob<'a> {
    type Item = Hob<'a>;

    type IntoIter = HobIter<'a>;

    fn into_iter(self) -> Self::IntoIter {
        HobIter { hob_ptr: self.as_ptr(), _a: PhantomData }
    }
}

impl<'a> Iterator for HobIter<'a> {
    type Item = Hob<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        const NOT_NULL: &str = "Ptr should not be NULL";
        let hob_header = unsafe { *(self.hob_ptr) };
        let hob = unsafe {
            match hob_header.r#type {
                HANDOFF => {
                    Hob::Handoff((self.hob_ptr as *const PhaseHandoffInformationTable).as_ref().expect(NOT_NULL))
                }
                MEMORY_ALLOCATION if hob_header.length as usize == mem::size_of::<MemoryAllocationModule>() => {
                    Hob::MemoryAllocationModule(
                        (self.hob_ptr as *const MemoryAllocationModule).as_ref().expect(NOT_NULL),
                    )
                }
                MEMORY_ALLOCATION => {
                    Hob::MemoryAllocation((self.hob_ptr as *const MemoryAllocation).as_ref().expect(NOT_NULL))
                }
                RESOURCE_DESCRIPTOR => {
                    Hob::ResourceDescriptor((self.hob_ptr as *const ResourceDescriptor).as_ref().expect(NOT_NULL))
                }
                GUID_EXTENSION => Hob::GuidHob((self.hob_ptr as *const GuidHob).as_ref().expect(NOT_NULL)),
                FV => Hob::FirmwareVolume((self.hob_ptr as *const FirmwareVolume).as_ref().expect(NOT_NULL)),
                FV2 => Hob::FirmwareVolume2((self.hob_ptr as *const FirmwareVolume2).as_ref().expect(NOT_NULL)),
                FV3 => Hob::FirmwareVolume3((self.hob_ptr as *const FirmwareVolume3).as_ref().expect(NOT_NULL)),
                CPU => Hob::Cpu((self.hob_ptr as *const Cpu).as_ref().expect(NOT_NULL)),
                UEFI_CAPSULE => Hob::Capsule((self.hob_ptr as *const Capsule).as_ref().expect(NOT_NULL)),
                END_OF_HOB_LIST => return None,
                hob_type => Hob::Misc(hob_type),
            }
        };
        self.hob_ptr = (self.hob_ptr as usize + hob_header.length as usize) as *const header::Hob;
        Some(hob)
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        hob,
        hob::{Hob, HobList, HobTrait},
    };

    use core::{
        ffi::c_void,
        mem::{drop, forget, size_of},
        slice::from_raw_parts,
    };

    // Expectation is someone will provide alloc
    extern crate alloc;
    use alloc::vec::Vec;

    // Generate a test firmware volume hob
    // # Returns
    // A FirmwareVolume hob
    fn gen_firmware_volume() -> hob::FirmwareVolume {
        let header = hob::header::Hob { r#type: hob::FV, length: size_of::<hob::FirmwareVolume>() as u16, reserved: 0 };

        hob::FirmwareVolume { header, base_address: 0, length: 0x0123456789abcdef }
    }

    // Generate a test firmware volume 2 hob
    // # Returns
    // A FirmwareVolume2 hob
    fn gen_firmware_volume2() -> hob::FirmwareVolume2 {
        let header =
            hob::header::Hob { r#type: hob::FV2, length: size_of::<hob::FirmwareVolume2>() as u16, reserved: 0 };

        hob::FirmwareVolume2 {
            header,
            base_address: 0,
            length: 0x0123456789abcdef,
            fv_name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
            file_name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
        }
    }

    // Generate a test firmware volume 3 hob
    // # Returns
    // A FirmwareVolume3 hob
    fn gen_firmware_volume3() -> hob::FirmwareVolume3 {
        let header =
            hob::header::Hob { r#type: hob::FV3, length: size_of::<hob::FirmwareVolume3>() as u16, reserved: 0 };

        hob::FirmwareVolume3 {
            header,
            base_address: 0,
            length: 0x0123456789abcdef,
            authentication_status: 0,
            extracted_fv: false.into(),
            fv_name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
            file_name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
        }
    }

    // Generate a test resource descriptor hob
    // # Returns
    // A ResourceDescriptor hob
    fn gen_resource_descriptor() -> hob::ResourceDescriptor {
        let header = hob::header::Hob {
            r#type: hob::RESOURCE_DESCRIPTOR,
            length: size_of::<hob::ResourceDescriptor>() as u16,
            reserved: 0,
        };

        hob::ResourceDescriptor {
            header,
            owner: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
            resource_type: hob::EFI_RESOURCE_SYSTEM_MEMORY,
            resource_attribute: hob::EFI_RESOURCE_ATTRIBUTE_PRESENT,
            physical_start: 0,
            resource_length: 0x0123456789abcdef,
        }
    }

    // Generate a test phase handoff information table hob
    // # Returns
    // A MemoryAllocation hob
    fn gen_memory_allocation() -> hob::MemoryAllocation {
        let header = hob::header::Hob {
            r#type: hob::MEMORY_ALLOCATION,
            length: size_of::<hob::MemoryAllocation>() as u16,
            reserved: 0,
        };

        let alloc_descriptor = hob::header::MemoryAllocation {
            name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
            memory_base_address: 0,
            memory_length: 0x0123456789abcdef,
            memory_type: 0,
            reserved: [0; 4],
        };

        hob::MemoryAllocation { header, alloc_descriptor }
    }

    fn gen_memory_allocation_module() -> hob::MemoryAllocationModule {
        let header = hob::header::Hob {
            r#type: hob::MEMORY_ALLOCATION,
            length: size_of::<hob::MemoryAllocationModule>() as u16,
            reserved: 0,
        };

        let alloc_descriptor = hob::header::MemoryAllocation {
            name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
            memory_base_address: 0,
            memory_length: 0x0123456789abcdef,
            memory_type: 0,
            reserved: [0; 4],
        };

        hob::MemoryAllocationModule {
            header,
            alloc_descriptor,
            module_name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]),
            entry_point: 0,
        }
    }

    fn gen_capsule() -> hob::Capsule {
        let header =
            hob::header::Hob { r#type: hob::UEFI_CAPSULE, length: size_of::<hob::Capsule>() as u16, reserved: 0 };

        hob::Capsule { header, base_address: 0, length: 0x12 }
    }

    fn gen_guid_hob() -> hob::GuidHob {
        let header =
            hob::header::Hob { r#type: hob::GUID_EXTENSION, length: size_of::<hob::GuidHob>() as u16, reserved: 0 };

        hob::GuidHob { header, name: r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]) }
    }

    fn gen_phase_handoff_information_table() -> hob::PhaseHandoffInformationTable {
        let header = hob::header::Hob {
            r#type: hob::HANDOFF,
            length: size_of::<hob::PhaseHandoffInformationTable>() as u16,
            reserved: 0,
        };

        hob::PhaseHandoffInformationTable {
            header,
            version: 0x00010000,
            boot_mode: 0,
            memory_top: 0xdeadbeef,
            memory_bottom: 0xdeadc0de,
            free_memory_top: 104,
            free_memory_bottom: 255,
            end_of_hob_list: 0xdeaddeadc0dec0de,
        }
    }

    // Generate a test end of hoblist hob
    // # Returns
    // A PhaseHandoffInformationTable hob
    fn gen_end_of_hoblist() -> hob::PhaseHandoffInformationTable {
        let header = hob::header::Hob {
            r#type: hob::END_OF_HOB_LIST,
            length: size_of::<hob::PhaseHandoffInformationTable>() as u16,
            reserved: 0,
        };

        hob::PhaseHandoffInformationTable {
            header,
            version: 0x00010000,
            boot_mode: 0,
            memory_top: 0xdeadbeef,
            memory_bottom: 0xdeadc0de,
            free_memory_top: 104,
            free_memory_bottom: 255,
            end_of_hob_list: 0xdeaddeadc0dec0de,
        }
    }

    fn gen_cpu() -> hob::Cpu {
        let header = hob::header::Hob { r#type: hob::CPU, length: size_of::<hob::Cpu>() as u16, reserved: 0 };

        hob::Cpu { header, size_of_memory_space: 0, size_of_io_space: 0, reserved: [0; 6] }
    }

    // Converts the Hoblist to a C array.
    // # Arguments
    // * `hob_list` - A reference to the HobList.
    //
    // # Returns
    // A tuple containing a pointer to the C array and the length of the C array.
    pub fn to_c_array(hob_list: &hob::HobList) -> (*const c_void, usize) {
        let size = hob_list.size();
        let mut c_array: Vec<u8> = Vec::with_capacity(size);

        for hob in hob_list.iter() {
            let slice = unsafe { from_raw_parts(hob.as_ptr(), hob.size()) };
            c_array.extend_from_slice(slice);
        }

        let void_ptr = c_array.as_ptr() as *const c_void;

        // in order to not call the destructor on the Vec at the end of this function, we need to forget it
        forget(c_array);

        (void_ptr, size)
    }

    // Implements a function to manually free a C array.
    //
    // # Arguments
    // * `c_array_ptr` - A pointer to the C array.
    // * `len` - The length of the C array.
    //
    // # Safety
    // This function is unsafe because it is not guaranteed that the pointer is valid.
    pub fn manually_free_c_array(c_array_ptr: *const c_void, len: usize) {
        let ptr = c_array_ptr as *mut u8;
        unsafe {
            drop(Vec::from_raw_parts(ptr, len, len));
        }
    }

    #[test]
    fn test_hoblist_empty() {
        let hoblist = HobList::new();
        assert_eq!(hoblist.len(), 0);
        assert!(hoblist.is_empty());
    }

    #[test]
    fn test_hoblist_push() {
        let mut hoblist = HobList::new();
        let resource = gen_resource_descriptor();
        hoblist.push(Hob::ResourceDescriptor(&resource));
        assert_eq!(hoblist.len(), 1);

        let firmware_volume = gen_firmware_volume();
        hoblist.push(Hob::FirmwareVolume(&firmware_volume));

        assert_eq!(hoblist.len(), 2);
    }

    #[test]
    fn test_hoblist_iterate() {
        let mut hoblist = HobList::default();
        let resource = gen_resource_descriptor();
        let firmware_volume = gen_firmware_volume();
        let firmware_volume2 = gen_firmware_volume2();
        let firmware_volume3 = gen_firmware_volume3();
        let end_of_hob_list = gen_end_of_hoblist();
        let capsule = gen_capsule();
        let guid_hob = gen_guid_hob();
        let memory_allocation = gen_memory_allocation();
        let memory_allocation_module = gen_memory_allocation_module();

        hoblist.push(Hob::ResourceDescriptor(&resource));
        hoblist.push(Hob::FirmwareVolume(&firmware_volume));
        hoblist.push(Hob::FirmwareVolume2(&firmware_volume2));
        hoblist.push(Hob::FirmwareVolume3(&firmware_volume3));
        hoblist.push(Hob::Capsule(&capsule));
        hoblist.push(Hob::GuidHob(&guid_hob));
        hoblist.push(Hob::MemoryAllocation(&memory_allocation));
        hoblist.push(Hob::MemoryAllocationModule(&memory_allocation_module));
        hoblist.push(Hob::Handoff(&end_of_hob_list));

        let mut count = 0;
        hoblist.iter().for_each(|hob| {
            match hob {
                Hob::ResourceDescriptor(resource) => {
                    assert_eq!(resource.resource_type, hob::EFI_RESOURCE_SYSTEM_MEMORY);
                }
                Hob::MemoryAllocation(memory_allocation) => {
                    assert_eq!(memory_allocation.alloc_descriptor.memory_length, 0x0123456789abcdef);
                }
                Hob::MemoryAllocationModule(memory_allocation_module) => {
                    assert_eq!(memory_allocation_module.alloc_descriptor.memory_length, 0x0123456789abcdef);
                }
                Hob::Capsule(capsule) => {
                    assert_eq!(capsule.base_address, 0);
                }
                Hob::GuidHob(guid_hob) => {
                    assert_eq!(guid_hob.name, r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]));
                }
                Hob::FirmwareVolume(firmware_volume) => {
                    assert_eq!(firmware_volume.length, 0x0123456789abcdef);
                }
                Hob::FirmwareVolume2(firmware_volume) => {
                    assert_eq!(firmware_volume.length, 0x0123456789abcdef);
                }
                Hob::FirmwareVolume3(firmware_volume) => {
                    assert_eq!(firmware_volume.length, 0x0123456789abcdef);
                }
                Hob::Handoff(handoff) => {
                    assert_eq!(handoff.memory_top, 0xdeadbeef);
                }
                _ => {
                    panic!("Unexpected hob type");
                }
            }
            count += 1;
        });
        assert_eq!(count, 9);
    }

    #[test]
    fn test_hoblist_discover() {
        // generate some test hobs
        let resource = gen_resource_descriptor();
        let handoff = gen_phase_handoff_information_table();
        let firmware_volume = gen_firmware_volume();
        let firmware_volume2 = gen_firmware_volume2();
        let firmware_volume3 = gen_firmware_volume3();
        let capsule = gen_capsule();
        let guid_hob = gen_guid_hob();
        let memory_allocation = gen_memory_allocation();
        let memory_allocation_module = gen_memory_allocation_module();
        let cpu = gen_cpu();
        let end_of_hob_list = gen_end_of_hoblist();

        // create a new hoblist
        let mut hoblist = HobList::new();

        // Push the resource descriptor to the hoblist
        hoblist.push(Hob::ResourceDescriptor(&resource));
        hoblist.push(Hob::Handoff(&handoff));
        hoblist.push(Hob::FirmwareVolume(&firmware_volume));
        hoblist.push(Hob::FirmwareVolume2(&firmware_volume2));
        hoblist.push(Hob::FirmwareVolume3(&firmware_volume3));
        hoblist.push(Hob::Capsule(&capsule));
        hoblist.push(Hob::GuidHob(&guid_hob));
        hoblist.push(Hob::MemoryAllocation(&memory_allocation));
        hoblist.push(Hob::MemoryAllocationModule(&memory_allocation_module));
        hoblist.push(Hob::Cpu(&cpu));
        hoblist.push(Hob::Handoff(&end_of_hob_list));

        // assert that the hoblist has 3 hobs and they are of the correct type

        let mut count = 0;
        hoblist.iter().for_each(|hob| {
            match hob {
                Hob::ResourceDescriptor(resource) => {
                    assert_eq!(resource.resource_type, hob::EFI_RESOURCE_SYSTEM_MEMORY);
                }
                Hob::MemoryAllocation(memory_allocation) => {
                    assert_eq!(memory_allocation.alloc_descriptor.memory_length, 0x0123456789abcdef);
                }
                Hob::MemoryAllocationModule(memory_allocation_module) => {
                    assert_eq!(memory_allocation_module.alloc_descriptor.memory_length, 0x0123456789abcdef);
                }
                Hob::Capsule(capsule) => {
                    assert_eq!(capsule.base_address, 0);
                }
                Hob::GuidHob(guid_hob) => {
                    assert_eq!(guid_hob.name, r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]));
                }
                Hob::FirmwareVolume(firmware_volume) => {
                    assert_eq!(firmware_volume.length, 0x0123456789abcdef);
                }
                Hob::FirmwareVolume2(firmware_volume) => {
                    assert_eq!(firmware_volume.length, 0x0123456789abcdef);
                }
                Hob::FirmwareVolume3(firmware_volume) => {
                    assert_eq!(firmware_volume.length, 0x0123456789abcdef);
                }
                Hob::Handoff(handoff) => {
                    assert_eq!(handoff.memory_top, 0xdeadbeef);
                }
                Hob::Cpu(cpu) => {
                    assert_eq!(cpu.size_of_memory_space, 0);
                }
                _ => {
                    panic!("Unexpected hob type");
                }
            }
            count += 1;
        });

        assert_eq!(count, 11);

        // c_hoblist is a pointer to the hoblist - we need to manually free it later
        let (c_array_hoblist, length) = to_c_array(&hoblist);

        // create a new hoblist
        let mut cloned_hoblist = HobList::new();
        cloned_hoblist.discover_hobs(c_array_hoblist);

        // assert that the hoblist has 2 hobs and they are of the correct type
        // we don't need to check the end of hoblist hob as it will not be 'discovered'
        // by the discover_hobs function and simply end the iteration
        count = 0;
        hoblist.into_iter().for_each(|hob| {
            match hob {
                Hob::ResourceDescriptor(resource) => {
                    assert_eq!(resource.resource_type, hob::EFI_RESOURCE_SYSTEM_MEMORY);
                }
                Hob::MemoryAllocation(memory_allocation) => {
                    assert_eq!(memory_allocation.alloc_descriptor.memory_length, 0x0123456789abcdef);
                }
                Hob::MemoryAllocationModule(memory_allocation_module) => {
                    assert_eq!(memory_allocation_module.alloc_descriptor.memory_length, 0x0123456789abcdef);
                }
                Hob::Capsule(capsule) => {
                    assert_eq!(capsule.base_address, 0);
                }
                Hob::GuidHob(guid_hob) => {
                    assert_eq!(guid_hob.name, r_efi::efi::Guid::from_fields(1, 2, 3, 4, 5, &[6, 7, 8, 9, 10, 11]));
                }
                Hob::FirmwareVolume(firmware_volume) => {
                    assert_eq!(firmware_volume.length, 0x0123456789abcdef);
                }
                Hob::FirmwareVolume2(firmware_volume) => {
                    assert_eq!(firmware_volume.length, 0x0123456789abcdef);
                }
                Hob::FirmwareVolume3(firmware_volume) => {
                    assert_eq!(firmware_volume.length, 0x0123456789abcdef);
                }
                Hob::Handoff(handoff) => {
                    assert_eq!(handoff.memory_top, 0xdeadbeef);
                }
                Hob::Cpu(cpu) => {
                    assert_eq!(cpu.size_of_memory_space, 0);
                }
                _ => {
                    panic!("Unexpected hob type");
                }
            }
            count += 1;
        });

        assert_eq!(count, 11);

        // free the c array
        manually_free_c_array(c_array_hoblist, length);
    }

    #[test]
    fn test_hob_iterator() {
        // generate some test hobs
        let resource = gen_resource_descriptor();
        let handoff = gen_phase_handoff_information_table();
        let firmware_volume = gen_firmware_volume();
        let firmware_volume2 = gen_firmware_volume2();
        let firmware_volume3 = gen_firmware_volume3();
        let capsule = gen_capsule();
        let guid_hob = gen_guid_hob();
        let memory_allocation = gen_memory_allocation();
        let memory_allocation_module = gen_memory_allocation_module();
        let cpu = gen_cpu();
        let end_of_hob_list = gen_end_of_hoblist();

        // create a new hoblist
        let mut hoblist = HobList::new();

        // Push the resource descriptor to the hoblist
        hoblist.push(Hob::ResourceDescriptor(&resource));
        hoblist.push(Hob::Handoff(&handoff));
        hoblist.push(Hob::FirmwareVolume(&firmware_volume));
        hoblist.push(Hob::FirmwareVolume2(&firmware_volume2));
        hoblist.push(Hob::FirmwareVolume3(&firmware_volume3));
        hoblist.push(Hob::Capsule(&capsule));
        hoblist.push(Hob::GuidHob(&guid_hob));
        hoblist.push(Hob::MemoryAllocation(&memory_allocation));
        hoblist.push(Hob::MemoryAllocationModule(&memory_allocation_module));
        hoblist.push(Hob::Cpu(&cpu));
        hoblist.push(Hob::Handoff(&end_of_hob_list));

        let (c_array_hoblist, length) = to_c_array(&hoblist);

        let hob = Hob::ResourceDescriptor(unsafe {
            (c_array_hoblist as *const hob::ResourceDescriptor).as_ref::<'static>().unwrap()
        });
        for h in &hob {
            println!("{:?}", h.header());
        }

        manually_free_c_array(c_array_hoblist, length);
    }
}
