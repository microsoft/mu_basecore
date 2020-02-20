//! Graphics Output Protocol
//!
//! Provides means to configure graphics hardware and get access to framebuffers. Replaces the old
//! UVA interface from EFI with a VGA-independent API.

pub const PROTOCOL_GUID: crate::base::Guid = crate::base::Guid::from_fields(
    0x9042a9de, 0x23dc, 0x4a38, 0x96, 0xfb, &[0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a]
);

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct PixelBitmask {
    pub red_mask: u32,
    pub green_mask: u32,
    pub blue_mask: u32,
    pub reserved_mask: u32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum GraphicsPixelFormat {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct ModeInformation {
    pub version: u32,
    pub horizontal_resolution: u32,
    pub vertical_resolution: u32,
    pub pixel_format: GraphicsPixelFormat,
    pub pixel_information: PixelBitmask,
    pub pixels_per_scan_line: u32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct Mode {
    pub max_mode: u32,
    pub mode: u32,
    pub info: *mut ModeInformation,
    pub size_of_info: usize,
    pub frame_buffer_base: crate::base::PhysicalAddress,
    pub frame_buffer_size: usize,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct BltPixel {
    pub blue: u8,
    pub green: u8,
    pub red: u8,
    pub reserved: u8,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum BltOperation {
    BltVideoFill,
    BltVideoToBltBuffer,
    BltBufferToVideo,
    BltVideoToVideo,
    BltOperationMax,
}

#[repr(C)]
pub struct Protocol {
    pub query_mode: eficall!{fn(
        *mut Protocol,
        u32,
        *mut usize,
        *mut *mut ModeInformation,
    ) -> crate::base::Status},
    pub set_mode: eficall!{fn(
        *mut Protocol,
        u32,
    ) -> crate::base::Status},
    pub blt: eficall!{fn(
        *mut Protocol,
        *mut BltPixel,
        BltOperation,
        usize,
        usize,
        usize,
        usize,
        usize,
        usize,
        usize,
    ) -> crate::base::Status},
    pub mode: *mut Mode,
}
