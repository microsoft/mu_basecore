// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#![crate_type = "staticlib"]

#![cfg_attr(not(test), no_std)]

#![allow(unused)]

use r_efi::efi;
use r_efi::efi::{Status, PhysicalAddress, CapsuleBlockDescriptor, CapsuleBlockDescriptorUnion, CapsuleHeader,
CAPSULE_FLAGS_PERSIST_ACROSS_RESET, CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE, CAPSULE_FLAGS_INITIATE_RESET, };

pub const MAX_ADDRESS: u64 = 0xFFFFFFFFFFFF;

use core::panic::PanicInfo;
use core::ffi::c_void;

use core::mem::size_of;
use core::mem::transmute;

#[panic_handler]
#[allow(clippy::empty_loop)]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

pub const CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE : u64 = 0x50706143; // 'C', 'a', 'p', 'P'

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct CapsulePeimPrivateData {
    pub signature: u64,
    pub capsule_all_image_size: u64,
    pub capsule_number: u64,
    pub capsule_offset: [u64 ; 1],
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct MemoryResourceDescriptor {
    pub physical_start: PhysicalAddress,
    pub resource_length: u64,
}

const MIN_COALESCE_ADDR : u64 = 0x100000;

fn find_free_mem (
    block_list: *mut CapsuleBlockDescriptor,
    mem_base: *mut u8,
    mem_size: usize,
    data_size: usize,
    ) -> *mut u8
{
    //
    // Need at least enough to copy the data to at the end of the buffer, so
    // say the end is less the data size for easy comparisons here.
    //
    let mut mem_base = mem_base;
    let mem_end : *mut u8 = (mem_base as usize + mem_size - data_size) as *mut u8;
    let mut curr_desc = block_list;
    //
    // Go through all the descriptor blocks and see if any obstruct the range
    //
    while curr_desc != core::ptr::null_mut() {
      //
      // Get the size of this block list and see if it's in the way
      //
      let mut failed = false;
      let mut temp_desc = curr_desc;
      let mut size = size_of::<CapsuleBlockDescriptor>();
    unsafe {
      while (*temp_desc).length != 0 {
        size = size + size_of::<CapsuleBlockDescriptor>();
        temp_desc = temp_desc.offset(1);
      }
    }
      if is_overlapped (mem_base as *mut c_void, data_size, curr_desc as *mut c_void, size) {
        //
        // Set our new base to the end of this block list and start all over
        //
        mem_base   = (curr_desc as usize + size) as *mut u8;
        curr_desc  = block_list;
        if (mem_base > mem_end) {
          return core::ptr::null_mut();
        }

        failed = true;
      }
      //
      // Now go through all the blocks and make sure none are in the way
      //
    unsafe {
      while (*curr_desc).length != 0 && !failed {
        if is_overlapped(mem_base as *mut c_void, data_size, (*curr_desc).data.data_block as *mut c_void, (*curr_desc).length as usize) {
          //
          // Set our new base to the end of this block and start all over
          //
          failed = true;
          mem_base   = ((*curr_desc).data.data_block as usize + (*curr_desc).length as usize) as *mut u8;
          curr_desc  = block_list;
          if mem_base > mem_end {
            return core::ptr::null_mut();
          }
        }
        curr_desc = curr_desc.offset(1);
      }
      //
      // Normal continuation -- jump to next block descriptor list
      //
      if !failed {
        curr_desc = (*curr_desc).data.continuation_pointer as *mut CapsuleBlockDescriptor;
      }
    }
    }

    mem_base
}

fn validate_capsule_by_memory_resource (
    memory_resource : *mut MemoryResourceDescriptor,
    address : PhysicalAddress,
    size : u64,
    ) -> bool
{
    if size > MAX_ADDRESS {
      return false;
    }
    if address > MAX_ADDRESS - size {
      return false;
    }
    if memory_resource == core::ptr::null_mut() {
      return true;
    }

    let mut index = 0;
    loop {
    unsafe {
      if (*memory_resource.offset(index)).resource_length == 0 {
        break;
      }
      if address >= (*memory_resource.offset(index)).physical_start && 
         (address + size) <= (*memory_resource.offset(index)).physical_start + (*memory_resource.offset(index)).resource_length {
        return true;
      }
    }
    }

    false
}

fn validate_capsule_integrity (
    block_list: *mut CapsuleBlockDescriptor,
    memory_resource: *mut MemoryResourceDescriptor
    ) -> *mut CapsuleBlockDescriptor
{
    //
    // Go through the list to look for inconsistencies. Check for:
    //   * misaligned block descriptors.
    //   * The first capsule header guid
    //   * The first capsule header flag
    //   * The first capsule header HeaderSize
    //   * Below check will be done in ValidateCapsuleByMemoryResource()
    //     Length > MAX_ADDRESS
    //     Ptr + sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR) > MAX_ADDRESS
    //     DataBlock + Length > MAX_ADDRESS
    //
    let mut capsule_size  = 0;
    let mut capsule_count = 0;
    let mut ptr = block_list;

    if !validate_capsule_by_memory_resource (memory_resource, ptr as usize as u64, size_of::<CapsuleBlockDescriptor>() as u64) {
      return core::ptr::null_mut();
    }
  unsafe {
    while (*ptr).length != 0 || (*ptr).data.continuation_pointer != 0 {
      //
      // Make sure the descriptor is aligned at UINT64 in memory
      //
      if ((ptr as usize) & 0x7) != 0 {
        return core::ptr::null_mut();
      }

      if (*ptr).length == 0 {
        //
        // Descriptor points to another list of block descriptors somewhere
        // else.
        //
        ptr = (*ptr).data.continuation_pointer as *mut CapsuleBlockDescriptor;
        if !validate_capsule_by_memory_resource (memory_resource, ptr as usize as u64, size_of::<CapsuleBlockDescriptor>() as u64) {
          return core::ptr::null_mut();
        }
      } else {
        if !validate_capsule_by_memory_resource (memory_resource, (*ptr).data.data_block, (*ptr).length) {
          return core::ptr::null_mut();
        }
        //
        //To enhance the reliability of check-up, the first capsule's header is checked here.
        //More reliabilities check-up will do later.
        //
        if capsule_size == 0 {
          //
          //Move to the first capsule to check its header.
          //
          let capsule_header = (*ptr).data.data_block as *mut CapsuleHeader;
          if (*ptr).length < size_of::<CapsuleHeader>() as u64 {
            return core::ptr::null_mut();
          }
          //
          // Make sure HeaderSize field is valid
          //
          if (*capsule_header).header_size > (*capsule_header).capsule_image_size {
            return core::ptr::null_mut();
          }
          if is_capsule_corrupted (capsule_header) {
            return core::ptr::null_mut();
          }
          capsule_count = capsule_count + 1;
          capsule_size = (*capsule_header).capsule_image_size;
        }

        if capsule_size as u64 >= (*ptr).length {
          capsule_size = capsule_size - (*ptr).length as u32;
        } else {
          return core::ptr::null_mut();
        }
        //
        // Move to next BLOCK descriptor
        //
        ptr = ptr.offset(1);
        if !validate_capsule_by_memory_resource (memory_resource, ptr as usize as u64, size_of::<CapsuleBlockDescriptor>() as u64) {
          return core::ptr::null_mut();
        }
      }
    }
  }

    if capsule_count == 0 {
      return core::ptr::null_mut();
    }
    
    if capsule_size != 0 {
      return core::ptr::null_mut();
    }

    ptr
}

fn relocate_block_descriptors (
    block_list: *mut CapsuleBlockDescriptor,
    num_descriptors: usize,
    mem_base: *mut u8,
    mem_size: usize,
    ) -> *mut CapsuleBlockDescriptor
{
    //
    // Get the info on the blocks and descriptors. Since we're going to move
    // the descriptors low in memory, adjust the base/size values accordingly here.
    // NumDescriptors is the number of legit data descriptors, so add one for
    // a terminator. (Already done by caller, no check is needed.)
    //
    let buffer_size = num_descriptors * size_of::<CapsuleBlockDescriptor>();
    let new_block_list : *mut CapsuleBlockDescriptor = mem_base as *mut CapsuleBlockDescriptor;
    if mem_size < buffer_size {
      return core::ptr::null_mut();
    }

    let mem_size = mem_size - buffer_size;
    let mem_base = (mem_base as usize + buffer_size) as *mut u8;
    
    //
    // Go through all the blocks and make sure none are in the way
    //
    let mut temp_block_desc = block_list;
  unsafe {
    while (*temp_block_desc).data.continuation_pointer != 0 {
      if (*temp_block_desc).length == 0 {
        //
        // Next block of descriptors
        //
        temp_block_desc = (*temp_block_desc).data.continuation_pointer as *mut CapsuleBlockDescriptor;
      } else {
        //
        // If the capsule data pointed to by this descriptor is in the way,
        // move it.
        //
        if is_overlapped(new_block_list as *mut c_void, buffer_size, (*temp_block_desc).data.data_block as *mut c_void, (*temp_block_desc).length as usize) {
          //
          // Relocate the block
          //
          let reloc_buffer = find_free_mem(block_list, mem_base, mem_size, (*temp_block_desc).length as usize);
          if reloc_buffer == core::ptr::null_mut() {
            return core::ptr::null_mut();
          }
          core::ptr::copy ((*temp_block_desc).data.data_block as *mut c_void, reloc_buffer as *mut c_void, (*temp_block_desc).length as usize);
          (*temp_block_desc).data.data_block = reloc_buffer as usize as u64;
        }
        temp_block_desc = temp_block_desc.offset(1);
      }
    }

    //
    // Now go through all the block descriptors to make sure that they're not
    // in the memory region we want to copy them to.
    //
    let mut block_list = block_list;
    let mut curr_block_desc_head = block_list;
    let mut prev_block_desc_tail = core::ptr::null_mut() as *mut CapsuleBlockDescriptor;
    while curr_block_desc_head != core::ptr::null_mut() &&
          (*curr_block_desc_head).data.continuation_pointer != 0 {
      //
      // Get the size of this list then see if it overlaps our low region
      //
      temp_block_desc = curr_block_desc_head;
      let mut block_list_size = size_of::<CapsuleBlockDescriptor>();
      while (*temp_block_desc).length != 0 {
        block_list_size = block_list_size + size_of::<CapsuleBlockDescriptor>();
        temp_block_desc = temp_block_desc.offset(1);
      }

      if is_overlapped (new_block_list as *mut c_void, buffer_size, curr_block_desc_head as *mut c_void, block_list_size) {
        //
        // Overlaps, so move it out of the way
        //
        let reloc_buffer = find_free_mem (block_list, mem_base, mem_size, block_list_size);
        if reloc_buffer == core::ptr::null_mut() {
          return core::ptr::null_mut();
        }
        core::ptr::copy (curr_block_desc_head as *mut c_void, reloc_buffer as *mut c_void, block_list_size);
        //
        // Point the previous block's next point to this copied version. If
        // the tail pointer is null, then this is the first descriptor block.
        //
        if prev_block_desc_tail == core::ptr::null_mut() {
          block_list = reloc_buffer as *mut CapsuleBlockDescriptor;
        } else {
          (*prev_block_desc_tail).data.data_block = reloc_buffer as usize as u64;
        }
      }
      //
      // Save our new tail and jump to the next block list
      //
      prev_block_desc_tail = temp_block_desc;
      curr_block_desc_head = (*temp_block_desc).data.continuation_pointer as *mut CapsuleBlockDescriptor;
    }
    //
    // Cleared out low memory. Now copy the descriptors down there.
    //
    temp_block_desc = block_list;
    curr_block_desc_head = new_block_list;
    while temp_block_desc != core::ptr::null_mut() &&
          (*temp_block_desc).data.continuation_pointer != 0 {
      if (*temp_block_desc).length != 0 {
        (*curr_block_desc_head).data.data_block = (*temp_block_desc).data.data_block;
        (*curr_block_desc_head).length = (*temp_block_desc).length;
        curr_block_desc_head = curr_block_desc_head.offset(1);
        temp_block_desc = temp_block_desc.offset(1);
      } else {
        temp_block_desc = (*temp_block_desc).data.continuation_pointer as *mut CapsuleBlockDescriptor;
      }
    }
    //
    // Null terminate
    //
    (*curr_block_desc_head).data.continuation_pointer = 0;
    (*curr_block_desc_head).length = 0;
  }

    new_block_list
}

fn is_overlapped (
    buff1: *mut c_void,
    size1: usize,
    buff2: *mut c_void,
    size2: usize,
    ) -> bool
{
    if (buff1 as usize + size1 <= buff2 as usize) || 
       (buff2 as usize + size2 <= buff1 as usize) {
      return false;
    }
    return true;
}

fn get_capsule_info (
  desc : *mut CapsuleBlockDescriptor,
  ) -> (Status, usize, usize, usize)
{
    let mut count: usize = 0;
    let mut size: usize = 0;
    let mut number: usize = 0;
    let mut this_capsule_image_size: u32 = 0;
    let mut desc : *mut CapsuleBlockDescriptor = desc;

    assert!(desc != core::ptr::null_mut());

  unsafe {
    while (*desc).data.continuation_pointer != 0 {
      if (*desc).length == 0 {
        desc = (*desc).data.continuation_pointer as *mut CapsuleBlockDescriptor;
      } else {
        if (*desc).length >= (MAX_ADDRESS - size as u64) {
          return (Status::OUT_OF_RESOURCES, 0, 0, 0);
        }
        size = size + (*desc).length as usize;
        count = count + 1;
        if this_capsule_image_size == 0 {
          let capsule_header = (*desc).data.data_block as *mut CapsuleHeader;
          number = number + 1;
          this_capsule_image_size = (*capsule_header).capsule_image_size;
        }
        assert!(this_capsule_image_size as u64 >= (*desc).length);
        this_capsule_image_size = this_capsule_image_size - (*desc).length as u32;
        desc = desc.offset(1)
      }
    }
  }
    if count == 0 {
      return (Status::NOT_FOUND, 0, 0, 0)
    }

    assert!(this_capsule_image_size == 0);
 
    (Status::SUCCESS, count, size, number)
}

fn is_capsule_corrupted (
    capsule_header: *mut CapsuleHeader
    ) -> bool
{
  unsafe {
    if (*capsule_header).flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET == 0 {
      return true;
    }
    if (*capsule_header).flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE {
      return true;
    }
    if (*capsule_header).flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_INITIATE_RESET) == CAPSULE_FLAGS_INITIATE_RESET {
      return true;
    }
  }

    return false;
}

