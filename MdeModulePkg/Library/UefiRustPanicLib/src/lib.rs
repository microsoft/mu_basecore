// Copyright (c) 2019 Intel Corporation
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

#![no_std]
#![feature(alloc_error_handler)]

extern crate uefi_rust_allocation_lib;

use uefi_rust_print_lib_debug_lib::println;

#[panic_handler]
fn panic(info: &core::panic::PanicInfo) -> ! {
  println!("PANIC!: {:#?}", info);
  loop {}
}

#[alloc_error_handler]
fn alloc_error_handler(layout: core::alloc::Layout) -> ! {
    println!("ALLOC ERROR!: {:#?}", layout);
    loop {}
}
