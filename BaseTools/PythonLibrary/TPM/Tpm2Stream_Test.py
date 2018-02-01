## @file Tpm2Stream_Test.py
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


import unittest
import struct
import Tpm2Stream
import Tpm2Defs


class Tpm2StreamElement(unittest.TestCase):

  def test_object_has_zero_size_by_default(self):
    so = Tpm2Stream.Tpm2StreamElement()
    self.assertEqual(so.get_size(), 0)


class Tpm2CommandHeader(unittest.TestCase):

  def test_ch_marshals_correctly(self):
    ch1 = Tpm2Stream.TPM2_COMMAND_HEADER(0x4321, 0x00000000, 0xDEADBEEF)
    ch2 = Tpm2Stream.TPM2_COMMAND_HEADER(0x8001, 0x0000000A, Tpm2Defs.TPM_CC_Clear)

    self.assertEqual(ch1.marshal(), bytearray.fromhex('432100000000DEADBEEF'))
    self.assertEqual(ch2.marshal(), bytearray.fromhex('80010000000A') + struct.pack(">L", Tpm2Defs.TPM_CC_Clear))

  def test_ch_has_correct_size(self):
    ch1 = Tpm2Stream.TPM2_COMMAND_HEADER(0x4321, 0x00000000, 0xDEADBEEF)
    self.assertEqual(ch1.get_size(), 0x0A)

  def test_ch_size_can_be_updated(self):
    ch1 = Tpm2Stream.TPM2_COMMAND_HEADER(0x4321, 0x00000000, 0xDEADBEEF)
    self.assertEqual(ch1.marshal(), bytearray.fromhex('432100000000DEADBEEF'))
    ch1.update_size(0x1234)
    self.assertEqual(ch1.marshal(), bytearray.fromhex('432100001234DEADBEEF'))


if __name__ == '__main__':
  unittest.main()
