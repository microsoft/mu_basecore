//! Platform Initialization Protocols
//!
//! Each protocol in the PI Specification is maintained as a separate module.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

pub mod bds;
pub mod cpu_arch;
pub mod firmware_volume;
pub mod firmware_volume_block;
pub mod metronome;
pub mod runtime;
pub mod status_code;
pub mod timer;
pub mod watchdog;
