//! Linked List Entry
//!
//! Defined in the PI Specification as an EFI Linked List entry (EfiListEntry). See Related Definitions for the
//! Runtime Architectural Protocol.
//!
//! Represents a doubly linked list where with forward and back links.
//!
//! See <https://uefi.org/specs/PI/1.8A/V2_DXE_Architectural_Protocols.html#efi-runtime-arch-protocol>.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

#[repr(C)]
#[derive(Debug)]
pub struct Entry {
  pub forward_link: *mut Entry,
  pub back_link: *mut Entry,
}
