// @file -- bindings.rs
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

use core::ffi::c_void;

extern "C" {
  pub fn CopyMem (_: *mut c_void, _: *const c_void, _: usize) -> *const c_void;
}
