//! Firmware File System (FFS) Definitions and Support Code
//!
//! Based on the values defined in the UEFI Platform Initialization (PI) Specification V1.8A Section 3.2.2
//! Firmware File System.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

pub mod attributes;
pub mod file;
pub mod section;

use core::{
  fmt,
  mem::{size_of, size_of_val},
  slice,
};

use r_efi::efi;
use uuid::Uuid;

use crate::{
  address_helper::align_up,
  fw_fs::{
    ffs::{
      attributes::raw as FfsRawAttribute,
      file::{raw::r#type as FfsFileRawType, Header as FfsFileHeader, Type as FfsFileType},
      section as FfsSection,
      section::{header as FfsSectionHeader, raw_type as FfsSectionRawType},
    },
    fv::{
      file::{raw::attribute as FvRawAttribute, EfiFvFileAttributes},
      FirmwareVolume, Header as FvHeader,
    },
    fvb::attributes::raw::fvb2 as Fvb2RawAttributes,
  },
};

#[derive(Copy, Clone)]
/// Firmware File System (FFS) File
pub struct File {
  containing_fv: FirmwareVolume,
  ffs_file: *const FfsFileHeader,
}

impl File {
  pub fn new(containing_fv: FirmwareVolume, base_address: u64) -> File {
    let ffs_file: *const FfsFileHeader = base_address as *const FfsFileHeader;
    File { containing_fv, ffs_file }
  }

  pub fn file_size(&self) -> u64 {
    let mut size: u64 = 0;
    unsafe {
      size += (*self.ffs_file).size[0] as u64;
      size += ((*self.ffs_file).size[1] as u64) << 8;
      size += ((*self.ffs_file).size[2] as u64) << 16;
    }
    size
  }

  pub fn file_data_size(&self) -> usize {
    let file_size = self.file_size() as usize;
    if size_of::<FfsFileHeader>() > file_size {
      0
    } else {
      file_size - size_of::<FfsFileHeader>()
    }
  }

  pub fn file_type_raw(&self) -> u8 {
    unsafe { (*self.ffs_file).file_type }
  }

  pub fn file_type(&self) -> Option<FfsFileType> {
    match self.file_type_raw() {
      FfsFileRawType::RAW => Some(FfsFileType::Raw),
      FfsFileRawType::FREEFORM => Some(FfsFileType::FreeForm),
      FfsFileRawType::SECURITY_CORE => Some(FfsFileType::SecurityCore),
      FfsFileRawType::PEI_CORE => Some(FfsFileType::PeiCore),
      FfsFileRawType::DXE_CORE => Some(FfsFileType::DxeCore),
      FfsFileRawType::PEIM => Some(FfsFileType::Peim),
      FfsFileRawType::DRIVER => Some(FfsFileType::Driver),
      FfsFileRawType::COMBINED_PEIM_DRIVER => Some(FfsFileType::CombinedPeimDriver),
      FfsFileRawType::APPLICATION => Some(FfsFileType::Application),
      FfsFileRawType::MM => Some(FfsFileType::Mm),
      FfsFileRawType::FIRMWARE_VOLUME_IMAGE => Some(FfsFileType::FirmwareVolumeImage),
      FfsFileRawType::COMBINED_MM_DXE => Some(FfsFileType::CombinedMmDxe),
      FfsFileRawType::MM_CORE => Some(FfsFileType::MmCore),
      FfsFileRawType::MM_STANDALONE => Some(FfsFileType::MmStandalone),
      FfsFileRawType::MM_CORE_STANDALONE => Some(FfsFileType::MmCoreStandalone),
      FfsFileRawType::OEM_MIN..=FfsFileRawType::OEM_MAX => Some(FfsFileType::OemMin),
      FfsFileRawType::DEBUG_MIN..=FfsFileRawType::DEBUG_MAX => Some(FfsFileType::DebugMin),
      FfsFileRawType::FFS_PAD => Some(FfsFileType::FfsPad),
      FfsFileRawType::FFS_MIN..=FfsFileRawType::FFS_MAX => Some(FfsFileType::FfsUnknown),
      _ => None,
    }
  }

  pub fn file_attributes(&self) -> EfiFvFileAttributes {
    let attributes = unsafe { (*self.ffs_file).attributes };
    let data_alignment = (attributes & FfsRawAttribute::DATA_ALIGNMENT) >> 3;
    // decode alignment per Table 3.3 in PI spec 1.8 Part III.
    let mut file_attributes: u32 =
      match (data_alignment, (attributes & FfsRawAttribute::DATA_ALIGNMENT_2) == FfsRawAttribute::DATA_ALIGNMENT_2) {
        (0, false) => 0,
        (1, false) => 4,
        (2, false) => 7,
        (3, false) => 9,
        (4, false) => 10,
        (5, false) => 12,
        (6, false) => 15,
        (7, false) => 16,
        (x, true) if (0..8).contains(&x) => (17 + x) as u32,
        (_, _) => panic!("Invalid data_alignment!"),
      };
    if attributes & FfsRawAttribute::FIXED != 0 {
      file_attributes |= FvRawAttribute::FIXED;
    }
    file_attributes as EfiFvFileAttributes
  }

  pub fn file_name(&self) -> efi::Guid {
    unsafe { (*self.ffs_file).name }
  }

  pub fn base_address(&self) -> u64 {
    self.ffs_file as u64
  }

  pub fn top_address(&self) -> u64 {
    self.base_address() + self.file_size()
  }

  pub fn file_data(&self) -> &[u8] {
    let data_ptr = self.base_address() as usize + size_of::<FfsFileHeader>();
    let data_ptr = data_ptr as *mut u8;
    unsafe { slice::from_raw_parts(data_ptr, self.file_data_size()) }
  }

  pub fn next_ffs_file(&self) -> Option<File> {
    let mut next_file_address = self.ffs_file as u64;
    next_file_address += self.file_size();

    // per the PI spec, "Given a file F, the next file FvHeader is located at the next 8-byte aligned firmware volume
    // offset following the last byte the file F"
    // but, in fact, that just means "8-byte aligned" per the EDK2 implementation.
    next_file_address = align_up(next_file_address, 0x8);

    // check to see if we ran off the end of the FV yet.
    let erase_byte: [u8; 1] =
      if self.containing_fv.attributes() & Fvb2RawAttributes::ERASE_POLARITY != 0 { [0xFF] } else { [0] };
    let remaining_space = self.containing_fv.top_address() - size_of::<FvHeader>() as u64;
    if next_file_address <= remaining_space {
      let test_size = size_of::<FvHeader>().min(remaining_space.try_into().unwrap());
      let remaining_space_slice = unsafe { slice::from_raw_parts(next_file_address as *const u8, test_size) };

      if remaining_space_slice.windows(size_of_val(&erase_byte)).all(|window| window == erase_byte) {
        // No files are left, only erased bytes
        return None;
      }

      let file = File::new(self.containing_fv, next_file_address);
      // To be super paranoid, we could check a lot of things here to make sure we have a
      // legit file and didn't run into empty space at the end of the FV. For now, assume
      // if the "file_type" is something legit, that the file is good.

      if file.file_type().is_some() {
        return Some(file);
      }
    }
    None
  }

  pub fn first_ffs_section(&self) -> Option<Section> {
    if self.file_size() <= size_of::<FfsFileHeader>() as u64 {
      return None;
    }
    Some(Section::new(*self, self.base_address() + size_of::<FfsFileHeader>() as u64))
  }

  pub fn ffs_sections(&self) -> impl Iterator<Item = Section> {
    FfsSectionIterator::new(self.first_ffs_section())
  }
}

impl fmt::Debug for File {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    write!(
      f,
      "File @{:#x} type: {:?} name: {:?} size: {:?}",
      self.base_address(),
      self.file_type(),
      Uuid::from_bytes_le(*self.file_name().as_bytes()),
      self.file_size()
    )
  }
}

