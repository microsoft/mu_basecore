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

#![crate_type = "staticlib"]

#![cfg_attr(not(test), no_std)]

#![allow(unused)]

use r_efi::efi;
use r_efi::efi::{Status};
use r_efi::protocols::graphics_output::BltPixel;

extern {
  fn DebugPrint(ErrorLevel: usize, Format: *const u8, Arg: ...);

  fn AllocatePool (Size: usize) -> *mut c_void;
  fn AllocateZeroPool (Size: usize) -> *mut c_void;
  fn FreePool (Buffer: *mut c_void);
}

use core::panic::PanicInfo;
use core::ffi::c_void;

use core::mem::size_of;
use core::mem::transmute;

use base_lib::offset_of;
use industry_standard_include::bmp::{BmpColorMap, BmpImageHeader};
use debug_lib::DEBUG_INFO;

#[panic_handler]
#[allow(clippy::empty_loop)]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
#[export_name = "TranslateBmpToGopBlt"]
pub extern fn traslate_bmp_to_gop_blt (
    bmp_image: *const c_void,
    bmp_image_size: usize,
    gop_blt : *mut *mut BltPixel,
    gop_blt_size: *mut usize,
    pixel_height: *mut usize,
    pixel_width: *mut usize,
    ) -> Status
{
    unsafe {
      DebugPrint (DEBUG_INFO, b"traslate_bmp_to_gop_blt\n\0" as *const u8, 0);
    }
    if (bmp_image == core::ptr::null_mut()) || 
       (gop_blt == core::ptr::null_mut()) ||
       (gop_blt_size == core::ptr::null_mut()) ||
       (pixel_height == core::ptr::null_mut()) ||
       (pixel_width == core::ptr::null_mut()) {
      return Status::INVALID_PARAMETER;
    }

    if bmp_image_size < size_of::<BmpImageHeader>() {
      return Status::UNSUPPORTED;
    }

    //
    // Check header
    //
    let bmp_header : &BmpImageHeader = unsafe {transmute::<*const c_void, &BmpImageHeader>(bmp_image)};

    unsafe {
      DebugPrint (DEBUG_INFO, b"bmp_image_size - %d\n\0" as *const u8, bmp_image_size);

      DebugPrint (DEBUG_INFO, b"bmp_header\n\0" as *const u8, 0);
      DebugPrint (DEBUG_INFO, b".char_b             - \'%c\'\n\0" as *const u8, bmp_header.char_b as usize);
      DebugPrint (DEBUG_INFO, b".char_m             - \'%c\'\n\0" as *const u8, bmp_header.char_m as usize);
      DebugPrint (DEBUG_INFO, b".size               - %d\n\0" as *const u8, bmp_header.size as usize);
      DebugPrint (DEBUG_INFO, b".image_offset       - %d\n\0" as *const u8, bmp_header.image_offset as usize);
      DebugPrint (DEBUG_INFO, b".header_size        - %d\n\0" as *const u8, bmp_header.header_size as usize);
      DebugPrint (DEBUG_INFO, b".pixel_width        - %d\n\0" as *const u8, bmp_header.pixel_width as usize);
      DebugPrint (DEBUG_INFO, b".pixel_height       - %d\n\0" as *const u8, bmp_header.pixel_height as usize);
      DebugPrint (DEBUG_INFO, b".planes             - %d\n\0" as *const u8, bmp_header.planes as usize);
      DebugPrint (DEBUG_INFO, b".bit_per_pixel      - %d\n\0" as *const u8, bmp_header.bit_per_pixel as usize);
      DebugPrint (DEBUG_INFO, b".compression_type   - %d\n\0" as *const u8, bmp_header.compression_type as usize);
      DebugPrint (DEBUG_INFO, b".image_size         - %d\n\0" as *const u8, bmp_header.image_size as usize);
      DebugPrint (DEBUG_INFO, b".x_pixels_per_meter - %d\n\0" as *const u8, bmp_header.x_pixels_per_meter as usize);
      DebugPrint (DEBUG_INFO, b".y_pixels_per_meter - %d\n\0" as *const u8, bmp_header.y_pixels_per_meter as usize);
      DebugPrint (DEBUG_INFO, b".number_of_colors   - %d\n\0" as *const u8, bmp_header.number_of_colors as usize);
      DebugPrint (DEBUG_INFO, b".important_colors   - %d\n\0" as *const u8, bmp_header.important_colors as usize);
    }

    if (bmp_header.char_b != 'B' as u8) ||
       (bmp_header.char_m != 'M' as u8) || 
       (bmp_header.compression_type != 0) || 
       (bmp_header.pixel_width == 0) || 
       (bmp_header.pixel_height == 0) || 
       (bmp_header.header_size != size_of::<BmpImageHeader>() as u32 - offset_of!(BmpImageHeader, header_size) as u32) {
      return Status::UNSUPPORTED;
    }

    let mut data_size_per_line = 0;
    match (bmp_header.pixel_width).checked_mul(bmp_header.bit_per_pixel as u32) {
      Some(size) => {data_size_per_line = size},
      None => {return Status::UNSUPPORTED},
    }

    match data_size_per_line.checked_add(31) {
      Some(size) => {data_size_per_line = size},
      None => {return Status::UNSUPPORTED},
    }

    let data_size_per_line = (data_size_per_line >> 3) & 0xFFFFFFFC;
    let mut data_size = 0;
    match (data_size_per_line).checked_mul(bmp_header.pixel_height) {
      Some(size) => {data_size = size},
      None => {return Status::UNSUPPORTED},
    }

    if (bmp_header.size as usize != bmp_image_size) || 
       (bmp_header.size <  bmp_header.image_offset) || 
       (bmp_header.size - bmp_header.image_offset != data_size) {
      return Status::UNSUPPORTED;
    }

    let bmp_color_map : * const BmpColorMap = (bmp_image as usize + size_of::<BmpImageHeader>()) as *const BmpColorMap;
    if bmp_header.image_offset < size_of::<BmpImageHeader>() as u32 {
      return Status::UNSUPPORTED;
    }

    let mut color_number : u32 = 0;
    if bmp_header.image_offset > size_of::<BmpImageHeader>() as u32 {
      match (bmp_header.bit_per_pixel) {
        1 => {color_number = 2},
        4 => {color_number = 16},
        8 => {color_number = 256},
        _ => {color_number = 0},
      }
      if bmp_header.image_offset - (size_of::<BmpImageHeader>() as u32) < (size_of::<BmpColorMap>() as u32) * color_number {
        return Status::UNSUPPORTED;
      }
    }

    //
    // calc image
    //
    let mut blt_buffer_size = 0;
    match (bmp_header.pixel_height).checked_mul(bmp_header.pixel_width) {
      Some(size) => {blt_buffer_size = size},
      None => {return Status::UNSUPPORTED},
    }
    match (blt_buffer_size).checked_mul(size_of::<BltPixel>() as u32) {
      Some(size) => {blt_buffer_size = size},
      None => {return Status::UNSUPPORTED},
    }

    let mut is_allocated : bool = false;
    if unsafe {*gop_blt == core::ptr::null_mut() } {
      unsafe {
        *gop_blt = AllocatePool (blt_buffer_size as usize) as *mut BltPixel;
        if *gop_blt == core::ptr::null_mut() {
          return Status::OUT_OF_RESOURCES;
        }
        is_allocated = true;
      }
    } else {
      if unsafe {*gop_blt_size < blt_buffer_size as usize } {
        unsafe {*gop_blt_size = blt_buffer_size as usize };
        return Status::BUFFER_TOO_SMALL;
      }
    }

    unsafe {*gop_blt_size = blt_buffer_size as usize };
    unsafe {*pixel_height = bmp_header.pixel_height as usize};
    unsafe {*pixel_width= bmp_header.pixel_width as usize};

    let blt_buffer : *mut BltPixel = unsafe {*gop_blt};
    let mut blt : *mut BltPixel = core::ptr::null_mut();

    let mut image : *const u8 = (bmp_image as usize + (bmp_header.image_offset as usize)) as *const u8;
    let image_header = image;

    for height in 0 .. bmp_header.pixel_height {
      unsafe {
        blt = blt_buffer.offset(((bmp_header.pixel_height - height - 1) * bmp_header.pixel_width) as isize);
      }

      let mut width : u32 = 0;
      while width < bmp_header.pixel_width {
        match bmp_header.bit_per_pixel {
          // Translate 1-bit (2 colors) BMP to 24-bit color
          1 => {
            unsafe {
              for index in 0 .. 8 {
                (*blt).red   = (*bmp_color_map.offset((((*image) >> (7 - index)) & 0x1) as usize as isize)).red;
                (*blt).green = (*bmp_color_map.offset((((*image) >> (7 - index)) & 0x1) as usize as isize)).green;
                (*blt).blue  = (*bmp_color_map.offset((((*image) >> (7 - index)) & 0x1) as usize as isize)).blue;
                blt = blt.offset(1);
                width = width + 1;
                if width >= bmp_header.pixel_width {
                  break;
                }
              }
              blt = blt.offset(-1);
              width = width - 1;
            }
          },
          // Translate 4-bit (16 colors) BMP Palette to 24-bit color
          4 => {
            unsafe {
              let index : u8 = (*image) >> 4;
              (*blt).red = (*bmp_color_map.offset(index as usize as isize)).red;
              (*blt).green = (*bmp_color_map.offset(index as usize as isize)).green;
              (*blt).blue = (*bmp_color_map.offset(index as usize as isize)).blue;
              if width < bmp_header.pixel_width - 1 {
                unsafe { blt = blt.offset(1); }
                width = width + 1;
                let index : u8 = (*image) & 0xF;
                (*blt).red = (*bmp_color_map.offset(index as usize as isize)).red;
                (*blt).green = (*bmp_color_map.offset(index as usize as isize)).green;
                (*blt).blue = (*bmp_color_map.offset(index as usize as isize)).blue;
              }
            }
          },
          // Translate 8-bit (256 colors) BMP Palette to 24-bit color
          8 => {
            unsafe {
              (*blt).red = (*bmp_color_map.offset(*image as usize as isize)).red;
              (*blt).green = (*bmp_color_map.offset(*image as usize as isize)).green;
              (*blt).blue = (*bmp_color_map.offset(*image as usize as isize)).blue;
            }
          },
          // It is 24-bit BMP.
          24 => {
            unsafe {
              (*blt).blue = *image;
              image = image.offset(1);
              (*blt).green = *image;
              image = image.offset(1);
              (*blt).red = *image;
            }
          },
          // It is 32-bit BMP.
          32 => {
            unsafe {
              (*blt).blue = *image;
              image = image.offset(1);
              (*blt).green = *image;
              image = image.offset(1);
              (*blt).red = *image;
              image = image.offset(1);
            }
          },
          _ => {
            if is_allocated {
              unsafe {
                FreePool (*gop_blt as *mut c_void);
                *gop_blt = core::ptr::null_mut();
              }
            }
            return Status::UNSUPPORTED;
          },
        }
        width = width + 1;
        unsafe { blt = blt.offset(1); }
        unsafe { image = image.offset(1); }
      }


      let image_index = (image as usize - image_header as usize) as isize;
      if ((image_index % 4) != 0) {
        unsafe { image = image.offset(4 - (image_index % 4)); }
      }
    }

    Status::SUCCESS
}

