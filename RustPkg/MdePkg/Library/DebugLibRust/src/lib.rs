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

#![cfg_attr(not(test), no_std)]

#![allow(unused)]

pub const DEBUG_INIT : usize = 0x00000001;
pub const DEBUG_WARN : usize = 0x00000002;
pub const DEBUG_LOAD : usize = 0x00000004;
pub const DEBUG_FS : usize = 0x00000008;
pub const DEBUG_POOL : usize = 0x00000010;
pub const DEBUG_PAGE : usize = 0x00000020;
pub const DEBUG_INFO : usize = 0x00000040;
pub const DEBUG_DISPATCH : usize = 0x00000080;
pub const DEBUG_VARIABLE : usize = 0x00000100;
pub const DEBUG_BM : usize = 0x00000400;
pub const DEBUG_BLKIO : usize = 0x00001000;
pub const DEBUG_NET : usize = 0x00004000;
pub const DEBUG_UNDI : usize = 0x00010000;
pub const DEBUG_LOADFILE : usize = 0x00020000;
pub const DEBUG_EVENT : usize = 0x00080000;
pub const DEBUG_GCD : usize = 0x00100000;
pub const DEBUG_CACHE : usize = 0x00200000;
pub const DEBUG_VERBOSE : usize = 0x00400000;
pub const DEBUG_ERROR : usize = 0x80000000;
