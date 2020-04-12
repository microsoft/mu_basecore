// @file -- lib.rs
// Implementation of the UefiVariablePolicyLib that is written
// natively in Rust.
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#![allow(unused)]
#![allow(non_snake_case)]

#![cfg_attr(not(test), no_std)]

extern crate alloc;

#[cfg(not(test))]
extern crate uefi_rust_panic_lib;

#[cfg(not(test))]
extern crate uefi_rust_allocation_lib;

#[cfg(not(test))]
extern crate uefi_rust_print_lib_debug_lib;
#[cfg(not(test))]
use uefi_rust_print_lib_debug_lib::println;

use alloc::slice;
use alloc::string::String;
use alloc::vec::Vec;
use core::mem;
use r_efi::efi;

// TODO: Check for truncation in every cast.

//=====================================================================================================================
//
// LIBRARY IMPLEMENTATION
// This section is the UEFI-facing library interface.
// All transition to Rust (with the exception of VariablePolicyEntry::from_raw()) should be here.
//
type EfiGetVariable = extern "win64" fn(_: *const efi::Char16,
                                        _: *const efi::Guid,
                                        _: *mut u32,
                                        _: *mut usize,
                                        _: *mut u8) -> efi::Status;

struct LibState {
  policy_list: VariablePolicyList,
  get_variable_helper: EfiGetVariable,
  interface_locked: bool,
  protection_disabled: bool,
}

static mut INITIALIZED_STATE: Option<LibState> = None;

// Quick helper to assist with CHAR16 -> String.
// TODO: Put this in a single impl.
unsafe fn char16_to_string (string_in: *const efi::Char16) -> String {
  let string_in_u16 = string_in as *const u16;

  // Count the chars in the string.
  let mut string_len: isize = 0;
  while *(string_in_u16.offset(string_len)) != 0 {
    string_len += 1;
  }

  String::from_utf16_lossy(slice::from_raw_parts(string_in_u16, string_len as usize))
}

// Quick helper to wrap EfiGetVariable and turn Rust params into UEFI params.
unsafe fn get_variable(variable_name: &String, vendor_guid: &efi::Guid) -> Result<(Vec<u8>, u32), efi::Status> {
  match &INITIALIZED_STATE {
    Some(lib_state) => {
      let mut variable_name_char16: Vec<u16> = variable_name.encode_utf16().collect();
      let mut data_size: usize = 0;
      let mut attributes: u32 = 0;

      // Set the last char on variable_name_char16.
      variable_name_char16.push(0x0000);

      // Make the first call to get the variable size.
      let mut get_var_status = (lib_state.get_variable_helper)(
        variable_name_char16.as_ptr() as *const efi::Char16,
        vendor_guid as *const efi::Guid,
        &mut attributes as *mut u32,
        &mut data_size as *mut usize,
        0 as *mut u8
        );

      // If and only if the error is BUFFER_TOO_SMALL, try allocating
      // and setting the buffer.
      if get_var_status == efi::Status::BUFFER_TOO_SMALL && data_size > 0 {
        let mut data_buffer: Vec<u8> = Vec::with_capacity(data_size);

        get_var_status = (lib_state.get_variable_helper)(
                              variable_name_char16.as_ptr() as *const efi::Char16,
                              vendor_guid as *const efi::Guid,
                              &mut attributes as *mut u32,
                              &mut data_size as *mut usize,
                              data_buffer.as_mut_ptr()
                              );

        if get_var_status == efi::Status::SUCCESS {
          data_buffer.set_len(data_size);
          return Ok((data_buffer, attributes));
        }
      }

      Err(get_var_status)
    },
    None => Err(efi::Status::NOT_READY)
  }
}

#[no_mangle]
#[export_name = "RegisterVariablePolicy"]
pub extern "win64" fn register_variable_policy(policy_data: *const RawVariablePolicyEntry) -> efi::Status {
  let state = unsafe { &mut INITIALIZED_STATE };

  match state {
    Some(lib_state) => {
      if lib_state.interface_locked { return efi::Status::WRITE_PROTECTED; }

      // First, we need to turn the raw data into an entry.
      match VariablePolicyEntry::from_raw(policy_data) {
        Ok(new_policy) => {
          match lib_state.policy_list.add_policy(new_policy) {
            true => efi::Status::SUCCESS,
            false => efi::Status::ALREADY_STARTED
          }
        }
        Err(err_status) => err_status
      }
    },
    None => efi::Status::NOT_READY
  }
}

#[no_mangle]
#[export_name = "ValidateSetVariable"]
pub extern "win64" fn validate_set_variable (
    variable_name: *const efi::Char16,
    vendor_guid: *const efi::Guid,
    attributes: u32,
    data_size: usize,
    data: *const u8
    ) -> efi::Status {
  let state = unsafe { &INITIALIZED_STATE };

  match state {
    Some(lib_state) => {
      if lib_state.protection_disabled { return efi::Status::SUCCESS; }

      // Turn EFI params into Rust params.
      let my_vendor_guid: efi::Guid = unsafe { *vendor_guid };
      let my_data: &[u8] = unsafe { slice::from_raw_parts(data, data_size) };
      let my_variable_name = unsafe { char16_to_string(variable_name) };

      // Call policy_list.is_set_variable_valid().
      let is_valid = lib_state.policy_list.is_set_variable_valid(&my_variable_name, &my_vendor_guid, attributes, my_data);
      match is_valid {
        true => efi::Status::SUCCESS,
        false => efi::Status::INVALID_PARAMETER
      }
    },
    None => efi::Status::NOT_READY
  }
}

#[no_mangle]
#[export_name = "DisableVariablePolicy"]
pub extern "win64" fn disable_variable_policy (
    ) -> efi::Status {
  let state = unsafe { &mut INITIALIZED_STATE };

  match state {
    Some(lib_state) => {
      if lib_state.protection_disabled { return efi::Status::ALREADY_STARTED; }
      if lib_state.interface_locked { return efi::Status::WRITE_PROTECTED; }

      lib_state.protection_disabled = true;
      efi::Status::SUCCESS
    },
    None => efi::Status::NOT_READY
  }
}

#[no_mangle]
#[export_name = "DumpVariablePolicy"]
pub extern "win64" fn dump_variable_policy (
    policy: *mut u8,
    size: *mut u32
    ) -> efi::Status {
  let state = unsafe { &INITIALIZED_STATE };

  match state {
    Some(lib_state) => {
      unsafe {
        // Validate some initial state.
        if size.is_null()  || (*size > 0 && policy.is_null()) {
          return efi::Status::INVALID_PARAMETER;
        }

        // First, we need to serialize the policy list.
        let policy_buffer = lib_state.policy_list.to_raw();

        if (*size as usize) < policy_buffer.len() {
          *size = policy_buffer.len() as u32;
          efi::Status::BUFFER_TOO_SMALL
        }
        else {
          for index in 0..policy_buffer.len() {
            *(policy.offset(index as isize)) = policy_buffer[index];
          }
          *size = policy_buffer.len() as u32;
          efi::Status::SUCCESS
        }
      }
    },
    None => efi::Status::NOT_READY
  }
}

