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

#![feature(alloc_layout_extra)]
#![feature(allocator_api)]
#![feature(alloc_error_handler)]

#![cfg_attr(not(test), no_std)]

#![allow(unused)]

extern crate uefi_rust_panic_lib;

use core::alloc::{GlobalAlloc, Layout, AllocRef};
use r_efi::efi;
use r_efi::efi::{Status};
use core::ffi::c_void;

pub struct MyAllocator;

static mut ST : *mut efi::SystemTable = core::ptr::null_mut();
static mut BS : *mut efi::BootServices = core::ptr::null_mut();

unsafe impl GlobalAlloc for MyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
      let size = layout.size();
      let align = layout.align();
      if align > 8 {
        return core::ptr::null_mut();
      }

      let mut address : *mut c_void = core::ptr::null_mut();
      let status = ((*BS).allocate_pool) (
                     efi::MemoryType::BootServicesData,
                     size,
                     &mut address as *mut *mut c_void
                     );
      if status != Status::SUCCESS {
        return core::ptr::null_mut();
      }
      address as *mut u8
    }
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
      ((*BS).free_pool) (ptr as *mut c_void);
    }
}

#[global_allocator]
static ALLOCATOR: MyAllocator = MyAllocator;

#[alloc_error_handler]
fn alloc_error_handler(layout: core::alloc::Layout) -> !
{
    loop {}
}

pub extern fn init(system_table: *mut efi::SystemTable) {
    unsafe {
      ST = system_table;
      BS = (*ST).boot_services;
    }
}
