// @file -- lib.rs
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#![allow(unused)]
#![allow(non_snake_case)]

#![cfg_attr(not(test), no_std)]

#[cfg(not(test))]
extern crate uefi_rust_panic_lib;

use r_efi::{efi, base};

mod loader_context;

use loader_context::PeCoffLoaderImageContext;

#[no_mangle]
#[export_name = "PeCoffLoaderGetImageInfo"]
pub extern "win64" fn pe_coff_loader_get_image_info(context: *mut PeCoffLoaderImageContext) -> efi::Status {
  match unsafe { PeCoffLoaderImageContext::from_raw(context) } {
    Err(_) => efi::Status::INVALID_PARAMETER,
    Ok(image_context) => {
      efi::Status::SUCCESS
    }
  }
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
  efi::Status::UNSUPPORTED
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
