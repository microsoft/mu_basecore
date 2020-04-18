// @file -- loader_context.rs
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#![cfg_attr(not(test), no_std)]
#![allow(unused)]

// Enable printing in both test and build.
// Ewwww...
#[cfg(test)]
extern crate std;
#[cfg(test)]
use std::{println, eprintln};
#[cfg(not(test))]
use uefi_rust_print_lib_debug_lib::{println, eprintln};

use alloc::vec::Vec;
use alloc::boxed::Box;
use r_efi::{efi, base};


// typedef
// RETURN_STATUS
// (EFIAPI *PE_COFF_LOADER_READ_FILE)(
//   IN     VOID   *FileHandle,
//   IN     UINTN  FileOffset,
//   IN OUT UINTN  *ReadSize,
//   OUT    VOID   *Buffer
//   );
type PeCoffLoaderReadFile = extern "win64" fn(_: *const core::ffi::c_void,
                                              _: usize,
                                              _: *mut usize,
                                              _: *mut core::ffi::c_void) -> efi::Status;

#[repr(u32)]
#[derive(Clone,Copy,Debug,PartialEq)]
enum PeCoffImageError {
  ImageErrorSuccess = 0,
  ImageErrorImageRead,
  ImageErrorInvalidPeHeaderSignature,
  ImageErrorInvalidMachineType,
  ImageErrorInvalidSubsystem,
  ImageErrorInvalidImageAddress,
  ImageErrorInvalidImageSize,
  ImageErrorInvalidSectionAlignment,
  ImageErrorSectionNotLoaded,
  ImageErrorFailedRelocation,
  ImageErrorFailedIcacheFlush,
  ImageErrorUnsupported,
}

// REF: MdePkg/Include/Library/PeCoffLib.h
#[repr(C)]
pub struct PeCoffLoaderImageContext {
  image_address:          base::PhysicalAddress,
  image_size:             u64,
  destination_address:    base::PhysicalAddress,
  entry_point:            base::PhysicalAddress,
  image_read:             PeCoffLoaderReadFile,
  handle:                 *const core::ffi::c_void,
  fixup_data:             *const core::ffi::c_void,
  section_alignment:      u32,
  pe_coff_header_offset:  u32,
  debug_directory_entry_rva:  u32,
  code_view:              *const core::ffi::c_void,
  pdb_pointer:            *const u8,
  size_of_headers:        usize,
  image_code_memory_type: u32,
  image_data_memory_type: u32,
  image_error:            PeCoffImageError,
  fixup_data_size:        usize,
  machine:                u16,
  image_type:             u16,
  relocations_stripped:   base::Boolean,
  is_te_image:            base::Boolean,
  hii_resource_data:      base::PhysicalAddress,
  context:                u64
}

// Have to define this manually because of the extern functions.
impl core::fmt::Debug for PeCoffLoaderImageContext {
  fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
    f.debug_struct("PeCoffLoaderImageContext")
      .field("image_address", &self.image_address)
      .field("image_size", &self.image_size)
      .field("destination_address", &self.destination_address)
      .field("entry_point", &self.entry_point)
      .field("image_read", &(self.image_read as *const ()))
      .field("handle", &self.handle)
      .field("fixup_data", &self.fixup_data)
      .field("section_alignment", &self.section_alignment)
      .field("pe_coff_header_offset", &self.pe_coff_header_offset)
      .field("debug_directory_entry_rva", &self.debug_directory_entry_rva)
      .field("code_view", &self.code_view)
      .field("pdb_pointer", &self.pdb_pointer)
      .field("size_of_headers", &self.size_of_headers)
      .field("image_code_memory_type", &self.image_code_memory_type)
      .field("image_data_memory_type", &self.image_data_memory_type)
      .field("image_error", &self.image_error)
      .field("fixup_data_size", &self.fixup_data_size)
      .field("machine", &self.machine)
      .field("image_type", &self.image_type)
      .field("relocations_stripped", &self.relocations_stripped)
      .field("is_te_image", &self.is_te_image)
      .field("hii_resource_data", &self.hii_resource_data)
      .field("context", &self.context)
      .finish()
  }
}

