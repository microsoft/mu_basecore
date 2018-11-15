## @file VariableFormat.py
# Module contains helper classes and functions to work with UEFI Variables.
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

import uuid
import struct
import sys
from Uefi.UefiMultiPhase import *

#
# UEFI GUIDs
#
EfiVariableGuid               = uuid.UUID(fields=(0xDDCF3616, 0x3275, 0x4164, 0x98, 0xB6, 0xFE85707FFE7D))
EfiAuthenticatedVariableGuid  = uuid.UUID(fields=(0xAAF32C78, 0x947B, 0x439A, 0xA1, 0x80, 0x2E144EC37792))

#
# UEFI #Defines
#
HEADER_ALIGNMENT              = 4
VARIABLE_STORE_FORMATTED      = 0x5A
VARIABLE_STORE_HEALTHY        = 0xFE
VARIABLE_DATA                 = 0x55AA
VAR_IN_DELETED_TRANSITION     = 0xFE  ## Variable is in obsolete transition.
VAR_DELETED                   = 0xFD  ## Variable is obsolete.
VAR_HEADER_VALID_ONLY         = 0x7F  ## Variable header has been valid.
VAR_ADDED                     = 0x3F  ## Variable has been completely added.

#
# VARIABLE_STORE_HEADER
# Can parse or produce an VARIABLE_STORE_HEADER structure/byte buffer.
#
# typedef struct {
#   EFI_GUID  Signature;
#   UINT32  Size;
#   UINT8   Format;
#   UINT8   State;
#   UINT16  Reserved;
#   UINT32  Reserved1;
# } VARIABLE_STORE_HEADER;
class VariableStoreHeader(object):
  def __init__(self):
    self.StructString = "=16sLBBHL"
    self.StructSize = struct.calcsize(self.StructString)
    self.Signature = None
    self.Size = None
    self.Format = None
    self.State = None
    self.Reserved0 = None
    self.Reserved1 = None
    self.Type = 'Var'

  def load_from_file(self, file):
    # This function assumes that the file has been seeked
    # to the correct starting location.
    orig_seek = file.tell()
    struct_bytes = file.read(struct.calcsize(self.StructString))
    file.seek(orig_seek)

    # Load this object with the contents of the data.
    (signature_bin, self.Size, self.Format, self.State, self.Reserved0, self.Reserved1) = struct.unpack(self.StructString, struct_bytes)

    # Update the GUID to be a UUID object.
    if sys.byteorder == 'big':
      self.Signature = uuid.UUID(bytes=signature_bin)
    else:
      self.Signature = uuid.UUID(bytes_le=signature_bin)

    # Check one last thing.
    if self.Signature != EfiVariableGuid and self.Signature != EfiAuthenticatedVariableGuid:
      raise Exception("VarStore is of unknown type! %s" % self.Signature)
    if self.Signature == EfiAuthenticatedVariableGuid:
      self.Type = 'AuthVar'

    return self

  def serialize(self):
    signature_bin = self.Signature.bytes if sys.byteorder == 'big' else self.Signature.bytes_le
    return struct.pack(self.StructString, signature_bin, self.Size, self.Format, self.State, self.Reserved0, self.Reserved1)

#
# TODO: VariableHeader and AuthenticatedVariableHeader are not truly
#       header structures. They're entire variables. This code should be
#       cleaned up.
#

