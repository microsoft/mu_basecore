// @file -- lib.rs
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#![no_std]
#![allow(unused)]
#![allow(non_snake_case)]

// Import EDK2 Libs/Environment
#[cfg(not(test))]
extern crate uefi_rust_panic_lib;
#[cfg(not(test))]
extern crate uefi_rust_allocation_lib;
#[cfg(not(test))]
extern crate uefi_rust_print_lib_debug_lib;
#[cfg(not(test))]
use uefi_rust_print_lib_debug_lib::println;

extern crate alloc;

use r_efi::{efi, base};
use alloc::slice;

mod loader_context;
mod bindings;

use loader_context::PeCoffLoaderImageContext;

#[no_mangle]
#[export_name = "PeCoffLoaderGetImageInfo"]
pub extern "win64" fn pe_coff_loader_get_image_info(context: *mut PeCoffLoaderImageContext) -> efi::Status {
  // Unwrap and parse the first few bits of things.
  let image_context = match unsafe { PeCoffLoaderImageContext::from_raw(context) } {
    Ok(contents) => contents,
    Err(_) => return efi::Status::INVALID_PARAMETER
  };

  let image_contents = match image_context.read_image(0, &mut 0x1380) {
    Ok(contents) => contents,
    Err(_) => return efi::Status::INVALID_PARAMETER
  };

  let pe_image = match goblin::pe::PE::parse(&image_contents) {
    Ok(contents) => contents,
    Err(_) => return efi::Status::INVALID_PARAMETER
  };

  efi::Status::SUCCESS
}

#[no_mangle]
#[export_name = "PeCoffLoaderGetSecurityCookieAddress"]
pub extern "win64" fn pe_coff_loader_get_security_cookie_address(
      context: *mut PeCoffLoaderImageContext,
      security_cookie_address: *mut *const u64
      ) -> efi::Status {
  efi::Status::UNSUPPORTED
}

#[no_mangle]
#[export_name = "PeCoffLoaderRelocateImage"]
pub extern "win64" fn pe_coff_loader_relocate_image(context: *mut PeCoffLoaderImageContext) -> efi::Status {
  efi::Status::UNSUPPORTED
}

#[no_mangle]
#[export_name = "PeCoffLoaderLoadImage"]
pub extern "win64" fn pe_coff_loader_load_image(context: *mut PeCoffLoaderImageContext) -> efi::Status {
  efi::Status::UNSUPPORTED
}

#[no_mangle]
#[export_name = "PeCoffLoaderImageReadFromMemory"]
pub extern "win64" fn pe_coff_loader_image_read_from_memory(
      file_handle: *const core::ffi::c_void,
      file_offset: usize,
      read_size: *mut usize,
      output_buffer: *mut core::ffi::c_void
      ) -> efi::Status {
  if file_handle.is_null() || read_size.is_null() || output_buffer.is_null() {
    return efi::Status::INVALID_PARAMETER;
  }

  // NOTE:
  // All of this implementation is unsafe because the interface
  // design is unfixably broken. This lib should *not* provide this.
  // We're just going to replicated what the previous lib did.
  unsafe {
    let source = slice::from_raw_parts(file_handle as *const u8, file_offset + *read_size);
    let mut destination = slice::from_raw_parts_mut(output_buffer as *mut u8, *read_size);
    destination.copy_from_slice(&source[file_offset..file_offset+*read_size]);
  }

  efi::Status::SUCCESS
}

#[no_mangle]
#[export_name = "PeCoffLoaderRelocateImageForRuntime"]
pub extern "win64" fn pe_coff_loader_relocate_image_for_runtime(
      image_base: efi::PhysicalAddress,
      virt_image_base: efi::PhysicalAddress,
      image_size: usize,
      relocation_data: *const core::ffi::c_void
      ) -> () {
  ()
}

#[no_mangle]
#[export_name = "PeCoffLoaderUnloadImage"]
pub extern "win64" fn pe_coff_loader_unload_image(context: *mut PeCoffLoaderImageContext) -> efi::Status {
  efi::Status::UNSUPPORTED
}
