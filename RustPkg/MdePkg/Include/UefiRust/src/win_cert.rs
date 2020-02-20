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

use r_efi::efi;
use r_efi::efi::{Guid};

pub const WIN_CERTIFICATE_REVISION: u16 = 0x0200;

pub const WIN_CERTIFICATE_TYPE_PKCS_SIGNED_DATA: u16 = 0x0002;
pub const WIN_CERTIFICATE_TYPE_EFI_PKCS115:      u16 = 0x0EF0;
pub const WIN_CERTIFICATE_TYPE_EFI_GUID:         u16 = 0x0EF1;

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct WinCertificate {
    pub length: u32,
    pub revision: u16,
    pub certificate_type: u16,
}

pub const WIN_CERT_TYPE_RSA2048_SHA256_GUID: Guid = Guid::from_fields(
    0xa7717414, 0xc616, 0x4977, 0x94, 0x20, &[0x84, 0x47, 0x12, 0xa7, 0x35, 0xbf]
);

pub const WIN_CERT_TYPE_PKCS7_GUID: Guid = Guid::from_fields(
    0x4aafd29d, 0x68df, 0x49ee, 0x8a, 0xa9, &[0x34, 0x7d, 0x37, 0x56, 0x65, 0xa7]
);

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct WinCertificateUefiGuid {
    pub hdr: WinCertificate,
    pub cert_type: Guid,
    pub cert_data: [u8; 0],
}
