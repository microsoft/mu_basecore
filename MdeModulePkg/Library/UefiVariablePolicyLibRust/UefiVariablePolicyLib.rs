#![no_std]
// #![no_main]

use core::panic::PanicInfo;

// use r_efi::efi;
// use r_efi::efi::{Status};

// TODO: Figure out a common place for this to live. DebugLib?
#[panic_handler]
#[allow(clippy::empty_loop)]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

// #[no_mangle]
// #[export_name = "RegisterVariablePolicy"]
// pub extern "win64" fn register_variable_policy (
//     int: Something    
//     ) -> Status {

// }
