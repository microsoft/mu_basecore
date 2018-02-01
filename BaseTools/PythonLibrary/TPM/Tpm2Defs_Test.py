## @file Tpm2Defs_Test.py
# This file contains utility classes to help interpret definitions from the
# Tpm20.h header file in TianoCore.
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
import Tpm2Defs as t2d

class TestCommandCode(unittest.TestCase):

  def test_get_code_returns_codes(self):
    self.assertEqual(t2d.CommandCode.get_code('TPM_CC_Clear'), 0x00000126)
    self.assertEqual(t2d.CommandCode.get_code('TPM_CC_ActivateCredential'), 0x00000147)

  def test_get_code_returns_none_if_not_found(self):
    self.assertEqual(t2d.CommandCode.get_code('I_AM_NOT_A_VALID_CODE'), None)
    self.assertEqual(t2d.CommandCode.get_code(None), None)

  def test_get_string_returns_strings(self):
    self.assertEqual(t2d.CommandCode.get_string(0x00000126), 'TPM_CC_Clear')
    self.assertEqual(t2d.CommandCode.get_string(0x00000147), 'TPM_CC_ActivateCredential')

  def test_get_string_returns_none_if_not_found(self):
    self.assertEqual(t2d.CommandCode.get_string(0xFFFFFFFF), None)
    

if __name__ == '__main__':
  unittest.main()