#[no_mangle]
#[export_name = "IsVariablePolicyEnabled"]
pub extern "win64" fn is_variable_policy_enabled (
    ) -> efi::Boolean {
  let state = unsafe { &INITIALIZED_STATE };

  match state {
    Some(lib_state) => {
      match lib_state.protection_disabled {
        true => efi::Boolean::FALSE,
        false => efi::Boolean::TRUE,
      }
    },
    None => efi::Boolean::FALSE
  }
}

#[no_mangle]
#[export_name = "LockVariablePolicy"]
pub extern "win64" fn lock_variable_policy (
    ) -> efi::Status {
  let state = unsafe { &mut INITIALIZED_STATE };

  match state {
    Some(lib_state) => {
      match lib_state.interface_locked {
        true => efi::Status::WRITE_PROTECTED,
        false => {
          lib_state.interface_locked = true;
          efi::Status::SUCCESS
        }
      }
    },
    None => efi::Status::NOT_READY
  }
}

#[no_mangle]
#[export_name = "IsVariablePolicyInterfaceLocked"]
pub extern "win64" fn is_variable_policy_interface_locked (
    ) -> efi::Boolean {
  let state = unsafe { &INITIALIZED_STATE };

  match state {
    Some(lib_state) => {
      match lib_state.interface_locked {
        true => efi::Boolean::TRUE,
        false => efi::Boolean::FALSE,
      }
    },
    None => efi::Boolean::FALSE
  }
}

#[no_mangle]
#[export_name = "InitVariablePolicyLib"]
pub extern "win64" fn init_variable_policy_lib (
    get_variable_helper: EfiGetVariable
    ) -> efi::Status {
  let state = unsafe { &INITIALIZED_STATE };

  match state {
    Some(_) => efi::Status::ALREADY_STARTED,
    None => {
      let new_state = Some(LibState {
        policy_list: VariablePolicyList::new(),
        get_variable_helper: get_variable_helper,
        interface_locked: false,
        protection_disabled: false,
      });
      unsafe { INITIALIZED_STATE = new_state; }

      efi::Status::SUCCESS
    }
  }
}

#[no_mangle]
#[export_name = "IsVariablePolicyLibInitialized"]
pub extern "win64" fn is_variable_policy_lib_initialized (
    ) -> efi::Boolean {
  let state = unsafe { &INITIALIZED_STATE };
  match state {
    Some(_) => efi::Boolean::TRUE,
    None => efi::Boolean::FALSE
  }
}

#[no_mangle]
#[export_name = "DeinitVariablePolicyLib"]
pub extern "win64" fn deinit_variable_policy_lib (
    ) -> efi::Status {
  let state = unsafe { &INITIALIZED_STATE };
  match state {
    Some(_) => {
      // TODO: Make sure that this deinits everything.
      unsafe{ INITIALIZED_STATE = None; }
      efi::Status::SUCCESS
    },
    None => efi::Status::NOT_READY
  }
}
//=====================================================================================================================


#[derive(Debug)]
enum LockPolicyType {
  NoLock,
  LockNow,
  LockOnCreate,
  LockOnVarState(VarStateLock)
}

#[allow(non_snake_case)]
mod RawLockPolicyType {
  pub const NO_LOCK: u8            = 0;
  pub const LOCK_NOW: u8           = 1;
  pub const LOCK_ON_CREATE: u8     = 2;
  pub const LOCK_ON_VAR_STATE: u8  = 3;

  pub fn is_valid(raw_lock_type: u8) -> bool {
    match raw_lock_type {
      NO_LOCK | LOCK_NOW | LOCK_ON_CREATE | LOCK_ON_VAR_STATE => true,
      _ => false
    }
  }
}

#[derive(Debug)]
struct VariablePolicyEntry {
  min_size:           u32,
  max_size:           u32,
  attr_must_have:     u32,
  attr_cant_have:     u32,
  lock_policy_type:   LockPolicyType,
  namespace:          efi::Guid,
  name:               Option<String>,
}

#[derive(Debug)]
struct VarStateLock {
  namespace: efi::Guid,
  name:      String,
  value:     u8,
}

#[derive(Debug)]
struct VariablePolicyList {
  inner: Vec<VariablePolicyEntry>,
}

#[repr(C)]
#[derive(Debug)]
pub struct RawVariablePolicyEntry {
  version:            u32,
  size:               u16,
  offset_to_name:     u16,
  namespace:          efi::Guid,
  min_size:           u32,
  max_size:           u32,
  attr_must_have:     u32,
  attr_cant_have:     u32,
  lock_policy_type:   u8,
  padding_3:          [u8; 3],
  // lock_policy:     RawVarStateLock,
  // name:            [efi::Char16]
}
// Hardcode these sizes because even with repr(C), Rust calculates the size too large.
const RAW_VARIABLE_POLICY_ENTRY_SIZE: usize = mem::size_of::<u32>()
                                                + mem::size_of::<u16>()
                                                + mem::size_of::<u16>()
                                                + mem::size_of::<efi::Guid>()
                                                + mem::size_of::<u32>()
                                                + mem::size_of::<u32>()
                                                + mem::size_of::<u32>()
                                                + mem::size_of::<u32>()
                                                // lock_policy_type and padding_3
                                                + mem::size_of::<u32>();

#[repr(C)]
#[derive(Debug)]
struct RawVarStateLock {
  namespace:          efi::Guid,
  value:              u8,
  padding:            u8,
  // name:            [efi::Char16]
}
// Hardcode these sizes because even with repr(C), Rust calculates the size too large.
const RAW_VAR_STATE_LOCK_SIZE: usize = mem::size_of::<efi::Guid>()
                                        + mem::size_of::<u8>()
                                        + mem::size_of::<u8>();

impl VariablePolicyEntry {
  const RAW_ENTRY_REVISION: u32     = 0x0001_0000;

  const NO_MIN_SIZE: u32          = 0;
  const NO_MAX_SIZE: u32          = 0xffff_ffff;
  const NO_MUST_ATTR: u32         = 0;
  const NO_CANT_ATTR: u32         = 0;

  const MATCH_PRIORITY_EXACT: u8  = 0;
  const MATCH_PRIORITY_MAX: u8    = Self::MATCH_PRIORITY_EXACT;
  const MATCH_PRIORITY_MIN: u8    = 0xff;

  // TODO: Perhaps make new() return a Result<> so that we can validate
  //        all input parameters. Then we can have an unsafe unchecked_new() that
  //        would return an instance without validating anything.
  pub fn new(
    min_size: u32,
    max_size: u32,
    attr_must_have: u32,
    attr_cant_have: u32,
    lock_policy_type: LockPolicyType,
    namespace: efi::Guid,
    name: Option<String>
    ) -> Self
  {
    VariablePolicyEntry {
      min_size,
      max_size,
      attr_must_have,
      attr_cant_have,
      lock_policy_type,
      namespace,
      name: match name {
        Some(name_string) => Some(String::from(name_string)),
        None => None
      }
    }
  }

