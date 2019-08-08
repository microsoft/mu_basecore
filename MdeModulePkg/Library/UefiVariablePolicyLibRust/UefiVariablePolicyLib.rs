#![no_std]

extern crate r_efi;

use core::panic::PanicInfo;

// use r_efi::efi;
use r_efi::efi::{Status};
use r_efi::efi::{Guid};

// TODO: Figure out a common place for this to live. DebugLib?
#[panic_handler]
#[allow(clippy::empty_loop)]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct VariablePolicyEntry {
    pub version:        u32,
    pub size:           u16,
    pub offset_to_name: u16,
    pub namespace:      Guid,
    pub min_size:       u32,
    pub max_size:       u32,
    pub attr_must_have: u32,
    pub attr_cant_have: u32,
    pub lock_policy_type:   u8,
    pub padding_3:      [u8; 3]
    // pub padding_3:      u8[3]
}

#[no_mangle]
#[export_name = "RegisterVariablePolicy"]
pub extern "win64" fn register_variable_policy (
    new_policy: *const VariablePolicyEntry
    ) -> Status {
    Status::SUCCESS
}