pub(crate) struct FileIterator {
  next_ffs: Option<File>,
}

impl FileIterator {
  pub fn new(start_file: File) -> FileIterator {
    FileIterator { next_ffs: Some(start_file) }
  }
}

impl Iterator for FileIterator {
  type Item = File;
  fn next(&mut self) -> Option<File> {
    let current = self.next_ffs?;
    self.next_ffs = current.next_ffs_file();
    Some(current)
  }
}

#[derive(Copy, Clone)]
pub struct Section {
  containing_ffs: File,
  common_header: *const FfsSectionHeader::Common,
  ffs_section: FfsSection::Generic,
}

impl Section {
  pub fn new(containing_ffs: File, base_address: u64) -> Section {
    let common_header: *const FfsSectionHeader::Common = base_address as *const FfsSectionHeader::Common;
    Section {
      containing_ffs,
      common_header,
      ffs_section: match unsafe { (*common_header).section_type } {
        FfsSectionRawType::encapsulated::COMPRESSION => {
          FfsSection::Generic::Compression(common_header as *const FfsSectionHeader::Compression)
        }
        FfsSectionRawType::encapsulated::GUID_DEFINED => {
          FfsSection::Generic::GuidDefined(common_header as *const FfsSectionHeader::GuidDefined)
        }
        FfsSectionRawType::encapsulated::DISPOSABLE => {
          FfsSection::Generic::Disposable(common_header as *const FfsSectionHeader::Disposable)
        }
        FfsSectionRawType::PE32 => FfsSection::Generic::Pe32(common_header as *const FfsSectionHeader::Pe32),
        FfsSectionRawType::PIC => FfsSection::Generic::Pic(common_header as *const FfsSectionHeader::Pic),
        FfsSectionRawType::TE => FfsSection::Generic::Te(common_header as *const FfsSectionHeader::Te),
        FfsSectionRawType::DXE_DEPEX => {
          FfsSection::Generic::DxeDepex(common_header as *const FfsSectionHeader::DxeDepex)
        }
        FfsSectionRawType::VERSION => FfsSection::Generic::Version(common_header as *const FfsSectionHeader::Version),
        FfsSectionRawType::USER_INTERFACE => {
          FfsSection::Generic::UserInterface(common_header as *const FfsSectionHeader::UserInterface)
        }
        FfsSectionRawType::COMPATIBILITY16 => {
          FfsSection::Generic::Compatibility16(common_header as *const FfsSectionHeader::Compatibility16)
        }
        FfsSectionRawType::FIRMWARE_VOLUME_IMAGE => {
          FfsSection::Generic::FirmwareVolumeImage(common_header as *const FfsSectionHeader::FirmwareVolumeImage)
        }
        FfsSectionRawType::FREEFORM_SUBTYPE_GUID => {
          FfsSection::Generic::FreeformSubtypeGuid(common_header as *const FfsSectionHeader::FreeformSubtypeGuid)
        }
        FfsSectionRawType::RAW => FfsSection::Generic::Raw(common_header as *const FfsSectionHeader::Raw),
        FfsSectionRawType::PEI_DEPEX => {
          FfsSection::Generic::PeiDepex(common_header as *const FfsSectionHeader::PeiDepex)
        }
        FfsSectionRawType::MM_DEPEX => FfsSection::Generic::MmDepex(common_header as *const FfsSectionHeader::MmDepex),
        _ => FfsSection::Generic::Unknown(common_header),
      },
    }
  }

