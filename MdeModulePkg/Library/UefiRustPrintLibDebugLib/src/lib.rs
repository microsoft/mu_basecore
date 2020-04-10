// @file -- lib.rs
// Implementation of the Rust print! and println! macros
// that use the EDK2 DebugLib as the back-end.
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

// Borrows heavily from Philipp Oppermann's blog on Writing an OS in Rust
// https://os.phil-opp.com/vga-text-mode/

#![no_std]

extern crate alloc;

use alloc::vec::Vec;
use core::fmt;
use r_efi::efi;

extern "C" {
  fn DebugPrintEnabled () -> efi::Boolean;
  fn DebugPrintLevelEnabled (Level: usize) -> efi::Boolean;
  fn DebugPrint(ErrorLevel: usize, Format: *const u8, Arg: ...);
}

pub const DEBUG_INIT      : usize = 0x00000001;
pub const DEBUG_WARN      : usize = 0x00000002;
pub const DEBUG_LOAD      : usize = 0x00000004;
pub const DEBUG_FS        : usize = 0x00000008;
pub const DEBUG_POOL      : usize = 0x00000010;
pub const DEBUG_PAGE      : usize = 0x00000020;
pub const DEBUG_INFO      : usize = 0x00000040;
pub const DEBUG_DISPATCH  : usize = 0x00000080;
pub const DEBUG_VARIABLE  : usize = 0x00000100;
pub const DEBUG_BM        : usize = 0x00000400;
pub const DEBUG_BLKIO     : usize = 0x00001000;
pub const DEBUG_NET       : usize = 0x00004000;
pub const DEBUG_UNDI      : usize = 0x00010000;
pub const DEBUG_LOADFILE  : usize = 0x00020000;
pub const DEBUG_EVENT     : usize = 0x00080000;
pub const DEBUG_GCD       : usize = 0x00100000;
pub const DEBUG_CACHE     : usize = 0x00200000;
pub const DEBUG_VERBOSE   : usize = 0x00400000;
pub const DEBUG_ERROR     : usize = 0x80000000;

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)));
}

#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    DebugWriter{}.write_fmt(args).unwrap();
}

struct DebugWriter;

impl fmt::Write for DebugWriter {
  fn write_str(&mut self, s: &str) -> fmt::Result {
    // For now, hard-code what level we're going to use.
    internal_debug_string(DEBUG_ERROR, s);
    Ok(())
  }
}

fn internal_debug_string(level: usize, string: &str) {
  // Determine whether we're going to do anything.
  if unsafe { DebugPrintEnabled() } == efi::Boolean::TRUE &&
      unsafe { DebugPrintLevelEnabled(level) } == efi::Boolean::TRUE {
    let mut output_string = string.bytes()
                            .map(|byte| {
                              match byte {
                                  // printable ASCII byte or newline
                                  0x20..=0x7e | b'\n' => byte,
                                  // not part of printable ASCII range
                                  _ => 0xfe,
                              }
                            })
                            .collect::<Vec<u8>>();
    // We must have a NULL-terminator.
    output_string.push(0x00);

    unsafe { DebugPrint (level, output_string.as_ptr()) };
  }
}
