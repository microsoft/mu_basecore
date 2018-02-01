## @file PiFirmwareFile.py
# Module contains helper classes and functions to work with UEFI FFs.
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

#
# EFI_FFS_FILE_HEADER
#
# typedef struct {
#   EFI_GUID                Name;
#   EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
#   EFI_FV_FILETYPE         Type;
#   EFI_FFS_FILE_ATTRIBUTES Attributes;
#   UINT8                   Size[3];
#   EFI_FFS_FILE_STATE      State;
# } EFI_FFS_FILE_HEADER;
class EfiFirmwareFileSystemHeader(object):
  def __init__(self):
    self.StructString = "=16sHBBBBBB"
    self.FileSystemGuid = None
    self.Size0 = None
    self.Size1 = None
    self.Size2 = None
    self.Attributes = None
    self.Type = None
    self.State = None

  def get_size(self):
    return self.Size0 + (self.Size1 << 8) + (self.Size2 << 16)

  def load_from_file(self, file):
    orig_seek = file.tell()
    struct_bytes = file.read(struct.calcsize(self.StructString))
    file.seek(orig_seek)

    # Load this object with the contents of the data.
    (self.FileSystemGuid, self.Checksum, self.Type, self.Attributes, self.Size0, self.Size1, self.Size2, self.State) = struct.unpack(self.StructString, struct_bytes)

    # Update the GUID to be a UUID object.
    if sys.byteorder == 'big':
      self.FileSystemGuid = uuid.UUID(bytes=self.FileSystemGuid)
    else:
      self.FileSystemGuid = uuid.UUID(bytes_le=self.FileSystemGuid)

    return self

  def serialize(self):
    file_system_guid_bin = self.FileSystemGuid.bytes if sys.byteorder == 'big' else self.FileSystemGuid.bytes_le
    return struct.pack(self.StructString, file_system_guid_bin, self.Checksum, self.Type, self.Attributes, self.Size0, self.Size1, self.Size2, self.State)