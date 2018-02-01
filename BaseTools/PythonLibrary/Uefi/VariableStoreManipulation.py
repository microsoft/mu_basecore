## @file VariableStoreManipulation.py
# Contains classes and helper functions to modify variables in a UEFI ROM image.
#
##
# Copyright (c) 2017, Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

import Uefi.EdkII.PiFirmwareVolume as PiFV
import Uefi.EdkII.VariableFormat as VF

import os
import mmap

class VariableStore(object):
  def __init__(self, romfile, store_base=None, store_size=None):
    self.rom_file_path = romfile
    self.store_base = store_base
    self.store_size = store_size
    self.rom_file = None
    self.rom_file_map = None

    if not os.path.isfile(self.rom_file_path):
      raise Exception("'%s' is not the path to a file!" % self.rom_file_path)

    self.rom_file = open(self.rom_file_path, 'r+b')
    self.rom_file_map = mmap.mmap(self.rom_file.fileno(), 0)

    # Sanity check some things.
    file_size = self.rom_file_map.size()
    if (store_base is not None and store_size is not None and
        (store_base + store_size) > file_size):
      raise Exception("ROM file is %d bytes. Cannot seek to %d+%d bytes!" % (file_size, store_base, store_size))

    # Go ahead and advance the file cursor and load the FV header.
    self.rom_file.seek(self.store_base)
    self.fv_header = PiFV.EfiFirmwareVolumeHeader().load_from_file(self.rom_file)
    if self.fv_header.FileSystemGuid != PiFV.EfiSystemNvDataFvGuid:
      raise Exception("Store_base is not pointing at a valid SystemNvData FV!")
    if self.fv_header.FvLength != self.store_size:
      raise Exception("Store_size %d does not match FV size %d!" % (self.store_size, self.fv_header.FvLength))

    # Advance the file cursor and load the VarStore header.
    self.rom_file.seek(self.fv_header.HeaderLength, os.SEEK_CUR)
    self.var_store_header = VF.VariableStoreHeader().load_from_file(self.rom_file)
    if (self.var_store_header.Format != VF.VARIABLE_STORE_FORMATTED or
        self.var_store_header.State != VF.VARIABLE_STORE_HEALTHY):
      raise Exception("VarStore is invalid or cannot be processed with this helper!")

    # Now we're finally ready to read some variables.
    self.variables = []
    self.rom_file.seek(self.var_store_header.StructSize, os.SEEK_CUR)
    try:
      while True:
        new_var = self.get_new_var_class().load_from_file(self.rom_file)

        # Seek past the current variable in the store.
        self.rom_file.seek(new_var.get_buffer_size(), os.SEEK_CUR)

        # Add the variable to the array.
        self.variables.append(new_var)
    except EOFError:
      pass
    except:
      raise

    # Finally, reset the file cursor to the beginning of the VarStore FV.
    self.rom_file.seek(self.store_base)

  def __del__(self):
    if self.rom_file_map is not None:
      self.rom_file_map.flush()
      self.rom_file_map.close()

    if self.rom_file is not None:
      self.rom_file.close()

  def get_new_var_class(self):
    if self.var_store_header.Type == 'Var':
      new_var = VF.VariableHeader()
    else:
      new_var = VF.AuthenticatedVariableHeader()

    return new_var

  def add_variable(self, new_var):
    self.variables.append(new_var)

  def flush_to_file(self):
    # First, we need to make sure that our variables will fit in the VarStore.
    var_size = sum([var.get_buffer_size() for var in self.variables])
    # Add the terminating var header.
    dummy_var = self.get_new_var_class()
    var_size += dummy_var.StructSize
    if var_size > self.var_store_header.Size:
      raise Exception("Total variable size %d is too large to fit in VarStore %d!" % (var_size, self.var_store_header.Size))

    # Now, we just have to serialize each variable in turn and write them to the mmap buffer.
    var_offset = self.store_base + self.fv_header.HeaderLength + self.var_store_header.StructSize
    for var in self.variables:
      var_buffer_size = var.get_buffer_size()
      self.rom_file_map[var_offset:(var_offset + var_buffer_size)] = var.serialize(True)
      var_offset += var_buffer_size

    # Add a terminating Variable Header.
    self.rom_file_map[var_offset:(var_offset + dummy_var.StructSize)] = b'\xFF' * dummy_var.StructSize

    # Now we have to flush the mmap to the file.
    self.rom_file_map.flush()

if __name__ == '__main__':
    pass
