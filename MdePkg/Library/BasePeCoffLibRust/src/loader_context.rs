// @file -- loader_context.rs
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#![allow(unused)]

#![cfg_attr(not(test), no_std)]

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
  image_error:            u32,
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
  pub unsafe fn from_raw(ptr: *mut Self) -> Result<&'static mut Self, ()> {
    if ptr.is_null() {
      Err(())
    }
    else {
      Ok(core::mem::transmute::<*mut Self, &'static mut Self>(ptr))
    }
  }
}