  pub fn is_valid(&self) -> bool {
    // If the lock type is StateVar, string must not be empty.
    if let LockPolicyType::LockOnVarState(var_state) = &self.lock_policy_type {
      if var_state.name.len() < 1 {
        return false;
      }
    }

    // Check for a valid MAX value.
    if self.max_size == 0 {
      return false;
    }


    // If a var_name is provided, validate it.
    if let Some(var_name) = &self.name {
      // Make sure the string is at least one character.
      if var_name.len() < 1 ||
          // Make sure there aren't too many wildcards.
          var_name.matches("#").count() > Self::MATCH_PRIORITY_MIN as usize {
        return false;
      }
      // TODO: Make sure that all characters are valid for a var name.
    }

    true
  }

  // TODO: Have this return specific error codes, rather than EFI codes.
  //       Can map to EFI codes in the caller, if it's an EFI interface.
  pub fn from_raw(raw_var_policy_header: *const RawVariablePolicyEntry) -> Result<Self, efi::Status> {
    if raw_var_policy_header.is_null() {
      return Err(efi::Status::INVALID_PARAMETER);
    }

    let mut new_policy = VariablePolicyEntry {
      min_size: Self::NO_MIN_SIZE, max_size: Self::NO_MAX_SIZE,
      attr_must_have: Self::NO_MUST_ATTR, attr_cant_have: Self::NO_CANT_ATTR,
      lock_policy_type: LockPolicyType::NoLock,
      namespace: efi::Guid::from_fields(0, 0, 0, 0, 0, &[0, 0, 0, 0, 0, 0]),
      name: None
    };

    // Validate the first few fields of the structure.
    // If the header looks valid, start grabbing fields.
    unsafe {
      // Sanitize some quick values.
      if (*raw_var_policy_header).version != VariablePolicyEntry::RAW_ENTRY_REVISION ||
          ((*raw_var_policy_header).size as usize) < RAW_VARIABLE_POLICY_ENTRY_SIZE ||
          (*raw_var_policy_header).size < (*raw_var_policy_header).offset_to_name {
            return Err(efi::Status::INVALID_PARAMETER);
          }

      new_policy.min_size = (*raw_var_policy_header).min_size;
      new_policy.max_size = (*raw_var_policy_header).max_size;
      new_policy.attr_must_have = (*raw_var_policy_header).attr_must_have;
      new_policy.attr_cant_have = (*raw_var_policy_header).attr_cant_have;
      new_policy.namespace = (*raw_var_policy_header).namespace;

      let data_buffer = raw_var_policy_header as *const u8;
      let entry_end = data_buffer.offset((*raw_var_policy_header).size as isize) as usize;

      // Check for the valid list of lock policies.
      new_policy.lock_policy_type = match (*raw_var_policy_header).lock_policy_type {
        RawLockPolicyType::NO_LOCK => LockPolicyType::NoLock,
        RawLockPolicyType::LOCK_NOW => LockPolicyType::LockNow,
        RawLockPolicyType::LOCK_ON_CREATE => LockPolicyType::LockOnCreate,
        RawLockPolicyType::LOCK_ON_VAR_STATE => {
          let raw_var_state_header = data_buffer
                                        .offset(RAW_VARIABLE_POLICY_ENTRY_SIZE as isize)
                                        as *const RawVarStateLock;

          // If the policy type is VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE, make sure that the matching state variable Name
          // terminates before the OffsetToName for the matching policy variable Name.
          let mut string_len: isize   = 0;
          let state_name_string = (raw_var_state_header as *const u8)
                                    .offset((RAW_VAR_STATE_LOCK_SIZE) as isize)
                                    as *const u16;
          while *(state_name_string.offset(string_len)) != 0 {
            if entry_end <= state_name_string.offset(string_len) as usize {
              return Err(efi::Status::INVALID_PARAMETER);
            }
            string_len += 1;
          }

          // At this point we should have either exeeded the structure or be pointing at the last char in LockPolicy->Name.
          // We should check to make sure that the policy Name comes immediately after this charcter.
          if state_name_string.offset(string_len+1) as usize !=
              data_buffer.offset((*raw_var_policy_header).offset_to_name as isize) as usize {
            return Err(efi::Status::INVALID_PARAMETER);
          }

          LockPolicyType::LockOnVarState(VarStateLock {
            namespace: (*raw_var_state_header).namespace,
            value: (*raw_var_state_header).value,
            name: String::from_utf16_lossy(slice::from_raw_parts(state_name_string, string_len as usize)),
          })
        },
        _ => return Err(efi::Status::INVALID_PARAMETER)
      };

      // Make sure that OffsetToName is exactly sizeof(VARIABLE_POLICY_ENTRY) if not VarState.
      // TODO: Find a better way to do this.
      if let LockPolicyType::LockOnVarState(_) = new_policy.lock_policy_type {
      }
      else {
        if (*raw_var_policy_header).offset_to_name != RAW_VARIABLE_POLICY_ENTRY_SIZE as u16 {
          return Err(efi::Status::INVALID_PARAMETER);
        }
      }

      // Check to make sure that the name has a terminating character
      // before the end of the structure.
      // We've already checked that the name is within the bounds of the structure.
      if (*raw_var_policy_header).size != (*raw_var_policy_header).offset_to_name {
        let mut string_len: isize = 0;
        let name_string = (raw_var_policy_header as *const u8)
                            .offset((*raw_var_policy_header).offset_to_name as isize)
                            as *const u16;
        while *(name_string.offset(string_len)) != 0 {
          if entry_end <= name_string.offset(string_len) as usize {
            return Err(efi::Status::INVALID_PARAMETER);
          }
          string_len += 1;
        }

        // Finally, we should be pointed at the very last character in Name, so we should be right
        // up against the end of the structure.
        if name_string.offset(string_len+1) as usize != entry_end {
          return Err(efi::Status::INVALID_PARAMETER);
        }

        new_policy.name = Some(
          String::from_utf16_lossy(slice::from_raw_parts(name_string, string_len as usize))
          );
      }
    }

    if new_policy.is_valid() {
      Ok(new_policy)
    }
    else {
      Err(efi::Status::INVALID_PARAMETER)
    }
  }