  pub fn is_pe32(&self) -> bool {
    matches!(&self.ffs_section, FfsSection::Generic::Pe32(_))
  }

  pub fn base_address(&self) -> u64 {
    self.common_header as u64
  }

  pub fn section_type(&self) -> Option<FfsSection::Type> {
    match self.ffs_section {
      FfsSection::Generic::Compression(_) => Some(FfsSection::Type::Compression),
      FfsSection::Generic::GuidDefined(_) => Some(FfsSection::Type::GuidDefined),
      FfsSection::Generic::Disposable(_) => Some(FfsSection::Type::Disposable),
      FfsSection::Generic::Pe32(_) => Some(FfsSection::Type::Pe32),
      FfsSection::Generic::Pic(_) => Some(FfsSection::Type::Pic),
      FfsSection::Generic::Te(_) => Some(FfsSection::Type::Te),
      FfsSection::Generic::DxeDepex(_) => Some(FfsSection::Type::DxeDepex),
      FfsSection::Generic::Version(_) => Some(FfsSection::Type::Version),
      FfsSection::Generic::UserInterface(_) => Some(FfsSection::Type::UserInterface),
      FfsSection::Generic::Compatibility16(_) => Some(FfsSection::Type::Compatibility16),
      FfsSection::Generic::FirmwareVolumeImage(_) => Some(FfsSection::Type::FirmwareVolumeImage),
      FfsSection::Generic::FreeformSubtypeGuid(_) => Some(FfsSection::Type::FreeformSubtypeGuid),
      FfsSection::Generic::Raw(_) => Some(FfsSection::Type::Raw),
      FfsSection::Generic::PeiDepex(_) => Some(FfsSection::Type::PeiDepex),
      FfsSection::Generic::MmDepex(_) => Some(FfsSection::Type::MmDepex),
      _ => None,
    }
  }