#[no_mangle]
#[export_name = "TranslateGopBltToBmp"]
pub extern fn traslate_gop_blt_to_bmp (
    gop_blt : *mut BltPixel,
    pixel_height: u32,
    pixel_width: u32,
    bmp_image: *mut *mut c_void,
    bmp_image_size: *mut u32,
    ) -> Status
{
    unsafe {
      DebugPrint (DEBUG_INFO, b"traslate_gop_blt_to_bmp\n\0" as *const u8, 0);
    }
    if (gop_blt == core::ptr::null_mut()) ||
       (bmp_image == core::ptr::null_mut()) || 
       (bmp_image_size == core::ptr::null_mut()) ||
       (pixel_height == 0) ||
       (pixel_width == 0) {
      return Status::INVALID_PARAMETER;
    }

    let padding_size = pixel_width & 0x3;

    let mut bmp_size = 0;
    match (pixel_width).checked_mul(3) {
      Some(size) => {bmp_size = size},
      None => {return Status::UNSUPPORTED},
    }
    match (bmp_size).checked_add(padding_size) {
      Some(size) => {bmp_size = size},
      None => {return Status::UNSUPPORTED},
    }

    match (bmp_size).checked_mul(pixel_height) {
      Some(size) => {bmp_size = size},
      None => {return Status::UNSUPPORTED},
    }
    match (bmp_size).checked_add(size_of::<BmpImageHeader>() as u32) {
      Some(size) => {bmp_size = size},
      None => {return Status::UNSUPPORTED},
    }

    if unsafe {*bmp_image == core::ptr::null_mut() } {
      unsafe {
        *bmp_image_size = bmp_size;
        *bmp_image = AllocateZeroPool(bmp_size as usize);
        if *bmp_image == core::ptr::null_mut() {
          return Status::OUT_OF_RESOURCES;
        }
      }
    } else {
      if unsafe {*bmp_image_size < bmp_size } {
        unsafe {*bmp_image_size = bmp_size };
        return Status::BUFFER_TOO_SMALL;
      }
    }

    unsafe {*bmp_image_size = bmp_size };

    let bmp_image_header : *mut BmpImageHeader = unsafe {(*bmp_image)} as *mut BmpImageHeader;
    unsafe {
      (*bmp_image_header).char_b = 'B' as u8;
      (*bmp_image_header).char_m = 'M' as u8;
      (*bmp_image_header).size = bmp_size;
      (*bmp_image_header).image_offset = size_of::<BmpImageHeader>() as u32;
      (*bmp_image_header).header_size = size_of::<BmpImageHeader>() as u32 - offset_of!(BmpImageHeader, header_size) as u32;
      (*bmp_image_header).pixel_width = pixel_width;
      (*bmp_image_header).pixel_height = pixel_height;
      (*bmp_image_header).planes = 1;
      (*bmp_image_header).bit_per_pixel = 24;
      (*bmp_image_header).image_size = bmp_size - size_of::<BmpImageHeader>() as u32;
    }

    // convert
    let mut image : *mut u8 = (bmp_image_header as usize + size_of::<BmpImageHeader>()) as *mut u8;
    let mut blt_pixel: *const BltPixel;
    for row in 0 .. pixel_height {
      unsafe {
        blt_pixel = gop_blt.offset(((pixel_height - row - 1) * pixel_width) as usize as isize);
      }

      for col in 0 .. pixel_width {
        unsafe {
          *image = (*blt_pixel).blue;
          image = image.offset(1);
          *image = (*blt_pixel).green;
          image = image.offset(1);
          *image = (*blt_pixel).red;
          image = image.offset(1);
          blt_pixel = blt_pixel.offset(1);
        }
      }

      unsafe {image = image.offset(padding_size as usize as isize); }
    }

    Status::SUCCESS
}