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
use core::{fmt, mem, num::Wrapping, slice};
use r_efi::efi;
use uuid::Uuid;

use crate::fw_fs::{
  ffs::{File as FfsFile, FileIterator as FfsFileIterator},
  fvb::attributes::EfiFvbAttributes2,
};

use super::ffs::{
  file::Header as FfsFileHeader,
  guid::{EFI_FIRMWARE_FILE_SYSTEM2_GUID, EFI_FIRMWARE_FILE_SYSTEM3_GUID},
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
  pub(crate) file_system_guid: efi::Guid,
  pub(crate) fv_length: u64,
  pub(crate) signature: u32,
  pub(crate) attributes: EfiFvbAttributes2,
  pub(crate) header_length: u16,
  pub(crate) checksum: u16,
  pub(crate) ext_header_offset: u16,
  pub(crate) reserved: u8,
  pub(crate) revision: u8,
  pub(crate) block_map: [BlockMapEntry; 0],
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
  pub(crate) fv_name: efi::Guid,
  pub(crate) ext_header_size: u32,
}

/// Firmware Volume
#[derive(Copy, Clone)]
pub struct FirmwareVolume {
  fv_header: &'static Header,
}

impl FirmwareVolume {
  /// Instantiate a new FirmwareVolume structure given the base address of an FV in memory.
  ///
  /// ## Safety
  /// Caller must ensure that base_address points to the start of a valid FV header and that it is safe to access memory
  /// from the start of that header to the full length specified by the FV header. Caller must also ensure that the
  /// memory containing the FV outlives the FirmwareVolume instance.
  ///
  /// Various sanity checks will be performed by this routine to ensure FV consistency.
  pub unsafe fn new(base_address: efi::PhysicalAddress) -> Result<FirmwareVolume, efi::Status> {
    let fv_header = base_address as *const self::Header;

    // sanity checks

    // signature: must be ASCII '_FVH'
    if (*fv_header).signature != 0x4856465f {
      //'_FVH'
      Err(efi::Status::INVALID_PARAMETER)?;
    }

    // header_length: must be large enough to hold the header.
    if (*fv_header).header_length < mem::size_of::<Header>().try_into().unwrap() {
      Err(efi::Status::INVALID_PARAMETER)?;
    }

    // checksum: fv header must sum to zero (and must be multiple of 2 bytes)
    if (*fv_header).header_length & 0x01 != 0 {
      Err(efi::Status::INVALID_PARAMETER)?;
    }
    let header_size = (*fv_header).header_length / (mem::size_of::<u16>() as u16);
    if Wrapping(0u16)
      != slice::from_raw_parts(fv_header as *const u16, header_size as usize).iter().map(|&x| Wrapping(x)).sum()
    {
      Err(efi::Status::INVALID_PARAMETER)?;
    }

    // revision: must be at least 2. Assumes that if later specs bump the rev they will maintain
    // backwards compat with existing header definition.
    if (*fv_header).revision < 2 {
      Err(efi::Status::INVALID_PARAMETER)?;
    }

    // file_system_guid: must be EFI_FIRMWARE_FILE_SYSTEM2_GUID or EFI_FIRMWARE_FILE_SYSTEM3_GUID.
    if (*fv_header).file_system_guid != EFI_FIRMWARE_FILE_SYSTEM2_GUID
      && (*fv_header).file_system_guid != EFI_FIRMWARE_FILE_SYSTEM3_GUID
    {
      Err(efi::Status::INVALID_PARAMETER)?;
    }

    // fv_length: must be large enough to hold the header.
    if (*fv_header).fv_length < (*fv_header).header_length as u64 {
      Err(efi::Status::INVALID_PARAMETER)?;
    }

    //ext_header_offset: must be inside the fv
    if (*fv_header).ext_header_offset as u64 > (*fv_header).fv_length {
      Err(efi::Status::INVALID_PARAMETER)?;
    }

    Ok(FirmwareVolume { fv_header: fv_header.as_ref().unwrap() })
  }

