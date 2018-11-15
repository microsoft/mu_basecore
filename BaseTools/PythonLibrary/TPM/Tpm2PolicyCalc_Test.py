## @file Tpm2PolicyCalc_Test.py
# This file contains classes used to calculate TPM 2.0 policies
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
from Tpm2PolicyCalc import *


class TestPolicyLocality(unittest.TestCase):

  def test_create_with_empty_list(self):
    policy = PolicyLocality(None)
    self.assertEqual(policy.get_bitfield(), 0)
    policy2 = PolicyLocality(())
    self.assertEqual(policy2.get_bitfield(), 0)

  def test_create_with_base_localities(self):
    policy = PolicyLocality([0, 2, 4])
    self.assertEqual(policy.get_bitfield(), 0b00010101)
    policy2 = PolicyLocality([1, 2])
    self.assertEqual(policy2.get_bitfield(), 0b00000110)
    policy3 = PolicyLocality([3])
    self.assertEqual(policy3.get_bitfield(), 0b00001000)
    policy4 = PolicyLocality([57])
    self.assertEqual(policy4.get_bitfield(), 57)

  def test_create_with_invalid_localites(self):
    with self.assertRaises(ValueError):
      policy = PolicyLocality([5])
    with self.assertRaises(ValueError):
      policy = PolicyLocality([12])
    with self.assertRaises(ValueError):
      policy = PolicyLocality([31])
    with self.assertRaises(ValueError):
      policy = PolicyLocality([256])

  def test_create_with_mixed_lower_and_upper(self):
    with self.assertRaises(ValueError):
      policy = PolicyLocality([1, 4, 35])
    with self.assertRaises(ValueError):
      policy = PolicyLocality([36, 128])

  def test_get_buffer(self):
    self.assertEqual(PolicyLocality([0, 2, 4]).get_buffer_for_digest(), bytearray.fromhex("0000016F" + "15"))
    self.assertEqual(PolicyLocality([34]).get_buffer_for_digest(), bytearray.fromhex("0000016F" + "22"))


class TestPolicyCommandCode(unittest.TestCase):

  def test_create_with_no_code(self):
    with self.assertRaises(ValueError):
      PolicyCommandCode(None)

  def test_create_with_invalid_code(self):
    with self.assertRaises(ValueError):
      PolicyCommandCode("MonkeyValue")
    with self.assertRaises(ValueError):
      PolicyCommandCode(12)
    with self.assertRaises(ValueError):
      PolicyCommandCode({})

  def test_create_with_valid_codes(self):
    policy = PolicyCommandCode('TPM_CC_Clear')
    self.assertEqual(policy.get_code(), 'TPM_CC_Clear')
    policy = PolicyCommandCode('TPM_CC_ClearControl')
    self.assertEqual(policy.get_code(), 'TPM_CC_ClearControl')
    policy = PolicyCommandCode('TPM_CC_Quote')
    self.assertEqual(policy.get_code(), 'TPM_CC_Quote')

  def test_get_buffer(self):
    self.assertEqual(PolicyCommandCode('TPM_CC_Clear').get_buffer_for_digest(), bytearray.fromhex("0000016C" + "00000126"))
    self.assertEqual(PolicyCommandCode('TPM_CC_ClearControl').get_buffer_for_digest(), bytearray.fromhex("0000016C" + "00000127"))


class TestPolicyTreeSolo(unittest.TestCase):

  def test_policy_command_code(self):
    expected_result_1 = bytearray.fromhex("940CFB4217BB1EDCF7FB41937CA974AA68E698AB78B8124B070113E211FD46FC")
    expected_result_2 = bytearray.fromhex("C4DFABCEDA8DE836C95661952892B1DEF7203AFB46FEFEC43FFCFC93BE540730")
    expected_result_3 = bytearray.fromhex("1D2DC485E177DDD0A40A344913CEEB420CAA093C42587D2E1B132B157CCB5DB0")

    test1 = PolicyTreeSolo(PolicyCommandCode("TPM_CC_ClearControl"))
    test2 = PolicyTreeSolo(PolicyCommandCode("TPM_CC_Clear"))
    test3 = PolicyTreeSolo(PolicyCommandCode("TPM_CC_NV_UndefineSpaceSpecial"))

    phash = PolicyHasher('sha256')
    self.assertEqual(test1.get_policy(phash), expected_result_1)
    self.assertEqual(test2.get_policy(phash), expected_result_2)
    self.assertEqual(test3.get_policy(phash), expected_result_3)

  def test_policy_locality(self):
    expected_result = bytearray.fromhex("07039B45BAF2CC169B0D84AF7C53FD1622B033DF0A5DCDA66360AA99E54947CD")

    test = PolicyTreeSolo(PolicyLocality([3,4]))

    phash = PolicyHasher('sha256')
    self.assertEqual(test.get_policy(phash), expected_result)


class TestPolicyTreeAnd(unittest.TestCase):

  def test_single_and_should_match_solo(self):
    soloTest = PolicyTreeSolo(PolicyCommandCode("TPM_CC_Clear"))
    andTest = PolicyTreeAnd([PolicyCommandCode("TPM_CC_Clear")])

    phash = PolicyHasher('sha256')
    self.assertEqual(soloTest.get_policy(phash), andTest.get_policy(phash))


class TestPolicyTreeOr(unittest.TestCase):

  def test_single_and_should_match_solo(self):
    expected_result = bytearray.fromhex("3F44FB41486D4A36A8ADCA2203E73A5068BFED5FDCE5092B9A3C6CCE8ABF3B0C")

    test1 = PolicyTreeSolo(PolicyCommandCode("TPM_CC_ClearControl"))
    test2 = PolicyTreeSolo(PolicyCommandCode("TPM_CC_Clear"))
    orTest = PolicyTreeOr([test1, test2])

    phash = PolicyHasher('sha256')
    self.assertEqual(orTest.get_policy(phash), expected_result)


class TestPolicyTree(object):

  def test_complex_policy_1(self):
    expected_result = bytearray.fromhex("DFFDB6C8EAFCBE691E358882B18703121EAB40DE2386F7A8E7B4A06591E1F0EE")

    # Computation details:
    #   A = TPM2_PolicyLocality(3 & 4)
    #   B = TPM2_PolicyCommandCode(TPM_CC_NV_UndefineSpaceSpecial)
    #   C = TPM2_PolicyCommandCode(TPM_CC_NV_Write)
    #   policy = {{A} AND {C}} OR {{A} AND {B}}

    a = PolicyLocality([3, 4])
    b = PolicyCommandCode('TPM_CC_NV_UndefineSpaceSpecial')
    c = PolicyCommandCode('TPM_CC_NV_Write')

    leg1 = PolicyTreeAnd([a, c])
    leg2 = PolicyTreeAnd([a, b])
    final = PolicyTreeOr([leg1, leg2])

    phash = PolicyHasher('sha256')
    self.assertEqual(final.get_policy(phash), expected_result)


if __name__ == '__main__':
  unittest.main()
