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
#![feature(asm)]
#![feature(global_asm)]
#![feature(naked_functions)]

#![cfg_attr(not(test), no_std)]

#![allow(unused)]

mod mem;
mod common;

use r_efi::efi;
use r_efi::efi::{Status};

use core::panic::PanicInfo;
use core::ffi::c_void;

use core::mem::size_of;
use core::mem::transmute;

#[repr(C, packed)]
#[derive(Copy, Clone, Debug)]
pub struct Ia32Descriptor {
    pub limit: u16,
    pub base: usize,
}