fn build_capsule_descriptors (    
    block_list_buffer: *mut PhysicalAddress,
    memory_resource : *mut MemoryResourceDescriptor,
    ) -> (Status, *mut CapsuleBlockDescriptor)
{
    let mut index: isize = 0;
    let mut last_block: *mut CapsuleBlockDescriptor = core::ptr::null_mut();
    let mut head_block: *mut CapsuleBlockDescriptor = core::ptr::null_mut();

  unsafe {
    while *block_list_buffer.offset(index) != 0 {
      if *block_list_buffer.offset(index) < MAX_ADDRESS {
        let temp_block = validate_capsule_integrity(
                       *block_list_buffer.offset(index) as *mut CapsuleBlockDescriptor,
                       memory_resource);
        if !temp_block.is_null() {
          if last_block.is_null() {
            last_block = temp_block;
            head_block = *block_list_buffer.offset(index) as *mut CapsuleBlockDescriptor
          } else {
            (*last_block).data.data_block = *block_list_buffer.offset(index) as PhysicalAddress;
            (*last_block).length = 0;
            last_block = temp_block;
          }
        }
      }
      index = index + 1;
    }
  }
    if !head_block.is_null() {
      return (Status::SUCCESS, head_block);
    } else {
      return (Status::NOT_FOUND, core::ptr::null_mut());
    }
}

