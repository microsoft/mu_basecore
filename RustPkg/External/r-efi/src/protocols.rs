//! UEFI Protocols
//!
//! The UEFI Specification splits most of its non-core parts into separate protocols. They can
//! refer to each other, but their documentation and implementation is split apart. We provide
//! each protocol as a separate module, so it is clearly defined where a symbol belongs to.

pub mod decompress;
pub mod device_path;
pub mod device_path_utilities;
pub mod file;
pub mod graphics_output;
pub mod loaded_image;
pub mod loaded_image_device_path;
pub mod simple_file_system;
pub mod simple_text_input;
pub mod simple_text_input_ex;
pub mod simple_text_output;
