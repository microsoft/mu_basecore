## @file Tpm2Stream.py
# This file contains utility classes to help marshal and unmarshal data to/from the TPM.
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


import struct


class Tpm2StreamElement(object):
  def __init__(self):
    self.pack_string = ""

  """This get_size refers to the size of this structure when marshalled"""
  def get_size(self):
    return struct.calcsize(self.pack_string)


class Tpm2StreamPrimitive(Tpm2StreamElement):
  def __init__(self, size, value):
    super(Tpm2StreamPrimitive, self).__init__()

    if size not in (1, 2, 4, 8):
      raise ValueError("Size must be 1, 2, 4, or 8 bytes!")

    self.pack_string = {
      1: ">B",
      2: ">H",
      4: ">L",
      8: ">Q"
    }[size]
    self.value = value

  def marshal(self):
    return struct.pack(self.pack_string, self.value)


class TPM2_COMMAND_HEADER(Tpm2StreamElement):
  def __init__(self, tag, size, code):
    super(TPM2_COMMAND_HEADER, self).__init__()
    self.tag = tag
    self.code = code
    self.size = size
    self.pack_string = ">HLL"

  """This update_size refers to the size of the whole command"""
  def update_size(self, size):
    self.size = size

  def marshal(self):
    return struct.pack(self.pack_string, self.tag, self.size, self.code)


class TPM2B(Tpm2StreamElement):
  def __init__(self, data):
    super(TPM2B, self).__init__()
    self.data = data
    self.size = len(data)
    self.pack_string = ">H%ds" % self.size

  def update_data(self, data):
    self.data = data
    self.size = len(data)
    self.pack_string = ">H%ds" % self.size

  def marshal(self):
    return struct.pack(self.pack_string, self.size, self.data)


class Tpm2CommandStream(object):
  def __init__(self, tag, size, code):
    super(Tpm2CommandStream, self).__init__()
    self.header = TPM2_COMMAND_HEADER(tag, size, code)
    self.stream_size = self.header.get_size()
    self.header.update_size(self.stream_size)
    self.stream_elements = []

  def get_size(self):
    return self.stream_size

  def add_element(self, element):
    self.stream_elements.append(element)
    self.stream_size += element.get_size()
    self.header.update_size(self.stream_size)

  def get_stream(self):
    return self.header.marshal() + b''.join(element.marshal() for element in self.stream_elements)