impl PeCoffLoaderImageContext {
  fn new(image_read_fn: PeCoffLoaderReadFile) -> Self {
    Self {
      image_address:          0,
      image_size:             0,
      destination_address:    0,
      entry_point:            0,
      image_read:             image_read_fn,
      handle:                 core::ptr::null(),
      fixup_data:             core::ptr::null(),
      section_alignment:      0,
      pe_coff_header_offset:  0,
      debug_directory_entry_rva:  0,
      code_view:              core::ptr::null(),
      pdb_pointer:            core::ptr::null(),
      size_of_headers:        0,
      image_code_memory_type: 0,
      image_data_memory_type: 0,
      image_error:            PeCoffImageError::ImageErrorSuccess,
      fixup_data_size:        0,
      machine:                0,
      image_type:             0,
      relocations_stripped:   base::Boolean::FALSE,
      is_te_image:            base::Boolean::FALSE,
      hii_resource_data:      0,
      context:                0
    }
  }

  fn is_raw_struture_valid(&self) -> bool {
    // Validate the loaded raw data against fundemental expectations
    // of all callers and consumers.

    // Make sure that the image_read function is populated.
    if (self.image_read as *const ()).is_null() {
      return false;
    }

    // Make sure that at least *some* image_size is set.
    // This library implementation will require at least a size
    // hint, or else it will be guaranteed to fail parsing.
    // TODO: Move this test somewhere else.
    // if self.image_size == 0 {
    //   return false;
    // }

    // Make sure that image_error is a valid enum.
    // if (self.image_error as u32) > (PeCoffImageError::ImageErrorUnsupported as u32) {
    //   return false;
    // }

    true
  }

  unsafe fn from_raw(ptr: *mut Self) -> Result<&'static mut Self, ()> {
    if ptr.is_null() {
      return Err(())
    }

    let image_context = core::mem::transmute::<*mut Self, &'static mut Self>(ptr);

    // image_error is never used for communicating TO this structure, so
    // we can safely initialize it to a known value, rather than bounds-check it.
    image_context.image_error = PeCoffImageError::ImageErrorSuccess;

    if image_context.is_raw_struture_valid() {
      Ok(image_context)
    }
    else {
      Err(())
    }
  }

  fn read_image(&self, offset: usize, size: &mut usize) -> Result<Vec<u8>, ()> {
    let mut buffer = Vec::with_capacity(*size) as Vec<u8>;
    let mut read_size = *size;
    let result = unsafe {
      (self.image_read)(self.handle,
                        offset,
                        &mut read_size,
                        buffer.as_mut_ptr() as *mut core::ffi::c_void)
    };
    match result {
      efi::Status::SUCCESS => {
        if read_size <= *size {
          *size = read_size;
          // If we were successful, we *must* set the length before returning.
          // According to the contract of the function, "size" will have been updated
          // with the bytes actually written.
          unsafe { buffer.set_len(*size) };
          Ok(buffer)
        }
        else {
          Err(())
        }
      },
      _ => Err(())
    }
  }
}

#[cfg(test)]
mod ffi_context_tests {
  use super::*;
  use alloc::vec;
  use core::mem;

  static mut MOCKED_READER_SIZES: Vec<usize> = Vec::new();
  static mut MOCKED_READER_RETURNS: Vec<efi::Status> = Vec::new();
  extern "win64" fn test_mocked_reader(
      file_handle: *const core::ffi::c_void,
      file_offset: usize,
      read_size: *mut usize,
      output_buffer: *mut core::ffi::c_void
      ) -> efi::Status {
    unsafe {
      *read_size = MOCKED_READER_SIZES.remove(0);
      MOCKED_READER_RETURNS.remove(0)
    }
  }

  #[test]
  fn calling_from_raw_on_null_should_fail() {
    unsafe {
      assert!(PeCoffLoaderImageContext::from_raw(0 as *mut PeCoffLoaderImageContext).is_err());
    }
  }

  #[test]
  fn from_raw_should_require_an_image_read_fn() {
    let mut zero_buffer = vec![0; mem::size_of::<PeCoffLoaderImageContext>()];
    let raw_context_ptr = zero_buffer.as_mut_ptr() as *mut PeCoffLoaderImageContext;

    unsafe {
      assert!(PeCoffLoaderImageContext::from_raw(raw_context_ptr).is_err());
    }
  }

  #[test]
  fn from_raw_should_only_require_an_image_read_fn() {
    let mut zero_buffer = vec![0; mem::size_of::<PeCoffLoaderImageContext>()];
    let raw_context_ptr = zero_buffer.as_mut_ptr() as *mut PeCoffLoaderImageContext;

    unsafe {
      (*raw_context_ptr).image_read = test_mocked_reader;
      assert!(PeCoffLoaderImageContext::from_raw(raw_context_ptr).is_ok());
    }
  }

