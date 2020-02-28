// Copyright (c) 2019 Intel Corporation
// Copyright (c) Microsoft Corporation.
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

#![feature(alloc_error_handler)]

#![cfg(no_std)]

extern crate uefi_rust_panic_lib;

use core::alloc::{GlobalAlloc, Layout};
use core::ffi::c_void;

extern "C" {
  fn AllocatePool (Size: usize) -> *mut c_void;
  // fn AllocateZeroPool (Size: usize) -> *mut c_void;
  fn FreePool (Buffer: *mut c_void);
}

pub struct MyAllocator;

unsafe impl GlobalAlloc for MyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
      let size = layout.size();
      let align = layout.align();
      if align > 8 {
        return core::ptr::null_mut();
      }

      AllocatePool (size) as *mut u8
    }
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
      FreePool (ptr as *mut c_void);
    }
}

#[cfg(not(test))]
#[global_allocator]
static ALLOCATOR: MyAllocator = MyAllocator;

#[cfg(not(test))]
#[alloc_error_handler]
fn alloc_error_handler(_layout: core::alloc::Layout) -> !
{
    loop {}
}