  fn to_raw(&self) -> Vec<u8> {
    // Start figuring out how much memory we will need to hold this thing.
    let mut buffer_size: usize = RAW_VARIABLE_POLICY_ENTRY_SIZE;
    if let LockPolicyType::LockOnVarState(var_state) = &self.lock_policy_type {
      buffer_size += RAW_VAR_STATE_LOCK_SIZE;
      // Add a sufficient number of CHAR16 + NULL.
      buffer_size += (var_state.name.len() + 1) * mem::size_of::<efi::Char16>();
    }
    let name_offset: usize = buffer_size;
    if let Some(var_name) = &self.name {
      // Add a sufficient number of CHAR16 + NULL.
      buffer_size += (var_name.len() + 1) * mem::size_of::<efi::Char16>();
    }

    // Great, now we've got a size, we can theoretically carve off some memory.
    let mut raw_buffer: Vec<u8> = Vec::with_capacity(buffer_size);
    let raw_buffer_ptr: *mut u8 = raw_buffer.as_mut_ptr();

    // Heck yes. Start, uh... filling things in.
    unsafe {
      // Start by setting all of the header data.
      let var_pol_header = raw_buffer_ptr as *mut RawVariablePolicyEntry;
      (*var_pol_header).version = VariablePolicyEntry::RAW_ENTRY_REVISION;
      (*var_pol_header).size = buffer_size as u16;
      (*var_pol_header).offset_to_name = name_offset as u16;
      (*var_pol_header).namespace = self.namespace;
      (*var_pol_header).min_size = self.min_size;
      (*var_pol_header).max_size = self.max_size;
      (*var_pol_header).attr_cant_have = self.attr_cant_have;
      (*var_pol_header).attr_must_have = self.attr_must_have;
      (*var_pol_header).lock_policy_type = match &self.lock_policy_type {
        LockPolicyType::NoLock => RawLockPolicyType::NO_LOCK,
        LockPolicyType::LockNow => RawLockPolicyType::LOCK_NOW,
        LockPolicyType::LockOnCreate => RawLockPolicyType::LOCK_ON_CREATE,
        LockPolicyType::LockOnVarState(_) => RawLockPolicyType::LOCK_ON_VAR_STATE
      };
      (*var_pol_header).padding_3 = [0x00, 0x00, 0x00];

      // Now update the VarStateLock, if set.
      if let LockPolicyType::LockOnVarState(var_state) = &self.lock_policy_type {
        let var_state_lock_header = raw_buffer_ptr.offset(RAW_VARIABLE_POLICY_ENTRY_SIZE as isize) as *mut RawVarStateLock;
        (*var_state_lock_header).namespace = var_state.namespace;
        (*var_state_lock_header).value = var_state.value;
        (*var_state_lock_header).padding = 0;

        // Need to copy the var_state string to the correct offset.
        // TODO: Make sure this section behaves correctly for possible double chars.
        let var_state_name_ptr = (var_state_lock_header as *mut u8)
                                    .offset(RAW_VAR_STATE_LOCK_SIZE as isize)
                                    as *mut u16;
        let mut char16_iter = var_state.name.encode_utf16().enumerate();
        while let Some((index, char)) = char16_iter.next() {
          *(var_state_name_ptr.offset(index as isize)) = char;
        }
        // Set the terminating NULL.
        *(var_state_name_ptr.offset(var_state.name.len() as isize)) = 0x0000;
      }

      // Now update the name string, if provided.
      // TODO: Make sure this section behaves correctly for possible double chars.
      if let Some(var_name) = &self.name {
        let var_name_ptr = raw_buffer_ptr.offset(name_offset as isize) as *mut u16;
        let mut char16_iter = var_name.encode_utf16().enumerate();
        while let Some((index, char)) = char16_iter.next() {
          *(var_name_ptr.offset(index as isize)) = char;
        }
        // Set the terminating NULL.
        *(var_name_ptr.offset(var_name.len() as isize)) = 0x0000;
      }

      // Finally, update the contents of the Vector.
      raw_buffer.set_len(buffer_size);
    } // unsafe {}

    raw_buffer
  }

  fn eval_match(&self, variable_name_option: Option<&String>, vendor_guid: &efi::Guid) -> Option<u8> {
    let mut match_priority: Option<u8> = None;

    // If the namespaces don't match, we're done here.
    if self.namespace != *vendor_guid {
      return None;
    }

    // If the variable_name_option is None, there are only two options...
    // Either it's a perfect match agains a namespace-wildcard policy, or it's a complete failure.
    if variable_name_option.is_none() {
      match self.name.is_none() {
        true => return Some(Self::MATCH_PRIORITY_EXACT),
        false => return None,
      }
    }

    // Now that we're here, we know we have at least some variable_name...
    let variable_name = variable_name_option.unwrap();

    // If there is a string, process further.
    if let Some(name_string) = &self.name {
      // Now that we know there's a string match requirement
      // check to see whether it matches.
      if name_string.len() == variable_name.len() {
        // We can now assume they match and keep going until they don't.
        match_priority = Some(Self::MATCH_PRIORITY_EXACT);

        // Grab the characters and start walking them...
        let mut left_chars = name_string.chars();
        let mut right_chars = variable_name.chars();

        // Walk through all the characters.
        while let (Some(left_char), Some(right_char)) = (left_chars.next(), right_chars.next()) {
          // If they're not equal, need to check for '#' and digits.
          if left_char != right_char || left_char == '#' {
            // If this is a valid wildcard, increment the match priority.
            if left_char == '#' && right_char.is_ascii_digit() {
              if match_priority < Some(Self::MATCH_PRIORITY_MIN) {
                match_priority = Some(match_priority.unwrap() + 1);
              }
            }
            // Otherwise, we've failed to match.
            else {
              match_priority = None;
              break;
            }
          }
        }
      }
    }
    // Otherwise, we're done here.
    else {
      match_priority = Some(Self::MATCH_PRIORITY_MIN);
    }

    match_priority
  }
}

impl VariablePolicyList {
  pub fn new() -> Self {
    VariablePolicyList { inner: Vec::new() }
  }

  pub fn add_policy(&mut self, policy: VariablePolicyEntry) -> bool {
    // Make sure that there are no duplicates of this policy.
    // TODO: For some stupid reason this works. Find out why and what would be better.
    let match_name = match &policy.name {
      Some(inner_name) => Some(inner_name),
      None => None,
    };
    let (match_policy, match_priority) = self.get_best_match(match_name, &policy.namespace);

    // If there was a match and the match_priority is exact, we have a duplicate.
    if match_policy.is_some() && match_priority == VariablePolicyEntry::MATCH_PRIORITY_EXACT {
      false
    }
    // Otherwise, add it to the list and let's move on.
    else {
      self.inner.push(policy);
      true
    }
  }

  pub fn to_raw(&self) -> Vec<u8> {
    let mut output_buffer: Vec<u8> = Vec::new();
    for next_policy in &self.inner {
      output_buffer.extend(next_policy.to_raw());
    }
    output_buffer
  }

  pub fn get_best_match(&self, variable_name_option: Option<&String>, vendor_guid: &efi::Guid) -> (Option<&VariablePolicyEntry>, u8) {
    let mut match_priority = VariablePolicyEntry::MATCH_PRIORITY_MIN;
    let mut current_match: Option<&VariablePolicyEntry> = None;

    for next_policy in &self.inner {
      if let Some(policy_priority) = next_policy.eval_match(variable_name_option, vendor_guid) {
        if current_match.is_none() || policy_priority < match_priority {
          current_match = Some(&next_policy);
          match_priority = policy_priority;
        }
      }
    }

    (current_match, match_priority)
  }