  #[test]
  fn from_raw_should_handle_bad_status() {
    let mut image_context = PeCoffLoaderImageContext::new(test_mocked_reader);
    let raw_context_ptr = &mut image_context as *mut PeCoffLoaderImageContext;

    unsafe {
      *(&mut (*raw_context_ptr).image_error as *mut PeCoffImageError as *mut u32) = (PeCoffImageError::ImageErrorUnsupported as u32) + 1;
      match PeCoffLoaderImageContext::from_raw(raw_context_ptr) {
        Ok(context) => {
          assert_eq!(context.image_error, PeCoffImageError::ImageErrorSuccess);
        },
        Err(_) => panic!("PeCoffLoaderImageContext::from_raw() should not have failed"),
      }
    }
  }

  #[test]
  fn failed_reads_should_return_err() {
    let mut image_context = PeCoffLoaderImageContext::new(test_mocked_reader);

    // Test 1, return an error.
    unsafe {
      MOCKED_READER_RETURNS.push(efi::Status::INVALID_PARAMETER);
      MOCKED_READER_SIZES.push(10);
    }
    let mut read_size = 10;
    assert!(image_context.read_image(0, &mut read_size).is_err());


    // Test 2, read too much data.
    unsafe {
      MOCKED_READER_RETURNS.push(efi::Status::SUCCESS);
      MOCKED_READER_SIZES.push(20);
    }
    let mut read_size = 10;
    let result = image_context.read_image(0, &mut read_size);
    assert!(result.is_err());
  }

  #[test]
  fn successful_reads_should_have_a_correct_size() {
    let mut image_context = PeCoffLoaderImageContext::new(test_mocked_reader);

    // Test 1, read less data.
    unsafe {
      MOCKED_READER_RETURNS.push(efi::Status::SUCCESS);
      MOCKED_READER_SIZES.push(5);
    }
    let mut read_size = 10;
    let result = image_context.read_image(10, &mut read_size);
    assert!(result.is_ok());
    assert_eq!(read_size, 5);
    assert_eq!(result.unwrap().len(), 5);

    // Test 2, read same data.
    unsafe {
      MOCKED_READER_RETURNS.push(efi::Status::SUCCESS);
      MOCKED_READER_SIZES.push(10);
    }
    let mut read_size = 10;
    let result = image_context.read_image(0, &mut read_size);
    assert!(result.is_ok());
    assert_eq!(read_size, 10);
    assert_eq!(result.unwrap().len(), 10);
  }
}

//======================================================
// PE_IMAGES
//======================================================

struct PeImage {
  signature: [u8; 4],
  // TODO: Figure out how to not parse this multiple times.
  // metadata: goblin::pe::PE<'a>,
  contents: Vec<u8>
}

impl PeImage {
  const STRUCTURE_SIG: [u8; 4] = [0xDE, 0xAD, 0xBE, 0xEF];

  fn new(contents: Vec<u8>) -> Self {
    Self {
      // TODO: Figure out how to replicate SIGNATURE_32()
      signature: Self::STRUCTURE_SIG,
      contents: contents
    }
  }

  unsafe fn from_archive(ptr: *mut Self) -> Result<Box<Self>, ()> {
    let bx = Box::from_raw(ptr);
    if bx.as_ref().signature == PeImage::STRUCTURE_SIG {
      Ok(bx)
    }
    else {
      // Consume the box again so that memory is not changed on error.
      // TODO: This may not be necessary -- and may even be wrong -- but I'm being pedantic.
      let temp = Box::into_raw(bx);
      Err(())
    }
  }

  fn into_archive(bx: Box<Self>) -> Result<*mut Self, ()> {
    Ok(Box::into_raw(bx))
  }
}

impl Drop for PeImage {
  fn drop(&mut self) {
    // TODO: Zero the signature.
    println!("JBB Dropping Metadata");
  }
}

pub struct EdkiiPeImage {
  ffi_context: &'static mut PeCoffLoaderImageContext,
  pe_image: Option<Box<PeImage>>,
}

