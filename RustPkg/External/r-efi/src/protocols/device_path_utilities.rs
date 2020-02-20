//! Device Path Utilities Protocol
//!
//! The device-path utilities protocol provides common utilities for creating and manipulating
//! device paths and device nodes.

pub const PROTOCOL_GUID: crate::base::Guid = crate::base::Guid::from_fields(
    0x379be4e, 0xd706, 0x437d, 0xb0, 0x37, &[0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4]
);

#[repr(C)]
pub struct Protocol {
    pub get_device_path_size: eficall!{fn(
        *const crate::protocols::device_path::Protocol,
    ) -> usize},
    pub duplicate_device_path: eficall!{fn(
        *const crate::protocols::device_path::Protocol,
    ) -> *mut crate::protocols::device_path::Protocol},
    pub append_device_path: eficall!{fn(
        *const crate::protocols::device_path::Protocol,
        *const crate::protocols::device_path::Protocol,
    ) -> *mut crate::protocols::device_path::Protocol},
    pub append_device_node: eficall!{fn(
        *const crate::protocols::device_path::Protocol,
        *const crate::protocols::device_path::Protocol,
    ) -> *mut crate::protocols::device_path::Protocol},
    pub append_device_path_instance: eficall!{fn(
        *const crate::protocols::device_path::Protocol,
        *const crate::protocols::device_path::Protocol,
    ) -> *mut crate::protocols::device_path::Protocol},
    pub get_next_device_path_instance: eficall!{fn(
        *mut *mut crate::protocols::device_path::Protocol,
        *mut usize,
        ) -> *mut crate::protocols::device_path::Protocol},
    pub is_device_path_multi_instance: eficall!{fn(
        *const crate::protocols::device_path::Protocol,
    ) -> crate::base::Boolean},
    pub create_device_node: eficall!{fn(
        u8,
        u8,
        u16,
    ) -> *mut crate::protocols::device_path::Protocol},
}