  pub fn is_set_variable_valid(
    &self, variable_name: &String, vendor_guid: &efi::Guid, attributes: u32, data: &[u8]
    ) -> bool
  {
    // Step one: figure out if we have a policy that controls this variable.
    let (match_policy, _) = self.get_best_match(Some(variable_name), vendor_guid);

    // Determine whether this is a delete operation.
    // If so, it will affect which tests are applied.
    let is_del = if data.len() == 0 && (attributes & r_efi::system::VARIABLE_APPEND_WRITE) == 0 {
      true
    }
    else {
      false
    };

    // Only matters if we have a matching policy.
    // If we have an active policy, check it against the incoming data.
    if let Some(matched_policy) = match_policy {
      // Only enforce size and attribute constraints when updating data, not deleting.
      if !is_del {
        // Check for size constraints.
        if (matched_policy.min_size > 0 && (data.len() as u32) < matched_policy.min_size) ||
              (matched_policy.max_size > 0 && data.len() as u32 > matched_policy.max_size) {
          return false;
        }

        // Check for attribute constraints.
        if (matched_policy.attr_must_have & attributes) != matched_policy.attr_must_have ||
            (matched_policy.attr_cant_have & attributes) != 0 {
          return false;
        }
      }

      // Evaluate lock policy.
      match &matched_policy.lock_policy_type {
        LockPolicyType::NoLock => (),
        LockPolicyType::LockNow => return false,

        // If LockOnCreate, we should only pass if NOT_FOUND.
        // Otherwise, can go ahead and return an error.
        LockPolicyType::LockOnCreate => {
          match unsafe { get_variable(variable_name, vendor_guid) } {
            Ok(_) => return false,
            Err(err_status) => {
              match err_status {
                efi::Status::NOT_FOUND => (),
                _ => return false
              }
            }
          }
        },
        LockPolicyType::LockOnVarState(var_state) => {
          match unsafe { get_variable(&var_state.name, &var_state.namespace) } {
            Ok((var_data, var_attr)) => {
              if var_data.len() == 1 && var_data[0] == var_state.value {
                return false;
              }
            },
            Err(err_status) => {
              if err_status != efi::Status::NOT_FOUND {
                return false;
              }
            }
          }
        }
      }
    }

    true
  }
}

#[cfg(test)]
mod tests {
  use r_efi::efi;
  use super::*;

  const TEST_VAR_NAME_1: &str = "TestVar1";
  const TEST_VAR_NAME_2: &str = "TestVar2";
  const TEST_VAR_NAME_3: &str = "TestVar3";

  // {F955BA2D-4A2C-480C-BFD1-3CC522610592}
  const TEST_VAR_GUID_1: efi::Guid = efi::Guid::from_fields(0xf955ba2d, 0x4a2c, 0x480c, 0xbf, 0xd1, &[0x3c, 0xc5, 0x22, 0x61, 0x5, 0x92]);
  // {2DEA799E-5E73-43B9-870E-C945CE82AF3A}
  const TEST_VAR_GUID_2: efi::Guid = efi::Guid::from_fields(0x2dea799e, 0x5e73, 0x43b9, 0x87, 0xe, &[0xc9, 0x45, 0xce, 0x82, 0xaf, 0x3a]);
  // {698A2BFD-A616-482D-B88C-7100BD6682A9}
  const TEST_VAR_GUID_3: efi::Guid = efi::Guid::from_fields(0x698a2bfd, 0xa616, 0x482d, 0xb8, 0x8c, &[0x71, 0x0, 0xbd, 0x66, 0x82, 0xa9]);

  const TEST_POLICY_ATTRIBUTES_NULL: u32     = VariablePolicyEntry::NO_CANT_ATTR;
  const TEST_POLICY_MIN_SIZE_NULL: u32       = VariablePolicyEntry::NO_MIN_SIZE;
  const TEST_POLICY_MAX_SIZE_NULL: u32       = VariablePolicyEntry::NO_MAX_SIZE;

  const TEST_POLICY_MIN_SIZE_10: u32         = 10;
  const TEST_POLICY_MAX_SIZE_200: u32        = 200;

  const TEST_300_HASHES_STRING: &str      = concat!("##################################################",
                                                    "##################################################",
                                                    "##################################################",
                                                    "##################################################",
                                                    "##################################################",
                                                    "##################################################");

  // Based off of code from:
  // https://github.com/teg/r-efi-string/blob/master/src/lib.rs
  unsafe fn string_to_char16(input: &String, output: *mut u16) {
    let mut len: isize = 0;
    for c in input.chars() {
      match c as u32 {
        0x0001 ..= 0xd7ff | 0xf900 ..= 0xffff => {
          *output.offset(len) = c as u16;
          len += 1;
        },
        _ => assert!(false, "STRING NO BUENO! {}", input)
      }
    }
    *output.offset(input.len() as isize) = 0 as u16;
  }

  fn variable_policy_entry_to_vec_u8(entry_in: &VariablePolicyEntry) -> Vec<u8> {
    // Start figuring out how much memory we will need to hold this thing.
    let mut buffer_size: usize = RAW_VARIABLE_POLICY_ENTRY_SIZE;
    if let LockPolicyType::LockOnVarState(var_state) = &entry_in.lock_policy_type {
      buffer_size += RAW_VAR_STATE_LOCK_SIZE;
      // Add a sufficient number of CHAR16 + NULL.
      buffer_size += (var_state.name.len() + 1) * mem::size_of::<efi::Char16>();
    }
    let name_offset: usize = buffer_size;
    if let Some(var_name) = &entry_in.name {
      // Add a sufficient number of CHAR16 + NULL.
      buffer_size += (var_name.len() + 1) * mem::size_of::<efi::Char16>();
    }

    // Great, now we've got a size, we can theoretically carve off some memory.
    let mut raw_buffer: Vec<u8> = Vec::with_capacity(buffer_size);
    let raw_buffer_ptr: *mut u8 = raw_buffer.as_mut_ptr();

    // Heck yes. Start, uh... filling things in.
    unsafe {
      // Start by setting all of the header data.
      let var_pol_header = raw_buffer_ptr as *mut RawVariablePolicyEntry;
      (*var_pol_header).version = VariablePolicyEntry::RAW_ENTRY_REVISION;
      (*var_pol_header).size = buffer_size as u16;
      (*var_pol_header).offset_to_name = name_offset as u16;
      (*var_pol_header).namespace = entry_in.namespace;
      (*var_pol_header).min_size = entry_in.min_size;
      (*var_pol_header).max_size = entry_in.max_size;
      (*var_pol_header).attr_cant_have = entry_in.attr_cant_have;
      (*var_pol_header).attr_must_have = entry_in.attr_must_have;
      (*var_pol_header).lock_policy_type = match &entry_in.lock_policy_type {
        LockPolicyType::NoLock => RawLockPolicyType::NO_LOCK,
        LockPolicyType::LockNow => RawLockPolicyType::LOCK_NOW,
        LockPolicyType::LockOnCreate => RawLockPolicyType::LOCK_ON_CREATE,
        LockPolicyType::LockOnVarState(_) => RawLockPolicyType::LOCK_ON_VAR_STATE
      };
      (*var_pol_header).padding_3 = [0x00, 0x00, 0x00];

      // Now update the VarStateLock, if set.
      if let LockPolicyType::LockOnVarState(var_state) = &entry_in.lock_policy_type {
        let var_state_lock_header = raw_buffer_ptr.offset(RAW_VARIABLE_POLICY_ENTRY_SIZE as isize) as *mut RawVarStateLock;
        (*var_state_lock_header).namespace = var_state.namespace;
        (*var_state_lock_header).value = var_state.value;
        (*var_state_lock_header).padding = 0;

        // Need to copy the var_state string to the correct offset.
        string_to_char16(&var_state.name, (var_state_lock_header as *mut u8).offset(RAW_VAR_STATE_LOCK_SIZE as isize) as *mut u16);
      }

      // Now update the name string, if provided.
      if let Some(var_name) = &entry_in.name {
        string_to_char16(&var_name, raw_buffer_ptr.offset(name_offset as isize) as *mut u16);
      }

      // Finally, update the contents of the Vector.
      raw_buffer.set_len(buffer_size);
    }

    raw_buffer
  }

