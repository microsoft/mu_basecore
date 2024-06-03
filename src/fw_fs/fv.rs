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

extern crate alloc;

use crate::address_helper::align_up;
use alloc::string::ToString;
use core::{fmt, slice};
use r_efi::efi::Guid;
use uuid::Uuid;

use crate::fw_fs::{
  ffs::{File as FfsFile, FileIterator as FfsFileIterator},
  fvb::attributes::EfiFvbAttributes2,
};

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
  pub(crate) file_system_guid: Guid,
  pub(crate) fv_length: u64,
  pub(crate) signature: u32,
  pub(crate) attributes: EfiFvbAttributes2,
  pub(crate) header_length: u16,
  pub(crate) checksum: u16,
  pub(crate) ext_header_offset: u16,
  pub(crate) reserved: u8,
  pub(crate) revision: u8,
  // Block map starts here
}

#[repr(C)]
#[derive(Debug)]
pub(crate) struct BlockMapEntry {
  pub(crate) num_blocks: u32,
  pub(crate) length: u32,
}

/// EFI_FIRMWARE_VOLUME_EXT_HEADER
#[repr(C)]
#[derive(Debug)]
pub(crate) struct ExtHeader {
  pub(crate) fv_name: Guid,
  pub(crate) ext_header_size: u32,
}

#[derive(Copy, Clone)]
pub struct FirmwareVolume {
  fv_header: *const self::Header,
}

impl FirmwareVolume {
  pub fn new(base_address: u64) -> FirmwareVolume {
    let fv_header = base_address as *const self::Header;
    // Note: This assumes that base_address points to something that is an FV with an FFS on it.
    // More robust code would evaluate the FV header file_system_guid and make sure it actually has the correct
    // filesystem type, and probably a number of other sanity checks.
    FirmwareVolume { fv_header }
  }

  fn ext_header(&self) -> Option<*const ExtHeader> {
    let ext_header_offset = unsafe { (*self.fv_header).ext_header_offset as u64 };
    if ext_header_offset == 0 {
      return None;
    }
    Some((self.base_address() + ext_header_offset) as *const ExtHeader)
  }

  fn block_map(&self) -> &[BlockMapEntry] {
    let block_map_start = unsafe { self.fv_header.offset(1) as *const BlockMapEntry };
    let mut count = 0;
    let mut current_block_map_ptr = block_map_start;

    unsafe {
      while (*current_block_map_ptr).num_blocks != 0 && (*current_block_map_ptr).length != 0 {
        count += 1;
        current_block_map_ptr = current_block_map_ptr.add(1);
      }
      slice::from_raw_parts(block_map_start, count)
    }
  }

  pub fn fv_name(&self) -> Option<Guid> {
    if let Some(ext_header) = self.ext_header() {
      return unsafe { Some((*ext_header).fv_name) };
    }
    None
  }

  pub fn first_ffs_file(&self) -> FfsFile {
    let mut ffs_address = self.fv_header as u64;
    if let Some(ext_header) = self.ext_header() {
      // if ext header exists, then file starts after ext header
      unsafe {
        ffs_address += (*self.fv_header).ext_header_offset as u64;
        ffs_address += (*ext_header).ext_header_size as u64;
      }
    } else {
      // otherwise the file starts after the main header.
      unsafe { ffs_address += (*self.fv_header).header_length as u64 }
    }
    ffs_address = align_up(ffs_address, 0x8);
    // Note: it appears possible from the EDK2 implementation that an FV could have a file system with no actual
    // files.
    // More robust code would check and handle that case.
    FfsFile::new(*self, ffs_address)
  }

  pub fn ffs_files(&self) -> impl Iterator<Item = FfsFile> {
    FfsFileIterator::new(self.first_ffs_file())
  }

  pub fn base_address(&self) -> u64 {
    self.fv_header as u64
  }

  pub fn top_address(&self) -> u64 {
    unsafe { self.base_address() + (*self.fv_header).fv_length }
  }

  pub fn attributes(&self) -> EfiFvbAttributes2 {
    unsafe { (*self.fv_header).attributes }
  }

  pub fn get_lba_info(&self, lba: u32) -> Result<(u32, u32, u32), ()> {
    let block_map = self.block_map();

    let mut total_blocks = 0;
    let mut offset = 0;
    let mut block_size = 0;

    for entry in block_map {
      total_blocks += entry.num_blocks;
      block_size = entry.length;
      if lba < total_blocks {
        break;
      }
      offset += entry.num_blocks * entry.length;
    }

    if lba >= total_blocks {
      return Err(()); //lba out of range.
    }

    let remaining_blocks = total_blocks - lba;
    Ok((offset + lba * block_size, block_size, remaining_blocks))
  }
}