  pub fn section_size(&self) -> u64 {
    let mut size: u64 = 0;
    unsafe {
      size += (*self.common_header).size[0] as u64;
      size += ((*self.common_header).size[1] as u64) << 8;
      size += ((*self.common_header).size[2] as u64) << 16;
    }
    size
  }

  pub fn section_data(&self) -> &[u8] {
    let data_offset = match self.ffs_section {
      FfsSection::Generic::Compression(_) => size_of::<FfsSectionHeader::Compression>() as u64,
      FfsSection::Generic::FreeformSubtypeGuid(_) => size_of::<FfsSectionHeader::FreeformSubtypeGuid>() as u64,
      FfsSection::Generic::GuidDefined(guid_defined) => unsafe { (*guid_defined).data_offset as u64 },
      _ => size_of::<FfsSectionHeader::Common>() as u64,
    };

    let data_start_addr = self.base_address() + data_offset;
    let data_size = self.section_size() - data_offset;

    unsafe { slice::from_raw_parts(data_start_addr as *const u8, data_size as usize) }
  }

  pub fn next_section(&self) -> Option<Section> {
    let mut next_section_address = self.common_header as u64;
    next_section_address += self.section_size();

    // per the PI spec, "The section headers aligned on 4 byte boundaries relative to the start of the file's image"
    // but, in fact, that just means "4-byte aligned" per the EDK2 implementation.
    next_section_address = align_up(next_section_address, 0x4);

    // check to see if we ran off the end of the file yet.
    if next_section_address <= (self.containing_ffs.top_address() - size_of::<FfsSectionHeader::Common>() as u64) {
      return Some(Section::new(self.containing_ffs, next_section_address));
    }
    None
  }
}

impl fmt::Debug for Section {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    write!(f, "Section @{:#x} type: {:?} size: {:?}", self.base_address(), self.section_type(), self.section_size())
  }
}

struct FfsSectionIterator {
  next_section: Option<Section>,
}

impl FfsSectionIterator {
  pub fn new(start_section: Option<Section>) -> FfsSectionIterator {
    FfsSectionIterator { next_section: start_section }
  }
}

impl Iterator for FfsSectionIterator {
  type Item = Section;
  fn next(&mut self) -> Option<Section> {
    let current = self.next_section?;
    self.next_section = current.next_section();
    Some(current)
  }
}