#
# VARIABLE_HEADER
# Can parse or produce an VARIABLE_HEADER structure/byte buffer.
#
# typedef struct {
#   UINT16      StartId;
#   UINT8       State;
#   UINT8       Reserved;
#   UINT32      Attributes;
#   UINT32      NameSize;
#   UINT32      DataSize;
#   EFI_GUID    VendorGuid;
# } VARIABLE_HEADER;
class VariableHeader(object):
  def __init__(self):
    self.StructString = "=HBBLLL16s"
    self.StructSize = struct.calcsize(self.StructString)
    self.StartId = VARIABLE_DATA
    self.State = VAR_ADDED
    self.Attributes = (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
    self.NameSize = 0
    self.DataSize = 0
    self.VendorGuid = uuid.uuid4()
    self.Name = None
    self.Data = None

  def populate_structure_fields(self, in_bytes):
    (self.StartId, self.State, reserved, self.Attributes, self.NameSize, self.DataSize, self.VendorGuid) = struct.unpack(self.StructString, in_bytes)

  def load_from_bytes(self, in_bytes):
    # Load this object with the contents of the data.
    self.populate_structure_fields(in_bytes[0:self.StructSize])

    # Update the GUID to be a UUID object.
    if sys.byteorder == 'big':
      self.VendorGuid = uuid.UUID(bytes=self.VendorGuid)
    else:
      self.VendorGuid = uuid.UUID(bytes_le=self.VendorGuid)

    # Before loading data, make sure that this is a valid variable.
    if self.StartId != VARIABLE_DATA:
      file.seek(orig_seek)
      raise EOFError("No variable data!")

    # Finally, load the data.
    data_offset = self.StructSize
    self.Name = in_bytes[data_offset:(data_offset + self.NameSize)].decode('utf-16')[:-1]  # Strip the terminating char.
    data_offset += self.NameSize
    self.Data = in_bytes[data_offset:(data_offset + self.DataSize)]

    return self

  def load_from_file(self, file):
    # This function assumes that the file has been seeked
    # to the correct starting location.
    orig_seek = file.tell()
    struct_bytes = file.read(struct.calcsize(self.StructString))

    # Load this object with the contents of the data.
    self.populate_structure_fields(struct_bytes)

    # Update the GUID to be a UUID object.
    if sys.byteorder == 'big':
      self.VendorGuid = uuid.UUID(bytes=self.VendorGuid)
    else:
      self.VendorGuid = uuid.UUID(bytes_le=self.VendorGuid)

    # Before loading data, make sure that this is a valid variable.
    if self.StartId != VARIABLE_DATA:
      file.seek(orig_seek)
      raise EOFError("No variable data!")

    # Finally, load the data.
    self.Name = file.read(self.NameSize).decode('utf-16')[:-1]  # Strip the terminating char.
    self.Data = file.read(self.DataSize)

    file.seek(orig_seek)
    return self

  def get_buffer_data_size(self):
    return self.StructSize + self.NameSize + self.DataSize

  def get_buffer_padding_size(self):
    buffer_data_size = self.get_buffer_data_size()
    padding_size = 0
    if buffer_data_size % HEADER_ALIGNMENT != 0:
      padding_size += HEADER_ALIGNMENT - (buffer_data_size % HEADER_ALIGNMENT)
    return padding_size

  def get_buffer_size(self):
    return self.get_buffer_data_size() + self.get_buffer_padding_size()

  def get_packed_name(self):
    # Make sure to replace the terminating char.
    # name_bytes = b"\x00".join([char for char in (self.Name + b'\x00')])
    name_bytes = self.Name.encode('utf-16')

    # Python encode will leave an "0xFFFE" on the front
    # to declare the encoding type. UEFI does not use this.
    name_bytes = name_bytes[2:]

    # Python encode skips the terminating character, so let's add that.
    name_bytes += b"\x00\x00"

    return name_bytes

  def set_name(self, new_name):
    self.Name = new_name
    self.NameSize = len(self.get_packed_name())

  def set_data(self, new_data):
    self.Data = new_data
    self.DataSize = len(new_data)

  def pack_struct(self):
    vendor_guid = self.VendorGuid.bytes if sys.byteorder == 'big' else self.VendorGuid.bytes_le
    return struct.pack(self.StructString, self.StartId, self.State, 0, self.Attributes,
                        self.NameSize, self.DataSize, vendor_guid)

  def serialize(self, with_padding=False):
    bytes = self.pack_struct()

    # Now add the name and data.
    bytes += self.get_packed_name()
    bytes += self.Data

    # Add padding if necessary.
    if with_padding:
      bytes += b"\xFF" * self.get_buffer_padding_size()

    return bytes

#
# AUTHENTICATED_VARIABLE_HEADER
# Can parse or produce an AUTHENTICATED_VARIABLE_HEADER structure/byte buffer.
#
# typedef struct {
#   UINT16      StartId;
#   UINT8       State;
#   UINT8       Reserved;
#   UINT32      Attributes;
#   UINT64      MonotonicCount;
#   EFI_TIME    TimeStamp;
#   UINT32      PubKeyIndex;
#   UINT32      NameSize;
#   UINT32      DataSize;
#   EFI_GUID    VendorGuid;
# } AUTHENTICATED_VARIABLE_HEADER;
class AuthenticatedVariableHeader(VariableHeader):
  def __init__(self):
    super(AuthenticatedVariableHeader, self).__init__()
    self.StructString = "=HBBLQ16sLLL16s"
    self.StructSize = struct.calcsize(self.StructString)
    self.MonotonicCount = 0
    self.TimeStamp = b''
    self.PubKeyIndex = 0

  def populate_structure_fields(self, in_bytes):
    (self.StartId, self.State, reserved, self.Attributes, self.MonotonicCount, self.TimeStamp, self.PubKeyIndex,
      self.NameSize, self.DataSize, self.VendorGuid) = struct.unpack(self.StructString, in_bytes)

  def pack_struct(self, with_padding=False):
    vendor_guid = self.VendorGuid.bytes if sys.byteorder == 'big' else self.VendorGuid.bytes_le
    return struct.pack(self.StructString, self.StartId, self.State, 0, self.Attributes, self.MonotonicCount,
                        self.TimeStamp, self.PubKeyIndex, self.NameSize, self.DataSize, vendor_guid)

if __name__ == '__main__':
    pass
