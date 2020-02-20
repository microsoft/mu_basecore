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

use core::panic::PanicInfo;
use core::ffi::c_void;
use core::mem::{align_of, size_of, transmute};

use base_lib::offset_of;

use r_efi::efi;
use r_efi::efi::{Status};

use uefi_include::fmp::FirmwareImageAuthentication;
use uefi_include::win_cert::{WinCertificateUefiGuid, WIN_CERTIFICATE_TYPE_EFI_GUID, WIN_CERT_TYPE_PKCS7_GUID};

#[panic_handler]
#[allow(clippy::empty_loop)]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

extern {
  fn AllocatePool (Size: usize) -> *mut c_void;
  fn FreePool (Buffer: *mut c_void);

  fn Pkcs7Verify (
       p7_data : *mut u8,
       p7_length : usize,
       trusted_cert : *mut u8,
       cert_length : usize,
       in_data : *mut u8,
       data_length : usize,
       ) -> efi::Boolean;
}

fn fmp_authenticated_handler_pkcs7 (
    image : *mut FirmwareImageAuthentication,
    image_size: usize,
    public_key_data: *mut u8,
    public_key_data_length: usize,
    ) -> Status
{
    let image_auth : &mut FirmwareImageAuthentication = unsafe { transmute::<*mut FirmwareImageAuthentication, &mut FirmwareImageAuthentication>(image) };

    let p7_length = image_auth.auth_info.hdr.length - offset_of!(WinCertificateUefiGuid, cert_data) as u32;
    let p7_data : *mut u8 = &mut image_auth.auth_info.cert_data as *mut [u8; 0] as *mut u8;

    let temp_buffer = unsafe { AllocatePool (image_size - image_auth.auth_info.hdr.length as usize) };
    if temp_buffer == core::ptr::null_mut() {
      return Status::OUT_OF_RESOURCES;
    }

    unsafe {
      core::ptr::copy_nonoverlapping (
        (image as usize + size_of::<u64>() + image_auth.auth_info.hdr.length as usize) as *mut c_void,
        temp_buffer,
        image_size - size_of::<u64>() - image_auth.auth_info.hdr.length as usize
        );
      core::ptr::copy_nonoverlapping (
        &image_auth.monotonic_count as *const u64 as *mut u64 as *mut c_void,
        (temp_buffer as usize + image_size - size_of::<u64>() - image_auth.auth_info.hdr.length as usize) as *mut c_void,
        size_of::<u64>()
        );
    }
    let crypto_status = unsafe { Pkcs7Verify(
                                   p7_data,
                                   p7_length as usize,
                                   public_key_data,
                                   public_key_data_length,
                                   temp_buffer as *mut u8,
                                   image_size - image_auth.auth_info.hdr.length as usize
                                   ) };
    unsafe { FreePool(temp_buffer); }

    if crypto_status == efi::Boolean::FALSE {
      return Status::SECURITY_VIOLATION;
    }

    Status::SUCCESS
}

#[no_mangle]
#[export_name = "AuthenticateFmpImage"]
pub extern fn authenticate_fmp_image (
    image : *mut FirmwareImageAuthentication,
    image_size: usize,
    public_key_data: *mut u8,
    public_key_data_length: usize,
    ) -> Status
{
    if image == core::ptr::null_mut() ||
       image_size == 0 {
      return Status::UNSUPPORTED;
    }

    let image_auth : &FirmwareImageAuthentication = unsafe { transmute::<*mut FirmwareImageAuthentication, &FirmwareImageAuthentication>(image) };

    if image_size < size_of::<FirmwareImageAuthentication>() ||
       image_auth.auth_info.hdr.length <= offset_of!(WinCertificateUefiGuid, cert_data) as u32 ||
       image_auth.auth_info.hdr.length > core::u32::MAX - size_of::<u64>() as u32 ||
       image_size <= image_auth.auth_info.hdr.length as usize + size_of::<u64>() ||
       image_auth.auth_info.hdr.revision != 0x0200 ||
       image_auth.auth_info.hdr.certificate_type != WIN_CERTIFICATE_TYPE_EFI_GUID {
      return Status::INVALID_PARAMETER;
    }

    if image_auth.auth_info.cert_type == WIN_CERT_TYPE_PKCS7_GUID {
      return fmp_authenticated_handler_pkcs7 (image, image_size, public_key_data, public_key_data_length);
    }

    Status::UNSUPPORTED
}