  #[test]
  fn policies_should_match_by_name_and_guid() {
    let new_policy = VariablePolicyEntry::new(
      TEST_POLICY_MIN_SIZE_NULL, TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL, TEST_POLICY_ATTRIBUTES_NULL,
      LockPolicyType::NoLock, TEST_VAR_GUID_1, Some(String::from(TEST_VAR_NAME_1))
      );

    assert!(new_policy.eval_match(Some(&String::from(TEST_VAR_NAME_2)), &TEST_VAR_GUID_1).is_none());
    assert!(new_policy.eval_match(Some(&String::from(TEST_VAR_NAME_1)), &TEST_VAR_GUID_2).is_none());

    assert!(new_policy.eval_match(Some(&String::from(TEST_VAR_NAME_1)), &TEST_VAR_GUID_1).is_some());
  }

  #[test]
  fn wildcard_policies_should_match_digits() {
    let new_policy = VariablePolicyEntry::new(
      TEST_POLICY_MIN_SIZE_NULL, TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL, TEST_POLICY_ATTRIBUTES_NULL,
      LockPolicyType::NoLock, TEST_VAR_GUID_1, Some(String::from("Wildcard#VarName##"))
      );

    let check_var1_name = "Wildcard1VarName12";
    let check_var2_name = "Wildcard2VarName34";
    let check_var_b_name = "WildcardBVarName56";
    let check_var_h_name = "Wildcard#VarName56";

    // Make sure that two different sets of wildcard numbers match.
    assert!(new_policy.eval_match(Some(&String::from(check_var1_name)), &TEST_VAR_GUID_1).is_some());
    assert!(new_policy.eval_match(Some(&String::from(check_var2_name)), &TEST_VAR_GUID_1).is_some());

    // Make sure that the non-number charaters don't match.
    assert!(new_policy.eval_match(Some(&String::from(check_var_b_name)), &TEST_VAR_GUID_1).is_none());

    // Make sure that '#' signs don't match.
    assert!(new_policy.eval_match(Some(&String::from(check_var_h_name)), &TEST_VAR_GUID_1).is_none());
  }

  #[test]
  fn wildcard_policies_should_match_digits_advanced() {
    let new_policy = VariablePolicyEntry::new(
      TEST_POLICY_MIN_SIZE_NULL, TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL, TEST_POLICY_ATTRIBUTES_NULL,
      LockPolicyType::NoLock, TEST_VAR_GUID_1, Some(String::from(TEST_300_HASHES_STRING))
      );

    let check_shorter_string = "01234567890123456789012345678901234567890123456789";
    let check_valid_string = concat!("01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789");
    let check_longer_string = concat!("01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789",
                                      "01234567890123456789012345678901234567890123456789");

    // Make sure that the shorter and the longer do not match.
    assert!(new_policy.eval_match(Some(&String::from(check_shorter_string)), &TEST_VAR_GUID_1).is_none());
    assert!(new_policy.eval_match(Some(&String::from(check_longer_string)), &TEST_VAR_GUID_1).is_none());

    // Make sure that the valid one matches and has the expected priority.
    let match_eval = new_policy.eval_match(Some(&String::from(check_valid_string)), &TEST_VAR_GUID_1);
    assert_eq!(match_eval, Some(VariablePolicyEntry::MATCH_PRIORITY_MIN));
  }

  #[test]
  fn wildcard_policies_should_match_namespaces() {
    let new_policy = VariablePolicyEntry::new(
      TEST_POLICY_MIN_SIZE_NULL, TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL, TEST_POLICY_ATTRIBUTES_NULL,
      LockPolicyType::NoLock, TEST_VAR_GUID_1, None
      );

    let check_var1_name = "Wildcard1VarName12";
    let check_var2_name = "Wildcard2VarName34";
    let check_var_b_name = "WildcardBVarName56";
    let check_var_h_name = "Wildcard#VarName56";

    // Make sure that all names in the same namespace match.
    assert!(new_policy.eval_match(Some(&String::from(check_var1_name)), &TEST_VAR_GUID_1).is_some());
    assert!(new_policy.eval_match(Some(&String::from(check_var2_name)), &TEST_VAR_GUID_1).is_some());
    assert!(new_policy.eval_match(Some(&String::from(check_var_b_name)), &TEST_VAR_GUID_1).is_some());
    assert!(new_policy.eval_match(Some(&String::from(check_var_h_name)), &TEST_VAR_GUID_1).is_some());

    // Make sure that different namespace doesn't match.
    assert!(new_policy.eval_match(Some(&String::from(check_var1_name)), &TEST_VAR_GUID_2).is_none());
  }

  #[test]
  fn none_strings_should_not_match_strings() {
    let new_policy = VariablePolicyEntry::new(
      TEST_POLICY_MIN_SIZE_NULL, TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL, TEST_POLICY_ATTRIBUTES_NULL,
      LockPolicyType::NoLock, TEST_VAR_GUID_1, Some(String::from(TEST_VAR_NAME_1))
      );

    // Make sure that passing None does not match this at all.
    assert!(new_policy.eval_match(None, &TEST_VAR_GUID_1).is_none());
  }

  #[test]
  fn none_strings_should_match_wildcard_policies_exactly() {
    let new_policy = VariablePolicyEntry::new(
      TEST_POLICY_MIN_SIZE_NULL, TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL, TEST_POLICY_ATTRIBUTES_NULL,
      LockPolicyType::NoLock, TEST_VAR_GUID_1, None
      );

    // Make sure that passing None is an exact match.
    assert_eq!(new_policy.eval_match(None, &TEST_VAR_GUID_1), Some(VariablePolicyEntry::MATCH_PRIORITY_EXACT));
  }

  #[test]
  fn match_priorities_should_follow_rules() {
    let check_var1_name = "Wildcard1VarName12";
    let match_var1_name = "Wildcard1VarName12";
    let match_var2_name = "Wildcard#VarName12";
    let match_var3_name = "Wildcard#VarName#2";
    let match_var4_name = "Wildcard#VarName##";

    let mut new_policy = VariablePolicyEntry::new(
      TEST_POLICY_MIN_SIZE_NULL, TEST_POLICY_MAX_SIZE_NULL,
      TEST_POLICY_ATTRIBUTES_NULL, TEST_POLICY_ATTRIBUTES_NULL,
      LockPolicyType::NoLock, TEST_VAR_GUID_1, Some(String::from(match_var1_name))
      );

    // Check with a perfect match.
    assert_eq!(new_policy.eval_match(Some(&String::from(check_var1_name)), &TEST_VAR_GUID_1),
                Some(VariablePolicyEntry::MATCH_PRIORITY_EXACT));

    // Check with progressively lower priority matches.
    new_policy.name = Some(String::from(match_var2_name));
    assert_eq!(new_policy.eval_match(Some(&String::from(check_var1_name)), &TEST_VAR_GUID_1), Some(1));
    new_policy.name = Some(String::from(match_var3_name));
    assert_eq!(new_policy.eval_match(Some(&String::from(check_var1_name)), &TEST_VAR_GUID_1), Some(2));
    new_policy.name = Some(String::from(match_var4_name));
    assert_eq!(new_policy.eval_match(Some(&String::from(check_var1_name)), &TEST_VAR_GUID_1), Some(3));

    // Check against the entire namespace.
    new_policy.name = None;
    assert_eq!(new_policy.eval_match(Some(&String::from(check_var1_name)), &TEST_VAR_GUID_1),
                Some(VariablePolicyEntry::MATCH_PRIORITY_MIN));
  }