impl EdkiiPeImage {
  pub unsafe fn from_raw(ptr: *mut PeCoffLoaderImageContext) -> Result<Self, ()> {
    let ffi_context = unsafe{ PeCoffLoaderImageContext::from_raw(ptr)? };

    // Start creating the resulting image.
    let mut new_image = Self { ffi_context: ffi_context, pe_image: None };

    // Now that we've got the context, we can figure out whether we've
    // already parsed this PE image.
    match new_image.unarchive_image_data() {
      Ok(_) => Ok(new_image),
      Err(_) => {
        let mut read_size = new_image.ffi_context.image_size;
        let image_contents = new_image.ffi_context.read_image(0, &mut (read_size as usize))?;

        // Make sure that we read the entire image.
        if read_size != new_image.ffi_context.image_size {
          return Err(());
        }

        let pe_image = match goblin::pe::PE::parse(&image_contents) {
          Ok(parsed_image) => PeImage::new(image_contents),
          Err(_) => return Err(())
        };
        new_image.pe_image = Some(Box::new(pe_image));

        Ok(new_image)
      }
    }
  }

  fn unarchive_image_data(&mut self) -> Result<(), ()> {
    if self.pe_image.is_some() {
      return Err(());
    }

    match self.ffi_context.context {
      0 => Err(()),
      _ => {
        self.pe_image = unsafe { Some(PeImage::from_archive(self.ffi_context.context as *mut PeImage)?) };
        self.ffi_context.context = 0;
        Ok(())
      }
    }
  }

  fn archive_image_data(&mut self) -> Result<(), ()> {
    match self.pe_image.take() {
      None => Err(()),
      Some(boxed_metadata) => {
        match self.ffi_context.context {
          _ => Err(()),
          0 => {
            self.pe_image = None;
            self.ffi_context.context = PeImage::into_archive(boxed_metadata)? as u64;
            Ok(())
          }
        }
      }
    }
  }
}

#[cfg(test)]
mod pe_image_tests {
  use super::*;
  use std::{vec, path::PathBuf};
  use std::fs;

  fn get_binary_test_file_path(file_name: &str) -> PathBuf {
    let mut binaries_path = PathBuf::from(".");
    binaries_path.push("tests");
    binaries_path.push("binaries");
    binaries_path.push(file_name);
    assert!(binaries_path.is_file(), "{} is not a valid binary file", file_name);
    binaries_path
  }

  extern "win64" fn test_file_reader(
      file_handle: *const core::ffi::c_void,
      file_offset: usize,
      read_size: *mut usize,
      output_buffer: *mut core::ffi::c_void
      ) -> efi::Status {
    efi::Status::INVALID_PARAMETER
  }

  #[test]
  fn pe_image_should_archive_and_unarchive() -> Result<(), ()> {
    let test = vec![0, 30, 15, 45, 16];
    let boxed_pe_image = Box::new(PeImage::new(test.to_vec()));

    let archived_image = PeImage::into_archive(boxed_pe_image)?;
    let unarchived_image = unsafe { PeImage::from_archive(archived_image)? };

    assert_eq!(&test, &unarchived_image.as_ref().contents);

    Ok(())
  }

  #[test]
  fn pe_image_should_fail_to_unarchive_with_bad_sig() -> Result<(), ()> {
    let mut boxed_pe_image = Box::new(PeImage::new(vec![0, 30, 15, 45, 16]));

    // Meddle with the primal forces of nature.
    boxed_pe_image.as_mut().signature = [0xFE, 0xED, 0xF0, 0x0D];

    let archived_image = PeImage::into_archive(boxed_pe_image)?;

    assert!(unsafe { PeImage::from_archive(archived_image) }.is_err());

    // NOTE: This currently leaks memory. Probably should fix that at some point.

    Ok(())
  }

  #[test]
  fn from_raw_should_fail_if_no_size_is_provided() -> Result<(), ()> {
    let mut image_context = PeCoffLoaderImageContext::new(test_file_reader);
    let raw_context_ptr = &mut image_context as *mut PeCoffLoaderImageContext;

    unsafe {
      assert!(EdkiiPeImage::from_raw(raw_context_ptr).is_err());
    }

    Ok(())
  }

  #[test]
  fn should_load_contents_when_created() -> Result<(), ()> {
    let test_file_name = "RngDxe.efi";
    let mut image_context = PeCoffLoaderImageContext::new(test_file_reader);
    image_context.handle = Box::into_raw(Box::new(test_file_name)) as *const core::ffi::c_void;
    image_context.image_size = fs

    let raw_context_ptr = &mut image_context as *mut PeCoffLoaderImageContext;

    unsafe {
      assert!(EdkiiPeImage::from_raw(raw_context_ptr).is_err());
    }

    Ok(())
  }
}
