// Copyright (c) 2019 Intel Corporation
// Copyright (c) Microsoft Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#![crate_type = "staticlib"]

#![feature(alloc_layout_extra)]
#![feature(allocator_api)]
#![feature(alloc_error_handler)]
#![feature(asm)]

#![cfg_attr(not(test), no_std)]
#![no_main]

#![allow(unused)]

mod mem;

extern crate test_rust_lang_lib;
extern crate uefi_rust_intrinsic_lib;
extern crate uefi_rust_panic_lib;
extern crate uefi_rust_allocation_lib;

use r_efi::efi;
use r_efi::efi::{Status};

use core::panic::PanicInfo;
use core::ffi::c_void;

use core::mem::size_of;
use core::mem::transmute;

use core::slice::from_raw_parts;


static mut ST : *mut efi::SystemTable = core::ptr::null_mut();
static mut BS : *mut efi::BootServices = core::ptr::null_mut();

extern crate alloc;

use alloc::vec::Vec;
use alloc::boxed::Box;


#[no_mangle]
#[export_name = "AllocatePool"]
extern fn AllocatePool (size: usize) -> *mut c_void
{
      let mut address : *mut c_void = core::ptr::null_mut();

      let status = unsafe { ((*BS).allocate_pool) (
                     efi::MemoryType::BootServicesData,
                     size,
                     &mut address as *mut *mut c_void
                     ) } ;
      if status != Status::SUCCESS {
        return core::ptr::null_mut();
      }
      address as *mut c_void
}



#[no_mangle]
#[export_name = "AllocateZeroPool"]
extern fn AllocateZeroPool (size: usize) -> *mut c_void
{
    let buffer = AllocatePool (size);
    if buffer == core::ptr::null_mut() {
      return core::ptr::null_mut();
    }

    unsafe {core::ptr::write_bytes (buffer, 0, size);}

    buffer as *mut c_void
}



#[no_mangle]
#[export_name = "FreePool"]
extern fn FreePool (buffer: *mut c_void)
{
    unsafe {
      ((*BS).free_pool) (buffer as *mut c_void);
    }
}
#[no_mangle]
#[export_name = "ExternInit"]
extern fn ExternInit(data: *mut usize)
{
}


#[no_mangle]
pub extern fn efi_main(handle: efi::Handle, system_table: *mut efi::SystemTable) -> Status
{
    uefi_rust_allocation_lib::init(system_table);

    unsafe {
      ST = system_table;
      BS = (*ST).boot_services;
    }

    // L"Hello World!/r/n"
    let string_name  = & mut [
      0x48u16, 0x65u16, 0x6cu16, 0x6cu16, 0x6fu16, 0x20u16,
      0x57u16, 0x6fu16, 0x72u16, 0x6cu16, 0x64u16, 0x21u16,
      0x0Au16, 0x0Du16, 0x00u16
      ];
    unsafe {
      ((*((*ST).con_out)).output_string) (
          unsafe {(*ST).con_out},
          string_name.as_ptr() as *mut efi::Char16,
          );
    }

    test_rust_lang_lib::test_integer_overflow (0x10000, 0xFFFFFFFF, 0xFFFFFFFF);

    Status::SUCCESS
}