  #[test]
  fn from_raw_should_reject_null_pointers() {
    let null_pointer = 0 as *const RawVariablePolicyEntry;
    assert!(VariablePolicyEntry::from_raw(null_pointer).is_err());
  }

  #[test]
  fn from_raw_should_reject_bad_revisions() {
    let mut validation_policy: RawVariablePolicyEntry = RawVariablePolicyEntry {
      version: VariablePolicyEntry::RAW_ENTRY_REVISION,
      size: RAW_VARIABLE_POLICY_ENTRY_SIZE as u16,
      offset_to_name: RAW_VARIABLE_POLICY_ENTRY_SIZE as u16,
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: RawLockPolicyType::NO_LOCK,
      padding_3: [0x00, 0x00, 0x00],
    };

    // Update the version to a known-bad version.
    validation_policy.version = 0xffff_ffff;

    let struct_pointer: *const RawVariablePolicyEntry = &validation_policy as *const RawVariablePolicyEntry;
    assert!(VariablePolicyEntry::from_raw(struct_pointer).is_err());
  }

  #[test]
  fn from_raw_should_allow_namespace_wildcards() {
    let validation_policy: RawVariablePolicyEntry = RawVariablePolicyEntry {
      version: VariablePolicyEntry::RAW_ENTRY_REVISION,
      size: RAW_VARIABLE_POLICY_ENTRY_SIZE as u16,
      offset_to_name: RAW_VARIABLE_POLICY_ENTRY_SIZE as u16,
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: RawLockPolicyType::NO_LOCK,
      padding_3: [0x00, 0x00, 0x00],
    };

    let struct_pointer: *const RawVariablePolicyEntry = &validation_policy as *const RawVariablePolicyEntry;
    if let Ok(new_policy) = VariablePolicyEntry::from_raw(struct_pointer) {
      assert_eq!(new_policy.namespace, TEST_VAR_GUID_1);
      assert!(new_policy.name.is_none());
    }
    else {
      assert!(false, "VariablePolicyEntry::from_raw() returned Err()!");
    }
  }

