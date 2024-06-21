//! Firmware Filesystem
//!
//! Exports services used to interact with the Firmware Filesystem (FFS).
//!
//! See <https://uefi.org/specs/PI/1.8A/V3_Design_Discussion.html#firmware-storage-introduction>.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

pub mod ffs;
pub mod fv;
pub mod fvb;

pub use ffs::{
  attributes::{raw as FfsRawAttribute, Attribute as FfsAttribute},
  file::{
    raw::{r#type as FfsFileRawType, state as FfsFileRawState},
    State as FfsFileState, Type as FfsFileType,
  },
  section::{
    header as FfsSectionHeader, raw_type as FfsSectionRawType, raw_type::encapsulated as FfsEncapsulatedSectionRawType,
    EfiSectionType, Type as FfsSectionType,
  },
  File as FfsFile, Section as FfsSection,
};

pub use fv::{
  attributes::{raw::fv2 as Fv2RawAttributes, EfiFvAttributes, Fv2 as Fv2Attributes},
  file::{raw::attribute as FvFileRawAttribute, Attribute as FvFileAttribute, EfiFvFileAttributes},
  EfiFvFileType, FirmwareVolume, Header as FvHeader, WritePolicy,
};

pub use fvb::attributes::{raw::fvb2 as Fvb2RawAttributes, EfiFvbAttributes2, Fvb2 as Fvb2Attributes};
