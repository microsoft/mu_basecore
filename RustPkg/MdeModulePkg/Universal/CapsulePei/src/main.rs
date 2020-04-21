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

mod capsule_coalesce;
use capsule_coalesce::{capsule_data_coalesce, MemoryResourceDescriptor};

use r_efi::efi;
use r_efi::efi::{Status, PhysicalAddress};

use core::ffi::c_void;
use base_lib::Ia32Descriptor;

extern {
  fn AsmReadIdtr(idtr: *mut Ia32Descriptor);
  fn AsmWriteIdtr(idtr: *mut Ia32Descriptor);
  fn AsmDisablePaging64 (cs: u16, entry_point: u32, context1: u32, context2: u32, new_stack: u32);
}

#[repr(C, packed)]
#[derive(Copy, Clone, Debug)]
pub struct Switch32To64Context {
    pub entry_point: u64,
    pub stack_buffer_base: u64,
    pub stack_buffer_length: u64,
    pub jump_buffer: u64,
    pub block_list_addr: u64,
    pub memory_resource: u64,
    pub memory_base_64_ptr: u64,
    pub memory_size_64_ptr: u64,
    pub page_1g_support: u8,
    pub address_en_mask: u64,
}

#[repr(C, packed)]
#[derive(Copy, Clone, Debug)]
pub struct Switch64To32Context {
    pub return_cs: u16,
    pub return_entry_point: u64,
    pub return_status: u64,
    pub gdtr: Ia32Descriptor,
}

#[no_mangle]
pub extern fn efi_main(
  entry_point_context: *mut Switch32To64Context,
  return_context: *mut Switch64To32Context
  ) -> !
{
    let mut ia32_idtr : Ia32Descriptor = Ia32Descriptor {limit: 0xFFFF, base: 0};

  unsafe {
    AsmReadIdtr (&mut ia32_idtr as *mut Ia32Descriptor);

    let status = capsule_data_coalesce (
                   core::ptr::null_mut(),
                   (*entry_point_context).block_list_addr as usize as *mut PhysicalAddress,
                   (*entry_point_context).memory_resource as usize as *mut MemoryResourceDescriptor,
                   (*entry_point_context).memory_base_64_ptr as usize as *mut *mut c_void,
                   (*entry_point_context).memory_size_64_ptr as usize as *mut usize,
                   );

    //(*return_context).return_status = status as u64;

    AsmWriteIdtr (&mut ia32_idtr as *mut Ia32Descriptor);

    AsmDisablePaging64 (
        (*return_context).return_cs,
        (*return_context).return_entry_point as u32,
        entry_point_context as usize as u32,
        return_context as usize as u32,
        ((*entry_point_context).stack_buffer_base as usize + (*entry_point_context).stack_buffer_length as usize) as u32,
        );
  }

    loop {
      assert!(false);
    }
}
