//! Simple Text Output Protocol
//!
//! The simple-text-output protocol provides a simple way to print text on screen. It is modeled
//! around the old VGA-consoles, but does not carry all the old cruft. It expects a rectangular
//! text array and allows you to move the cursor around to write Unicode symbols to screen.

pub const PROTOCOL_GUID: crate::base::Guid = crate::base::Guid::from_fields(
    0x387477c2, 0x69c7, 0x11d2, 0x8e, 0x39, &[0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b]
);

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct Mode {
    pub max_mode: i32,
    pub mode: i32,
    pub attribute: i32,
    pub cursor_column: i32,
    pub cursor_row: i32,
    pub cursor_visible: crate::base::Boolean,
}

#[repr(C)]
pub struct Protocol {
    pub reset: eficall!{fn(
        *mut Protocol,
        crate::base::Boolean,
    ) -> crate::base::Status},
    pub output_string: eficall!{fn(
        *mut Protocol,
        *mut crate::base::Char16,
    ) -> crate::base::Status},
    pub test_string: eficall!{fn(
        *mut Protocol,
        *mut crate::base::Char16,
    ) -> crate::base::Status},
    pub query_mode: eficall!{fn(
        *mut Protocol,
        usize,
        *mut usize,
        *mut usize,
    ) -> crate::base::Status},
    pub set_mode: eficall!{fn(
        *mut Protocol,
        usize,
    ) -> crate::base::Status},
    pub set_attribute: eficall!{fn(
        *mut Protocol,
        usize,
    ) -> crate::base::Status},
    pub clear_screen: eficall!{fn(
        *mut Protocol,
    ) -> crate::base::Status},
    pub set_cursor_position: eficall!{fn(
        *mut Protocol,
        usize,
        usize,
    ) -> crate::base::Status},
    pub enable_cursor: eficall!{fn(
        *mut Protocol,
        crate::base::Boolean,
    ) -> crate::base::Status},
    pub mode: *mut Mode,
}
