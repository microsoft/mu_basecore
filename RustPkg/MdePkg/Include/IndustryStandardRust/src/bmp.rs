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

#![cfg_attr(not(test), no_std)]

#![allow(unused)]

use core::panic::PanicInfo;
use core::ffi::c_void;

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct BmpColorMap {
    pub blue: u8,
    pub green: u8,
    pub red: u8,
    pub reserved: u8,
}

#[repr(C, packed)]
#[derive(Copy, Clone, Debug)]
pub struct BmpImageHeader {
    pub char_b: u8,
    pub char_m: u8,
    pub size: u32,
    pub reserved: [u16; 2],
    pub image_offset: u32,
    pub header_size: u32,
    pub pixel_width: u32,
    pub pixel_height: u32,
    pub planes: u16,
    pub bit_per_pixel: u16,
    pub compression_type: u32,
    pub image_size: u32,
    pub x_pixels_per_meter: u32,
    pub y_pixels_per_meter: u32,
    pub number_of_colors: u32,
    pub important_colors: u32,
}