  fn ext_header(&self) -> Option<&'static ExtHeader> {
    if self.fv_header.ext_header_offset == 0 {
      return None;
    }
    unsafe {
      ((self.base_address() + self.fv_header.ext_header_offset as efi::PhysicalAddress) as *const ExtHeader).as_ref()
    }
  }

  fn block_map(&self) -> &'static [BlockMapEntry] {
    let block_map_start = self.fv_header.block_map.as_ptr();
    let mut count = 0;
    let mut current_block_map_ptr = block_map_start;

    //the block map is terminated by an entry with num_blocks = 0 and length = 0.
    unsafe {
      while (*current_block_map_ptr).num_blocks != 0 && (*current_block_map_ptr).length != 0 {
        count += 1;
        current_block_map_ptr = current_block_map_ptr.add(1);
      }
      slice::from_raw_parts(block_map_start, count)
    }
  }

  /// Returns the GUID name of the Firmware Volume
  pub fn fv_name(&self) -> Option<efi::Guid> {
    if let Some(ext_header) = self.ext_header() {
      return Some(ext_header.fv_name);
    }
    None
  }

  /// Returns the first file in the Firmware Volume
  pub fn first_ffs_file(&self) -> Option<FfsFile> {
    let mut ffs_address = self.base_address();
    if let Some(ext_header) = self.ext_header() {
      // if ext header exists, then file starts after ext header
      ffs_address += self.fv_header.ext_header_offset as efi::PhysicalAddress;
      ffs_address += ext_header.ext_header_size as efi::PhysicalAddress;
    } else {
      // otherwise the file starts after the main header.
      ffs_address += self.fv_header.header_length as efi::PhysicalAddress
    }
    ffs_address = align_up(ffs_address, 0x8);

    //Check the case where the FV contains no files.
    if ffs_address + mem::size_of::<FfsFileHeader>() as efi::PhysicalAddress >= self.top_address() {
      None
    } else {
      unsafe { FfsFile::new(*self, ffs_address).ok() }
    }
  }

  /// Returns an iterator over all files in the firmware volume.
  pub fn ffs_files(&self) -> impl Iterator<Item = FfsFile> {
    FfsFileIterator::new(self.first_ffs_file())
  }

  /// Returns the base address in memory of the firmware volume.
  pub fn base_address(&self) -> efi::PhysicalAddress {
    self.fv_header as *const Header as efi::PhysicalAddress
  }

  /// returns the memory address of the end of the firmware volume (not inclusive)
  pub fn top_address(&self) -> efi::PhysicalAddress {
    self.base_address() + self.fv_header.fv_length
  }

  /// returns the Firmware Volume Attributes
  pub fn attributes(&self) -> EfiFvbAttributes2 {
    self.fv_header.attributes
  }

  /// returns the (linear block offset from FV base, block_size, remaining_blocks) given an LBA.
  pub fn get_lba_info(&self, lba: u32) -> Result<(u32, u32, u32), efi::Status> {
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
      return Err(efi::Status::INVALID_PARAMETER); //lba out of range.
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

  use core::{mem, slice};
  use r_efi::efi;
  use serde::Deserialize;
  use uuid::Uuid;

  use crate::fw_fs::{
    ffs::{file::raw::r#type as FfsRawFileType, section::Type as FfsSectionType, Section as FfsSection},
    fv::{BlockMapEntry, FirmwareVolume},
  };

  use super::Header;

  #[derive(Debug, Deserialize)]
  struct TargetValues {
    total_number_of_files: u32,
    files_to_test: HashMap<String, FfsFileTargetValues>,
  }

  #[derive(Debug, Deserialize)]
  struct FfsFileTargetValues {
    base_address: u64,
    file_type: u8,
    attributes: u8,
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

  fn test_firmware_volume_worker(
    fv_bytes: Vec<u8>,
    fv: FirmwareVolume,
    mut expected_values: TargetValues,
  ) -> Result<(), Box<dyn Error>> {
    let mut count = 0;
    for ffs_file in fv.ffs_files() {
      count += 1;
      let file_name = Uuid::from_bytes_le(*ffs_file.file_name().as_bytes()).to_string().to_uppercase();
      let sections = ffs_file.ffs_sections().collect::<Vec<_>>();
      if let Some(mut target) = expected_values.files_to_test.remove(&file_name) {
        assert_eq!(
          target.base_address,
          ffs_file.base_address() - fv_bytes.as_ptr() as efi::PhysicalAddress,
          "[{file_name}] Error with the file Base Address"
        );
        assert_eq!(target.file_type, ffs_file.file_type_raw(), "[{file_name}] Error with the file type.");
        assert_eq!(target.attributes, ffs_file.file_attributes_raw(), "[{file_name}] Error with the file attributes.");
        assert_eq!(target.size, ffs_file.file_size(), "[{file_name}] Error with the file size (Full size).");
        assert_eq!(
          target.data_size,
          ffs_file.file_data_size() as usize,
          "[{file_name}] Error with the file data size (Body size)."
        );
        assert_eq!(
          target.number_of_sections,
          sections.len(),
          "[{file_name}] Error with the number of section in the File"
        );

        for (idx, section) in sections.iter().enumerate() {
          if let Some(target) = target.sections.remove(&idx) {
            assert_eq!(
              target.base_address,
              section.base_address() - fv_bytes.as_ptr() as efi::PhysicalAddress,
              "[{file_name}, section: {idx}] Error with the section Base Address"
            );
            assert_eq!(
              target.section_type,
              section.section_type(),
              "[{file_name}, section: {idx}] Error with the section Type"
            );
            assert_eq!(
              target.size,
              section.section_size() as u64,
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

  #[test]
  fn test_firmware_volume() -> Result<(), Box<dyn Error>> {
    let root = Path::new(&env::var("CARGO_MANIFEST_DIR")?).join("test_resources");

    let fv_bytes = fs::read(root.join("DXEFV.Fv"))?;
    let fv = unsafe { FirmwareVolume::new(fv_bytes.as_ptr() as efi::PhysicalAddress).unwrap() };

    let expected_values =
      serde_yaml::from_reader::<File, TargetValues>(File::open(root.join("DXEFV_expected_values.yml"))?)?;

    test_firmware_volume_worker(fv_bytes, fv, expected_values)
  }

  #[test]
  fn test_giant_firmware_volume() -> Result<(), Box<dyn Error>> {
    let root = Path::new(&env::var("CARGO_MANIFEST_DIR")?).join("test_resources");

    let fv_bytes = fs::read(root.join("GIGANTOR.Fv"))?;
    let fv = unsafe { FirmwareVolume::new(fv_bytes.as_ptr() as efi::PhysicalAddress).unwrap() };

    let expected_values =
      serde_yaml::from_reader::<File, TargetValues>(File::open(root.join("GIGANTOR_expected_values.yml"))?)?;

    test_firmware_volume_worker(fv_bytes, fv, expected_values)
  }

  #[test]
  fn test_malformed_firmware_volume() -> Result<(), Box<dyn Error>> {
    let root = Path::new(&env::var("CARGO_MANIFEST_DIR")?).join("test_resources");

    // bogus signature.
    let mut fv_bytes = fs::read(root.join("DXEFV.Fv"))?;
    let fv_header = fv_bytes.as_mut_ptr() as *mut Header;
    unsafe {
      (*fv_header).signature ^= 0xdeadbeef;
      FirmwareVolume::new(fv_bytes.as_ptr() as efi::PhysicalAddress).unwrap_err()
    };

    // bogus header_length.
    let mut fv_bytes = fs::read(root.join("DXEFV.Fv"))?;
    let fv_header = fv_bytes.as_mut_ptr() as *mut Header;
    unsafe {
      (*fv_header).header_length = 0;
      FirmwareVolume::new(fv_bytes.as_ptr() as efi::PhysicalAddress).unwrap_err()
    };

    // bogus checksum.
    let mut fv_bytes = fs::read(root.join("DXEFV.Fv"))?;
    let fv_header = fv_bytes.as_mut_ptr() as *mut Header;
    unsafe {
      (*fv_header).checksum ^= 0xbeef;
      FirmwareVolume::new(fv_bytes.as_ptr() as efi::PhysicalAddress).unwrap_err()
    };

    // bogus revision.
    let mut fv_bytes = fs::read(root.join("DXEFV.Fv"))?;
    let fv_header = fv_bytes.as_mut_ptr() as *mut Header;
    unsafe {
      (*fv_header).revision = 1;
      FirmwareVolume::new(fv_bytes.as_ptr() as efi::PhysicalAddress).unwrap_err()
    };

    // bogus filesystem guid.
    let mut fv_bytes = fs::read(root.join("DXEFV.Fv"))?;
    let fv_header = fv_bytes.as_mut_ptr() as *mut Header;
    unsafe {
      (*fv_header).file_system_guid = efi::Guid::from_bytes(&[0xa5; 16]);
      FirmwareVolume::new(fv_bytes.as_ptr() as efi::PhysicalAddress).unwrap_err()
    };

    // bogus fv length.
    let mut fv_bytes = fs::read(root.join("DXEFV.Fv"))?;
    let fv_header = fv_bytes.as_mut_ptr() as *mut Header;
    unsafe {
      (*fv_header).fv_length = 0;
      FirmwareVolume::new(fv_bytes.as_ptr() as efi::PhysicalAddress).unwrap_err()
    };

    // bogus ext header offset.
    let mut fv_bytes = fs::read(root.join("DXEFV.Fv"))?;
    let fv_header = fv_bytes.as_mut_ptr() as *mut Header;
    unsafe {
      (*fv_header).fv_length = ((*fv_header).ext_header_offset - 1) as u64;
      FirmwareVolume::new(fv_bytes.as_ptr() as efi::PhysicalAddress).unwrap_err()
    };

    Ok(())
  }

  #[test]
  fn zero_size_block_map_gives_same_offset_as_no_block_map() {
    //code in FirmwareVolume::block_map() assumes that the size of a struct that ends in a zero-size array is the same
    //as an identical struct that doesn't have the array at all. This unit test validates that assumption.
    #[repr(C)]
    struct A {
      foo: usize,
      bar: u32,
      baz: u32,
      block_map: [BlockMapEntry; 0],
    }

    #[repr(C)]
    struct B {
      foo: usize,
      bar: u32,
      baz: u32,
    }
    assert_eq!(mem::size_of::<A>(), mem::size_of::<B>());

    let a = A { foo: 0, bar: 0, baz: 0, block_map: [BlockMapEntry { length: 0, num_blocks: 0 }; 0] };

    let a_ptr = &a as *const A;

    unsafe {
      assert_eq!((&(*a_ptr).block_map).as_ptr(), a_ptr.offset(1) as *const BlockMapEntry);
    }
  }
}