#[no_mangle]
#[export_name = "CapsuleDataCoalesce"]
pub extern fn capsule_data_coalesce (
    _: *const c_void,
    block_list_buffer: *mut PhysicalAddress,
    memory_resource : *mut MemoryResourceDescriptor,
    memory_base: *mut *mut c_void,
    memory_size: *mut usize,
    ) -> Status
{
    //
    // Build capsule descriptors list
    //
    let (status, block_list) = build_capsule_descriptors (block_list_buffer, memory_resource);
    if status != Status::SUCCESS {
      return status;
    }

    //
    // Get the size of our descriptors and the capsule size. GetCapsuleInfo()
    // returns the number of descriptors that actually point to data, so add
    // one for a terminator. Do that below.
    //
    let (status, mut num_descriptors, mut capsule_size, capsule_number) = get_capsule_info (block_list);
    if status != Status::SUCCESS {
      return status;
    }

    if num_descriptors == 0 || capsule_size == 0 || capsule_number == 0 {
      return Status::NOT_FOUND;
    }

    if capsule_number - 1 >= (MAX_ADDRESS as usize - (size_of::<CapsulePeimPrivateData>() + size_of::<u64>())) / size_of::<u64>() {
      return Status::BUFFER_TOO_SMALL;
    }

    //
    // Initialize our local copy of private data. When we're done, we'll create a
    // descriptor for it as well so that it can be put into free memory without
    // trashing anything.
    //
    let mut private_data : CapsulePeimPrivateData = CapsulePeimPrivateData {
                             signature : CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE,
                             capsule_all_image_size : capsule_size as u64,
                             capsule_number : capsule_number as u64,
                             capsule_offset : [0 ; 1]
                             };

    //
    // NOTE: Only data in sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) is valid, CapsuleOffset field is uninitialized at this moment.
    // The code sets partial length here for Descriptor.Length check, but later it will use full length to reserve those PrivateData region.
    //
    let mut private_data_desc : [CapsuleBlockDescriptor; 2] = [
                               CapsuleBlockDescriptor {
                                 data : CapsuleBlockDescriptorUnion {
                                   data_block : &mut private_data as *mut CapsulePeimPrivateData as usize as u64,
                                 },
                                 length: size_of::<CapsulePeimPrivateData>() as u64,
                               },
                               CapsuleBlockDescriptor {
                                 data : CapsuleBlockDescriptorUnion {
                                   data_block : block_list as usize as u64,
                                 },
                                 length: 0,
                               },
                             ];
    //
    // Add PrivateDataDesc[0] in beginning, as it is new descriptor. PrivateDataDesc[1] is NOT needed.
    // In addition, one NULL terminator is added in the end. See RelocateBlockDescriptors().
    //
    num_descriptors = num_descriptors + 2;

    if capsule_size >= (MAX_ADDRESS as usize - (size_of::<CapsulePeimPrivateData>() + (capsule_number - 1) * size_of::<u64>() + size_of::<u64>())) {
      return Status::BUFFER_TOO_SMALL;
    }

    //
    // Need add sizeof(UINT64) for PrivateData alignment
    //
    let capsule_size = capsule_size + (size_of::<CapsulePeimPrivateData>() + (capsule_number - 1) * size_of::<u64>() + size_of::<u64>());
    let block_list = &mut private_data_desc[0] as *mut CapsuleBlockDescriptor;

    if num_descriptors >= MAX_ADDRESS as usize / size_of::<CapsuleBlockDescriptor>() {
      return Status::BUFFER_TOO_SMALL;
    }
    let descriptors_size = num_descriptors * size_of::<CapsuleBlockDescriptor>();
    if descriptors_size >= MAX_ADDRESS as usize - capsule_size {
      return Status::BUFFER_TOO_SMALL;
    }

    //
    // Don't go below some min address. If the base is below it,
    // then move it up and adjust the size accordingly.
    //
  unsafe {
    if (*memory_base as usize) < MIN_COALESCE_ADDR as usize {
      if *memory_base as usize + *memory_size < MIN_COALESCE_ADDR as usize {
        return Status::BUFFER_TOO_SMALL;
      } else {
        *memory_size = *memory_size - (MIN_COALESCE_ADDR as usize - *memory_base as usize);
        *memory_base = MIN_COALESCE_ADDR as usize as *mut c_void;
      }
    }

    if *memory_size <= capsule_size + descriptors_size {
      return Status::BUFFER_TOO_SMALL;
    }
  }
    let (free_mem_base, free_mem_size) = unsafe { (*memory_base, *memory_size) };

    //
    // Relocate all the block descriptors to low memory to make further
    // processing easier.
    //
    let block_list = relocate_block_descriptors (block_list, num_descriptors, free_mem_base as *mut u8, free_mem_size);
    if block_list == core::ptr::null_mut() {
      return Status::BUFFER_TOO_SMALL;
    }

    //
    // Take the top of memory for the capsule. UINT64 align up.
    //
    let mut dest_ptr = (free_mem_base as usize + free_mem_size - capsule_size) as *mut u8;
    dest_ptr = ((dest_ptr as usize + 7) & !7usize) as *mut u8;
    let free_mem_base = (block_list as usize + descriptors_size) as *mut u8;
    let free_mem_size = dest_ptr as usize - free_mem_base as usize;
    let new_capsule_base = dest_ptr;
    let capsule_image_base = (new_capsule_base as usize + size_of::<CapsulePeimPrivateData>() + (capsule_number - 1) * size_of::<u64>()) as *mut u8;
    let private_data_ptr = new_capsule_base as *mut CapsulePeimPrivateData;
    
    //
    // Move all the blocks to the top (high) of memory.
    // Relocate all the obstructing blocks. Note that the block descriptors
    // were coalesced when they were relocated, so we can just ++ the pointer.
    //
    let mut capsule_begin_flag = true;
    let mut size_left = 0usize;
    let mut capsule_times = 0;
    let mut capsule_image_size = 0usize;
    let mut capsule_index = 0;
    let mut current_block_desc = block_list;
  unsafe {
    while (*current_block_desc).length != 0 || (*current_block_desc).data.continuation_pointer != 0 {
      let mut dest_length = 0usize;
      if capsule_times == 0 {
        //
        // The first entry is the block descriptor for EFI_CAPSULE_PEIM_PRIVATE_DATA.
        // CapsuleOffset field is uninitialized at this time. No need copy it, but need to reserve for future use.
        //
        assert!((*current_block_desc).data.data_block == &mut private_data as *mut CapsulePeimPrivateData as usize as u64);
        dest_length = size_of::<CapsulePeimPrivateData>() + (capsule_number - 1) * size_of::<u64>();
      } else {
        dest_length = (*current_block_desc).length as usize;
      }
      //
      // See if any of the remaining capsule blocks are in the way
      //
      let mut temp_block_desc = current_block_desc;
      while (*temp_block_desc).length != 0 {
        //
        // Is this block in the way of where we want to copy the current descriptor to?
        //
        if is_overlapped(dest_ptr as *mut c_void, dest_length, (*temp_block_desc).data.data_block as *mut c_void, (*temp_block_desc).length as usize) {
          //
          // Relocate the block
          //
          let reloc_ptr = find_free_mem (block_list, free_mem_base, free_mem_size, (*temp_block_desc).length as usize);
          if reloc_ptr == core::ptr::null_mut() {
            return Status::BUFFER_TOO_SMALL;
          }
          core::ptr::copy ((*temp_block_desc).data.data_block as usize as *mut c_void, reloc_ptr as *mut c_void, (*temp_block_desc).length as usize);

          (*temp_block_desc).data.data_block = reloc_ptr as usize as u64;
        }
        //
        // Next descriptor
        //
        temp_block_desc = temp_block_desc.offset(1);
      }
      //
      // Ok, we made it through. Copy the block.
      // we just support greping one capsule from the lists of block descs list.
      //
      capsule_times = capsule_times + 1;
      //
      //Skip the first block descriptor that filled with EFI_CAPSULE_PEIM_PRIVATE_DATA
      //
      if capsule_times > 1 {
        //
        //For every capsule entry point, check its header to determine whether to relocate it.
        //If it is invalid, skip it and move on to the next capsule. If it is valid, relocate it.
        //
        if capsule_begin_flag {
          capsule_begin_flag = false;
          let capsule_header = (*current_block_desc).data.data_block as *mut CapsuleHeader;
          size_left = (*capsule_header).capsule_image_size as usize;
          //
          // No more check here is needed, because IsCapsuleCorrupted() already in ValidateCapsuleIntegrity()
          //
          assert!(capsule_index <= capsule_number);
          //
          // Relocate this capsule
          //
          capsule_image_size = capsule_image_size + size_left;
          //
          // Cache the begin offset of this capsule
          //
          assert!((*private_data_ptr).signature == CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE);
          assert!(dest_ptr as usize >= capsule_image_base as usize);
          (*private_data_ptr).capsule_offset[capsule_index] = (dest_ptr as usize - capsule_image_base as usize) as u64;
          capsule_index = capsule_index + 1;
        }
        //
        // Below ASSERT is checked in ValidateCapsuleIntegrity()
        //
        assert!((*current_block_desc).length <= size_left as u64);

        core::ptr::copy ((*current_block_desc).data.data_block as usize as *mut c_void, dest_ptr as *mut c_void, (*current_block_desc).length as usize);
        dest_ptr = (dest_ptr as usize + (*current_block_desc).length as usize) as *mut u8;
        size_left = size_left - (*current_block_desc).length as usize;

        if size_left == 0 {
          //
          //Here is the end of the current capsule image.
          //
          capsule_begin_flag = true;
        }
      } else {
        //
        // The first entry is the block descriptor for EFI_CAPSULE_PEIM_PRIVATE_DATA.
        // CapsuleOffset field is uninitialized at this time. No need copy it, but need to reserve for future use.
        //
        assert!((*current_block_desc).length == size_of::<CapsulePeimPrivateData>() as u64);
        assert!(dest_ptr as usize == new_capsule_base as usize);
        core::ptr::copy ((*current_block_desc).data.data_block as *mut c_void, dest_ptr as *mut c_void, (*current_block_desc).length as usize);
        dest_ptr = (dest_ptr as usize + size_of::<CapsulePeimPrivateData>() + (capsule_number - 1) * size_of::<u64>()) as *mut u8;
      }
      //
      //Walk through the block descriptor list.
      //
      current_block_desc = current_block_desc.offset(1);
    }
    //
    // We return the base of memory we want reserved, and the size.
    // The memory peim should handle it appropriately from there.
    //
    *memory_size = capsule_size;
    *memory_base = new_capsule_base as *mut c_void;

    assert!((*private_data_ptr).signature == CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE);
    assert!((*private_data_ptr).capsule_all_image_size == capsule_image_size as u64);
    assert!((*private_data_ptr).capsule_number == capsule_index as u64);
  }
    Status::SUCCESS
}