  #[test]
  fn from_raw_should_allow_state_vars_for_namespaces() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 0xAA,
      }),
      name: None,
    });

    if let Ok(new_policy) = VariablePolicyEntry::from_raw(policy_buffer.as_ptr() as *const RawVariablePolicyEntry) {
      assert_eq!(new_policy.namespace, TEST_VAR_GUID_1);

      // Check for the VarState.
      // assert_eq!(new_policy.lock_policy_type, LockPolicyType::LockOnVarState());
      if let LockPolicyType::LockOnVarState(var_state) = new_policy.lock_policy_type {
        assert_eq!(var_state.name, TEST_VAR_NAME_2);
        assert_eq!(var_state.namespace, TEST_VAR_GUID_2);
        assert_eq!(var_state.value, 0xAA);
      }
      else {
        assert!(false, "Missing LockOnVarState!");
      }

      assert!(new_policy.name.is_none());
    }
    else {
      assert!(false, "VariablePolicyEntry::from_raw() returned Err()!");
    }
  }

  #[test]
  fn from_raw_should_reject_bad_sizes() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::NoLock,
      name: Some(String::from(TEST_VAR_NAME_1)),
    });

    // Set a bad size.
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;
    unsafe { (*raw_var_policy_header).size = (*raw_var_policy_header).size - 2; }

    // Check to make sure that it's rejected.
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
  }

  #[test]
  fn from_raw_should_reject_bad_offsets() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 1,
      }),
      name: Some(String::from(TEST_VAR_NAME_1)),
    });

    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;

    // Check for an offset outside the size bounds.
    unsafe { (*raw_var_policy_header).offset_to_name = (*raw_var_policy_header).size + 1; }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());

    // Check for an offset inside the policy header.
    unsafe { (*raw_var_policy_header).offset_to_name = (RAW_VARIABLE_POLICY_ENTRY_SIZE - 2) as u16; }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());

    // Check for an offset inside the state policy header.
    unsafe { (*raw_var_policy_header).offset_to_name = (RAW_VARIABLE_POLICY_ENTRY_SIZE + 2) as u16; }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());

    // Check for a ridiculous offset.
    unsafe { (*raw_var_policy_header).offset_to_name = 0xFFFF; }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
  }

  #[test]
  fn from_raw_should_reject_missing_state_strings() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 1,
      }),
      name: Some(String::from(TEST_VAR_NAME_1)),
    });
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;

    // Replace the regular name string with the state name string and make the state name string disappear.
    let offset_to_end_of_state_name = RAW_VARIABLE_POLICY_ENTRY_SIZE +
                                        RAW_VAR_STATE_LOCK_SIZE +
                                        (TEST_VAR_NAME_2.len() + 1)*mem::size_of::<efi::Char16>();
    unsafe {
      (*raw_var_policy_header).size           = offset_to_end_of_state_name as u16;
      (*raw_var_policy_header).offset_to_name = (RAW_VARIABLE_POLICY_ENTRY_SIZE + RAW_VAR_STATE_LOCK_SIZE) as u16;
    }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());

    // Also test with "" string.
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(""),
        value: 1,
      }),
      name: Some(String::from(TEST_VAR_NAME_1)),
    });
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
  }

  #[test]
  fn from_raw_should_reject_strings_missing_null() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 1,
      }),
      name: Some(String::from(TEST_VAR_NAME_1)),
    });
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;

    // Removing the NULL from the Name should fail.
    unsafe { (*raw_var_policy_header).size -= mem::size_of::<u16>() as u16; }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());

    // Removing the NULL from the State Name is a little trickier.
    // Copy the Name up one character.
    unsafe {
      (*raw_var_policy_header).offset_to_name -= mem::size_of::<u16>() as u16;
      let dest_buffer = (raw_var_policy_header as *const u8)
                          .offset((*raw_var_policy_header).offset_to_name as isize)
                          as *mut u16;
      let source_buffer = dest_buffer.offset(1) as *const u16;
      for i in 0 ..= TEST_VAR_NAME_1.len() {
        *(dest_buffer.offset(i as isize)) = *(source_buffer.offset(i as isize));
      }
    }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
  }

  #[test]
  fn from_raw_should_reject_malformed_strings() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 1,
      }),
      name: Some(String::from(TEST_VAR_NAME_1)),
    });
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;

    // Bisecting the NULL from the Name should fail.
    unsafe { (*raw_var_policy_header).size -= 1; }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());

    // Bisecting the NULL from the State Name is a little trickier.
    // Copy the Name up one byte.
    unsafe {
      (*raw_var_policy_header).offset_to_name -= 1;
      let dest_buffer = (raw_var_policy_header as *const u8)
                          .offset((*raw_var_policy_header).offset_to_name as isize)
                          as *mut u16;
      let source_buffer = dest_buffer.offset(1) as *const u16;
      for i in 0 ..= TEST_VAR_NAME_1.len() {
        *(dest_buffer.offset(i as isize)) = *(source_buffer.offset(i as isize));
      }
    }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
  }

  #[test]
  fn from_raw_should_reject_unpacked_policies() {
    let mut policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 1,
      }),
      name: Some(String::from(TEST_VAR_NAME_1)),
    });

    // Increase the size and move the Name out a bit.
    policy_buffer.reserve(mem::size_of::<u16>());
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;
    unsafe {
      (*raw_var_policy_header).size += mem::size_of::<u16>() as u16;
      (*raw_var_policy_header).offset_to_name += mem::size_of::<u16>() as u16;
      let dest_buffer = (raw_var_policy_header as *const u8)
                          .offset((*raw_var_policy_header).offset_to_name as isize)
                          as *mut u16;
      string_to_char16(&String::from(TEST_VAR_NAME_1), dest_buffer);
    }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());


    // Reintialize without the state policy and try the same test.
    let mut policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::NoLock,
      name: Some(String::from(TEST_VAR_NAME_1)),
    });
    policy_buffer.reserve(mem::size_of::<u16>());
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;
    unsafe {
      (*raw_var_policy_header).size += mem::size_of::<u16>() as u16;
      (*raw_var_policy_header).offset_to_name += mem::size_of::<u16>() as u16;
      let dest_buffer = (raw_var_policy_header as *const u8)
                          .offset((*raw_var_policy_header).offset_to_name as isize)
                          as *mut u16;
      string_to_char16(&String::from(TEST_VAR_NAME_1), dest_buffer);
    }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
  }

  #[test]
  fn from_raw_should_reject_invalid_name_characters() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 1,
      }),
      name: Some(String::from(TEST_VAR_NAME_1)),
    });
    let _raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;

    // Currently, there are no known invalid characters.
    // '#' in LockPolicy->Name are taken as literal.
    // TODO: Determine whether any characters should be considered invalid.
  }

  #[test]
  fn from_raw_should_reject_bad_policy_constraints() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 1,
      }),
      name: Some(String::from(TEST_VAR_NAME_1)),
    });
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;

    // Make sure that invalid MAXes are rejected.
    unsafe { (*raw_var_policy_header).max_size = 0; }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
  }

  #[test]
  fn from_raw_should_reject_unknown_lock_policies() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 1,
      }),
      name: Some(String::from(TEST_VAR_NAME_1)),
    });
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;

    unsafe { (*raw_var_policy_header).lock_policy_type += 1; }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
    unsafe { (*raw_var_policy_header).lock_policy_type += 1; }
    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
  }

  #[test]
  fn from_raw_should_reject_polices_with_too_many_wildcards() {
    let policy_buffer = variable_policy_entry_to_vec_u8(&VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::NoLock,
      name: Some(String::from(TEST_300_HASHES_STRING)),
    });
    let raw_var_policy_header = policy_buffer.as_ptr() as *mut RawVariablePolicyEntry;

    assert!(VariablePolicyEntry::from_raw(raw_var_policy_header as *const RawVariablePolicyEntry).is_err());
  }

  #[test]
  fn add_policy_should_reject_duplicate_policies() {
    let new_policy_1 = VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::NoLock,
      name: Some(String::from(TEST_VAR_NAME_1)),
    };

    let new_policy_2 = VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: 0xFFFF_0000,
      max_size: 0xFFFF_3232,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::NoLock,
      name: Some(String::from(TEST_VAR_NAME_1)),
    };

    let new_policy_3 = VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::NoLock,
      name: None,
    };

    let new_policy_4 = VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: 0xFFFF_0000,
      max_size: 0xFFFF_3232,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::NoLock,
      name: None,
    };

    let mut policy_list = VariablePolicyList::new();

    // Add make sure that identical names are rejected.
    assert_eq!(policy_list.add_policy(new_policy_1), true);
    assert_eq!(policy_list.add_policy(new_policy_2), false);

    // Make sure that wildcards are also rejected.
    assert_eq!(policy_list.add_policy(new_policy_3), true);
    assert_eq!(policy_list.add_policy(new_policy_4), false);
  }

  #[test]
  fn min_and_max_size_policies_should_be_honored() {
    let new_policy = VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      min_size: TEST_POLICY_MIN_SIZE_10,
      max_size: TEST_POLICY_MAX_SIZE_200,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::NoLock,
      name: Some(String::from(TEST_VAR_NAME_1)),
    };

    let mut policy_list = VariablePolicyList::new();

    // Without a policy, there should be no constraints on variable creation.
    assert!(policy_list.is_set_variable_valid(
      &String::from(TEST_VAR_NAME_1), &TEST_VAR_GUID_1,
      r_efi::system::VARIABLE_NON_VOLATILE | r_efi::system::VARIABLE_BOOTSERVICE_ACCESS,
      &[ 0xAA; (TEST_POLICY_MAX_SIZE_200 + 1) as usize ]
      ));

    // Set a policy to test against.
    assert!(policy_list.add_policy(new_policy));

    // With a policy, make sure that sizes outsize the target range fail.
    assert!(!policy_list.is_set_variable_valid(
      &String::from(TEST_VAR_NAME_1), &TEST_VAR_GUID_1,
      r_efi::system::VARIABLE_NON_VOLATILE | r_efi::system::VARIABLE_BOOTSERVICE_ACCESS,
      &[ 0xAA; (TEST_POLICY_MAX_SIZE_200 + 1) as usize ]
      ));

    // With a policy, make sure that sizes outsize the target range fail.
    assert!(!policy_list.is_set_variable_valid(
      &String::from(TEST_VAR_NAME_1), &TEST_VAR_GUID_1,
      r_efi::system::VARIABLE_NON_VOLATILE | r_efi::system::VARIABLE_BOOTSERVICE_ACCESS,
      &[ 0xAA; (TEST_POLICY_MIN_SIZE_10 - 1) as usize ]
      ));

    // With a policy, make sure a valid variable is still valid.
    assert!(policy_list.is_set_variable_valid(
      &String::from(TEST_VAR_NAME_1), &TEST_VAR_GUID_1,
      r_efi::system::VARIABLE_NON_VOLATILE | r_efi::system::VARIABLE_BOOTSERVICE_ACCESS,
      &[ 0xAA; (TEST_POLICY_MIN_SIZE_10 + 1) as usize ]
      ));
  }

  #[test]
  fn to_raw_and_test_encoding_should_be_identical() {
    let new_policy = VariablePolicyEntry {
      namespace: TEST_VAR_GUID_1,
      name: Some(String::from(TEST_VAR_NAME_1)),
      min_size: TEST_POLICY_MIN_SIZE_NULL,
      max_size: TEST_POLICY_MAX_SIZE_NULL,
      attr_cant_have: TEST_POLICY_ATTRIBUTES_NULL,
      attr_must_have: TEST_POLICY_ATTRIBUTES_NULL,
      lock_policy_type: LockPolicyType::LockOnVarState(VarStateLock {
        namespace: TEST_VAR_GUID_2,
        name: String::from(TEST_VAR_NAME_2),
        value: 1,
      }),
    };

    assert_eq!(new_policy.to_raw(), variable_policy_entry_to_vec_u8(&new_policy));
  }
}