impl fmt::Debug for FirmwareVolume {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    write!(
      f,
      "FirmwareVolume@{:#x}-{:#x} name: {:}",
      self.base_address(),
      self.top_address(),
      match self.fv_name() {
        Some(guid) => Uuid::from_bytes_le(*guid.as_bytes()).to_string(),
        None => "Unspecified".to_string(),
      }
    )
  }
}

#[cfg(test)]
mod unit_tests {
  use std::{
    collections::HashMap,
    env,
    error::Error,
    fs::{self, File},
    path::Path,
  };

  use core::slice;
  use serde::Deserialize;
  use uuid::Uuid;

  use crate::fw_fs::{
    ffs::{file::raw::r#type as FfsRawFileType, section::Type as FfsSectionType, Section as FfsSection},
    fv::FirmwareVolume,
  };

  #[derive(Debug, Deserialize)]
  struct TargetValues {
    total_number_of_files: u32,
    files_to_test: HashMap<String, FfsFileTargetValues>,
  }

  #[derive(Debug, Deserialize)]
  struct FfsFileTargetValues {
    base_address: u64,
    file_type: u8,
    attributes: u32,
    size: u64,
    data_size: usize,
    number_of_sections: usize,
    sections: HashMap<usize, FfsSectionTargetValues>,
  }

  #[derive(Debug, Deserialize)]
  struct FfsSectionTargetValues {
    base_address: u64,
    section_type: Option<FfsSectionType>,
    size: u64,
    text: Option<String>,
  }

  #[test]
  fn trivial_unit_test() {
    assert_eq!(FfsRawFileType::ALL, 0x00);
  }

  #[test]
  fn test_firmware_volume() -> Result<(), Box<dyn Error>> {
    let root = Path::new(&env::var("CARGO_MANIFEST_DIR")?).join("test_resources");

    let fv_bytes = fs::read(root.join("DXEFV.Fv"))?;
    let fv = FirmwareVolume::new(fv_bytes.as_ptr() as u64);

    let mut expected_values =
      serde_yaml::from_reader::<File, TargetValues>(File::open(root.join("DXEFV_expected_values.yml"))?)?;

    let mut count = 0;
    for ffs_file in fv.ffs_files() {
      count += 1;
      let file_name = Uuid::from_bytes_le(*ffs_file.file_name().as_bytes()).to_string().to_uppercase();
      let sections = ffs_file.ffs_sections().collect::<Vec<_>>();
      if let Some(mut target) = expected_values.files_to_test.remove(&file_name) {
        assert_eq!(
          target.base_address,
          ffs_file.base_address() - fv_bytes.as_ptr() as u64,
          "[{file_name}] Error with the file Base Address"
        );
        assert_eq!(target.file_type, ffs_file.file_type_raw(), "[{file_name}] Error with the file type.");
        assert_eq!(target.attributes, ffs_file.file_attributes(), "[{file_name}] Error with the file attributes.");
        assert_eq!(
          target.data_size,
          ffs_file.file_data_size(),
          "[{file_name}] Error with the file data size (Body size)."
        );
        assert_eq!(target.size, ffs_file.file_size(), "[{file_name}] Error with the file size (Full size).");
        assert_eq!(
          target.number_of_sections,
          sections.len(),
          "[{file_name}] Error with the number of section in the File"
        );

        for (idx, section) in sections.iter().enumerate() {
          if let Some(target) = target.sections.remove(&idx) {
            assert_eq!(
              target.base_address,
              section.base_address() - fv_bytes.as_ptr() as u64,
              "[{file_name}, section: {idx}] Error with the section Base Address"
            );
            assert_eq!(
              target.section_type,
              section.section_type(),
              "[{file_name}, section: {idx}] Error with the section Type"
            );
            assert_eq!(
              target.size,
              section.section_size(),
              "[{file_name}, section: {idx}] Error with the section Size"
            );
            assert_eq!(
              target.text,
              extract_text_from_section(section),
              "[{file_name}, section: {idx}] Error with the section Text"
            );
          }
        }

        assert!(target.sections.is_empty(), "Some section use case has not been run.");
      }
    }
    assert_eq!(
      expected_values.total_number_of_files, count,
      "The number of file found does not match the expected one."
    );
    assert!(expected_values.files_to_test.is_empty(), "Some file use case has not been run.");
    Ok(())
  }

  fn extract_text_from_section(section: &FfsSection) -> Option<String> {
    if section.section_type() == Some(FfsSectionType::UserInterface) {
      let data = section.section_data();
      let display_name = unsafe { slice::from_raw_parts(data.as_ptr() as *const u16, (data.len() / 2) - 1) };
      Some(String::from_utf16_lossy(display_name))
    } else {
      None
    }
  }
}
