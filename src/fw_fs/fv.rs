//! Firmware Volume (FV) Definitions and Support Code
//!
//! Based on the values defined in the UEFI Platform Initialization (PI) Specification V1.8A 3.1 Firmware Storage
//! Code Definitions.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

pub mod attributes;
pub mod file;
use r_efi::efi;

pub type EfiFvFileType = u8;

/// Firmware Volume Write Policy bit definitions
/// Note: Typically named `EFI_FV_*` in EDK II code.
mod raw {
  pub(super) mod write_policy {
    pub const UNRELIABLE_WRITE: u32 = 0x00000000;
    pub const RELIABLE_WRITE: u32 = 0x00000001;
  }
}

#[repr(u32)]
#[derive(Debug, Copy, Clone, PartialEq)]
pub enum WritePolicy {
  UnreliableWrite = raw::write_policy::UNRELIABLE_WRITE,
  ReliableWrite = raw::write_policy::RELIABLE_WRITE,
}

/// EFI_FIRMWARE_VOLUME_HEADER
#[repr(C)]
#[derive(Debug)]
pub struct Header {
  pub(crate) zero_vector: [u8; 16],
  pub(crate) file_system_guid: efi::Guid,
  pub(crate) fv_length: u64,
  pub(crate) signature: u32,
  pub(crate) attributes: u32,
  pub(crate) header_length: u16,
  pub(crate) checksum: u16,
  pub(crate) ext_header_offset: u16,
  pub(crate) reserved: u8,
  pub(crate) revision: u8,
  pub(crate) block_map: [BlockMapEntry; 0],
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct BlockMapEntry {
  pub num_blocks: u32,
  pub length: u32,
}

/// EFI_FIRMWARE_VOLUME_EXT_HEADER
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub(crate) struct ExtHeader {
  pub(crate) fv_name: efi::Guid,
  pub(crate) ext_header_size: u32,
}
