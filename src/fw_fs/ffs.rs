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
pub mod guid;
pub mod section;

use core::{fmt, mem, slice};

use attributes::raw::LARGE_FILE;
use r_efi::efi;
use section::header::{CommonSectionHeaderExtended, CommonSectionHeaderStandard};
use uuid::Uuid;

use crate::{
  address_helper::align_up,
  fw_fs::{
    ffs::{
      attributes::raw as EfiFfsFileAttributeRaw,
      file::{raw::r#type as FfsFileRawType, Type as FfsFileType},
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

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
enum FfsFileHeader {
  Standard(&'static file::Header),
  Extended(&'static file::Header2),
}

impl FfsFileHeader {
  fn header(&self) -> &'static file::Header {
    match self {
      Self::Standard(header) => header,
      Self::Extended(header) => &header.header,
    }
  }

  fn size(&self) -> u64 {
    match self {
      Self::Standard(header) => {
        let mut size: u64 = 0;
        size += header.size[0] as u64;
        size += (header.size[1] as u64) << 8;
        size += (header.size[2] as u64) << 16;
        size
      }
      Self::Extended(header) => header.extended_size,
    }
  }

  fn data_offset(&self) -> usize {
    match self {
      Self::Standard(_) => mem::size_of::<file::Header>(),
      Self::Extended(_) => mem::size_of::<file::Header2>(),
    }
  }

  fn base_address(&self) -> efi::PhysicalAddress {
    match self {
      Self::Standard(header) => *header as *const file::Header as efi::PhysicalAddress,
      Self::Extended(header2) => *header2 as *const file::Header2 as efi::PhysicalAddress,
    }
  }

  fn data_address(&self) -> efi::PhysicalAddress {
    self.base_address() + self.data_offset() as u64
  }
}

impl From<&'static file::Header> for FfsFileHeader {
  fn from(file: &'static file::Header) -> Self {
    if (file.attributes & LARGE_FILE) != 0 {
      let extended = unsafe { (file as *const file::Header as *const file::Header2).as_ref().unwrap() };
      Self::Extended(extended)
    } else {
      Self::Standard(file)
    }
  }
}

/// Firmware File System (FFS) File
#[derive(Copy, Clone)]
pub struct File {
  containing_fv: FirmwareVolume,
  ffs_file: FfsFileHeader,
}

impl File {
  /// Instantiate a new File structure given the containing volume and base address.
  ///
  /// ## Safety
  /// Caller must ensure that base_address points to the start of a valid FFS header and that it is safe to access
  /// memory from the start of that header to the full length fo the file specified by that header. Caller must also
  /// ensure that the memory containing the file data outlives this File instance.
  ///
  /// Various sanity checks will be performed by this routine to ensure File consistency.
  pub unsafe fn new(
    containing_fv: FirmwareVolume,
    file_base_address: efi::PhysicalAddress,
  ) -> Result<File, efi::Status> {
    if file_base_address < containing_fv.base_address() || containing_fv.top_address() <= file_base_address {
      Err(efi::Status::INVALID_PARAMETER)?;
    }

    let ffs_file = file_base_address as *const file::Header;
    let ffs_file = ffs_file.as_ref().ok_or(efi::Status::INVALID_PARAMETER)?;

    let ffs_file = ffs_file.into();

    Ok(File { containing_fv, ffs_file })
  }

  /// returns the file size (including header)
  pub fn file_size(&self) -> u64 {
    self.ffs_file.size()
  }

  /// returns file data size (not including header)
  pub fn file_data_size(&self) -> u64 {
    self.ffs_file.size() - self.ffs_file.data_offset() as u64
  }

  /// returns the file type
  pub fn file_type(&self) -> Option<FfsFileType> {
    match self.ffs_file.header().file_type {
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

  /// returns the FV File Attributes (see PI spec 1.8A 3.4.1.4)
  pub fn fv_file_attributes(&self) -> EfiFvFileAttributes {
    let attributes = self.ffs_file.header().attributes;
    let data_alignment = (attributes & EfiFfsFileAttributeRaw::DATA_ALIGNMENT) >> 3;
    // decode alignment per Table 3.3 in PI spec 1.8 Part III.
    let mut file_attributes: u32 = match (
      data_alignment,
      (attributes & EfiFfsFileAttributeRaw::DATA_ALIGNMENT_2) == EfiFfsFileAttributeRaw::DATA_ALIGNMENT_2,
    ) {
      (0, false) => 0,
      (1, false) => 4,
      (2, false) => 7,
      (3, false) => 9,
      (4, false) => 10,
      (5, false) => 12,
      (6, false) => 15,
      (7, false) => 16,
      (x @ 0..=7, true) => (17 + x) as u32,
      (_, _) => panic!("Invalid data_alignment!"),
    };
    if attributes & EfiFfsFileAttributeRaw::FIXED != 0 {
      file_attributes |= FvRawAttribute::FIXED;
    }
    file_attributes as EfiFvFileAttributes
  }

  /// returns the GUID filename for this file
  pub fn file_name(&self) -> efi::Guid {
    self.ffs_file.header().name
  }

  /// returns the base address in memory of this file
  pub fn base_address(&self) -> efi::PhysicalAddress {
    self.ffs_file.base_address()
  }

  /// returns the memory address of the end of the file (not inclusive)
  pub fn top_address(&self) -> efi::PhysicalAddress {
    self.base_address() + self.file_size()
  }

  /// returns the file data
  pub fn file_data(&self) -> &[u8] {
    let data_ptr = self.ffs_file.data_address() as *mut u8;
    unsafe { slice::from_raw_parts(data_ptr, self.file_data_size() as usize) }
  }

  /// returns the next file in the FV, if any.
  pub fn next_ffs_file(&self) -> Option<File> {
    let mut next_file_address = self.base_address();
    next_file_address += self.file_size();

    // per the PI spec, "Given a file F, the next file FvHeader is located at the next 8-byte aligned firmware volume
    // offset following the last byte the file F"
    // but, in fact, that just means "8-byte aligned" per the EDK2 implementation.
    next_file_address = align_up(next_file_address, 0x8);

    // check to see if we ran off the end of the FV yet.
    let erase_byte: [u8; 1] =
      if self.containing_fv.attributes() & Fvb2RawAttributes::ERASE_POLARITY != 0 { [0xFF] } else { [0] };
    let remaining_space = self.containing_fv.top_address() - mem::size_of::<FvHeader>() as efi::PhysicalAddress;
    if next_file_address <= remaining_space {
      let test_size = mem::size_of::<FvHeader>().min(remaining_space.try_into().unwrap());
      let remaining_space_slice = unsafe { slice::from_raw_parts(next_file_address as *const u8, test_size) };

      if remaining_space_slice.windows(mem::size_of_val(&erase_byte)).all(|window| window == erase_byte) {
        // No files are left, only erased bytes
        return None;
      }

      let file = unsafe { File::new(self.containing_fv, next_file_address).ok()? };
      // To be super paranoid, we could check a lot of things here to make sure we have a
      // legit file and didn't run into empty space at the end of the FV. For now, assume
      // if the "file_type" is something legit, that the file is good.

      if file.file_type().is_some() {
        return Some(file);
      }
    }
    None
  }

  /// returns the first section of the file (if any)
  pub fn first_ffs_section(&self) -> Option<Section> {
    if self.file_size() <= self.ffs_file.data_offset() as u64 {
      return None;
    }
    let first_section = unsafe { Section::new(*self, self.ffs_file.data_address()).ok()? };
    Some(first_section)
  }

  /// returns an iterator over the sections of the file.
  pub fn ffs_sections(&self) -> impl Iterator<Item = Section> {
    FfsSectionIterator::new(self.first_ffs_section())
  }

  /// returns the raw file type
  pub fn file_type_raw(&self) -> u8 {
    self.ffs_file.header().file_type
  }

  /// returns the raw file attributes
  pub fn file_attributes_raw(&self) -> u8 {
    self.ffs_file.header().attributes
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
  pub fn new(start_file: Option<File>) -> FileIterator {
    FileIterator { next_ffs: start_file }
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

#[derive(Debug, Clone, Copy)]
enum CommonSectionHeader {
  Standard(&'static FfsSectionHeader::CommonSectionHeaderStandard),
  Extended(&'static FfsSectionHeader::CommonSectionHeaderExtended),
}

impl CommonSectionHeader {
  unsafe fn new(base_address: efi::PhysicalAddress) -> Result<CommonSectionHeader, ()> {
    let common_hdr_ptr = (base_address as *const FfsSectionHeader::CommonSectionHeaderStandard).as_ref().ok_or(())?;

    let size = &common_hdr_ptr.size;

    if size.iter().all(|x| *x == 0xff) {
      let extended_hdr_ptr =
        (base_address as *const FfsSectionHeader::CommonSectionHeaderExtended).as_ref().ok_or(())?;
      Ok(CommonSectionHeader::Extended(extended_hdr_ptr))
    } else {
      Ok(CommonSectionHeader::Standard(common_hdr_ptr))
    }
  }

  fn section_type(&self) -> u8 {
    match self {
      CommonSectionHeader::Standard(header) => header.section_type,
      CommonSectionHeader::Extended(header) => header.section_type,
    }
  }

  fn section_size(&self) -> usize {
    match self {
      CommonSectionHeader::Standard(header) => {
        let mut size_bytes = header.size.to_vec();
        size_bytes.push(0);
        let size: u32 = u32::from_le_bytes(size_bytes.try_into().unwrap());
        size as usize
      }
      CommonSectionHeader::Extended(header) => header.extended_size as usize,
    }
  }

  fn base_address(&self) -> efi::PhysicalAddress {
    match *self {
      CommonSectionHeader::Standard(header) => header as *const _ as efi::PhysicalAddress,
      CommonSectionHeader::Extended(header) => header as *const _ as efi::PhysicalAddress,
    }
  }

  fn header_size(&self) -> usize {
    match self {
      CommonSectionHeader::Standard(_) => mem::size_of::<CommonSectionHeaderStandard>(),
      CommonSectionHeader::Extended(_) => mem::size_of::<CommonSectionHeaderExtended>(),
    }
  }
}

/// Section metadata
#[derive(Debug, Clone, Copy)]
pub enum SectionMetaData {
  None,
  Compression(&'static FfsSectionHeader::Compression),
  GuidDefined(&'static FfsSectionHeader::GuidDefined),
  Version(&'static FfsSectionHeader::Version),
  FreeformSubtypeGuid(&'static FfsSectionHeader::FreeformSubtypeGuid),
}

/// Firmware File System (FFS) Section
#[derive(Clone, Copy)]
pub struct Section {
  containing_ffs: File,
  header: CommonSectionHeader,
  meta_data: SectionMetaData,
  data: &'static [u8],
}

impl Section {
  /// Instantiate a new Section structure given the containing file and base address.
  ///
  /// ## Safety
  /// Caller must ensure that base_address points to the start of a valid FFS section header and that it is safe to
  /// access memory from the start of that header to the full length fo the section specified by that header. Caller
  /// must also ensure that the memory containing the section data outlives this Section instance.
  ///
  /// Various sanity checks will be performed by this routine to ensure Section consistency.
  pub unsafe fn new(containing_ffs: File, base_address: efi::PhysicalAddress) -> Result<Section, efi::Status> {
    let header = CommonSectionHeader::new(base_address).map_err(|_| efi::Status::INVALID_PARAMETER)?;

    let (meta_data, data, len) = match header.section_type() {
      FfsSectionRawType::encapsulated::COMPRESSION => {
        let compression = (header.base_address() + header.header_size() as efi::PhysicalAddress)
          as *const FfsSectionHeader::Compression;
        let compression = unsafe { compression.as_ref().ok_or(efi::Status::INVALID_PARAMETER)? };
        let total_header = header.header_size() + mem::size_of::<FfsSectionHeader::Compression>();
        let data = (header.base_address() + total_header as efi::PhysicalAddress) as *const u8;
        let len = header.section_size() - total_header;
        (SectionMetaData::Compression(compression), data, len)
      }
      FfsSectionRawType::encapsulated::GUID_DEFINED => {
        let guid_defined = (header.base_address() + header.header_size() as efi::PhysicalAddress)
          as *const FfsSectionHeader::GuidDefined;
        let guid_defined = unsafe { guid_defined.as_ref().ok_or(efi::Status::INVALID_PARAMETER)? };
        let total_header = header.header_size() + mem::size_of::<FfsSectionHeader::GuidDefined>();
        let data = (header.base_address() + total_header as efi::PhysicalAddress) as *const u8;
        let len = header.section_size() - total_header;
        (SectionMetaData::GuidDefined(guid_defined), data, len)
      }
      FfsSectionRawType::VERSION => {
        let version =
          (header.base_address() + header.header_size() as efi::PhysicalAddress) as *const FfsSectionHeader::Version;
        let version = unsafe { version.as_ref().ok_or(efi::Status::INVALID_PARAMETER)? };
        let total_header = header.header_size() + mem::size_of::<FfsSectionHeader::Version>();
        let data = (header.base_address() + total_header as efi::PhysicalAddress) as *const u8;
        let len = header.section_size() - total_header;
        (SectionMetaData::Version(version), data, len)
      }
      FfsSectionRawType::FREEFORM_SUBTYPE_GUID => {
        let freeform_subtype = (header.base_address() + header.header_size() as efi::PhysicalAddress)
          as *const FfsSectionHeader::FreeformSubtypeGuid;
        let freeform_subtype = unsafe { freeform_subtype.as_ref().ok_or(efi::Status::INVALID_PARAMETER)? };
        let total_header = header.header_size() + mem::size_of::<FfsSectionHeader::FreeformSubtypeGuid>();
        let data = (header.base_address() + total_header as efi::PhysicalAddress) as *const u8;
        let len = header.section_size() - total_header;
        (SectionMetaData::FreeformSubtypeGuid(freeform_subtype), data, len)
      }
      _ => {
        let data = (header.base_address() + header.header_size() as efi::PhysicalAddress) as *const u8;
        let len = header.section_size() - header.header_size();
        (SectionMetaData::None, data, len)
      }
    };

    let data = unsafe { slice::from_raw_parts(data, len) };

    Ok(Section { containing_ffs, header, meta_data, data })
  }

  /// returns the base address in memory of this section
  pub fn base_address(&self) -> efi::PhysicalAddress {
    self.header.base_address()
  }

  /// returns the section type
  pub fn section_type(&self) -> Option<FfsSection::Type> {
    match self.header.section_type() {
      FfsSectionRawType::encapsulated::COMPRESSION => Some(FfsSection::Type::Compression),
      FfsSectionRawType::encapsulated::GUID_DEFINED => Some(FfsSection::Type::GuidDefined),
      FfsSectionRawType::encapsulated::DISPOSABLE => Some(FfsSection::Type::Disposable),
      FfsSectionRawType::PE32 => Some(FfsSection::Type::Pe32),
      FfsSectionRawType::PIC => Some(FfsSection::Type::Pic),
      FfsSectionRawType::TE => Some(FfsSection::Type::Te),
      FfsSectionRawType::DXE_DEPEX => Some(FfsSection::Type::DxeDepex),
      FfsSectionRawType::VERSION => Some(FfsSection::Type::Version),
      FfsSectionRawType::USER_INTERFACE => Some(FfsSection::Type::UserInterface),
      FfsSectionRawType::COMPATIBILITY16 => Some(FfsSection::Type::Compatibility16),
      FfsSectionRawType::FIRMWARE_VOLUME_IMAGE => Some(FfsSection::Type::FirmwareVolumeImage),
      FfsSectionRawType::FREEFORM_SUBTYPE_GUID => Some(FfsSection::Type::FreeformSubtypeGuid),
      FfsSectionRawType::RAW => Some(FfsSection::Type::Raw),
      FfsSectionRawType::PEI_DEPEX => Some(FfsSection::Type::PeiDepex),
      FfsSectionRawType::MM_DEPEX => Some(FfsSection::Type::MmDepex),
      _ => None,
    }
  }

  /// returns the total section size (including the header and metadata, if any)
  pub fn section_size(&self) -> usize {
    self.header.section_size()
  }

  /// returns the section data
  pub fn section_data(&self) -> &[u8] {
    self.data
  }

  /// returns the section metadata
  pub fn metadata(&self) -> SectionMetaData {
    self.meta_data
  }

  /// returns the next section of the containing file
  pub fn next_section(&self) -> Option<Section> {
    let mut next_section_address = self.base_address();
    next_section_address += self.section_size() as efi::PhysicalAddress;

    // per the PI spec, "The section headers aligned on 4 byte boundaries relative to the start of the file's image"
    // but, in fact, that just means "4-byte aligned" per the EDK2 implementation.
    next_section_address = align_up(next_section_address, 0x4);

    // check to see if we ran off the end of the file yet.
    if next_section_address
      <= (self.containing_ffs.top_address()
        - mem::size_of::<FfsSectionHeader::CommonSectionHeaderStandard>() as efi::PhysicalAddress)
    {
      let next_section = unsafe { Section::new(self.containing_ffs, next_section_address).ok()? };
      return Some(next_section);
    }
    None
  }
}

impl fmt::Debug for Section {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    write!(f, "Section @{:#x} header: {:x?} size: 0x{:#x}", self.base_address(), self.header, self.section_size())
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
