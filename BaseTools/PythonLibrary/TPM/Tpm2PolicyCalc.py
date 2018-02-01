## @file Tpm2PolicyCalc.py
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


import Tpm2Defs as t2d
import hashlib
import struct


##========================================================================================
##
## POLICY TREE CLASSES
## These are used to describe a final policy structure.
## You can construct nodes to form complex policies from the policy primitive classes.
##
##                       PolicyTreeOr                                 <--- Tree Node
##                       /          \
##           PolicyTreeSolo         PolicyTreeAnd                     <--- Tree Nodes
##            /                     /        \
##  PolicyCommandCode     PolicyLocality    PolicyCommandCode         <--- Primitives
##
##========================================================================================


class PolicyHasher(object):
  def __init__(self, hash_type):
    if hash_type not in ['sha256', 'sha384']:
      raise ValueError("Invalid hash type '%s'!" % hash_type)

    self.hash_type = hash_type
    self.hash_size = {
      'sha256': 32,
      'sha384': 48
    }[hash_type]

  def get_size(self):
    return self.hash_size

  def hash(self, data):
    hash_obj = None
    if self.hash_type == 'sha256':
      hash_obj = hashlib.sha256()
    else:
      hash_obj = hashlib.sha384()

    hash_obj.update(data)

    return hash_obj.digest()


class PolicyCalculator(object):
  def __init__(self, primitive_dict, policy_tree):
    # For now, we'll leave this pretty sparse.
    # We should have WAY more testing for this stuff.
    self.primitive_dict = primitive_dict
    self.policy_tree = policy_tree

  def generate_digest(self, digest_type):
    pass


class PolicyTreeOr(object):
  def __init__(self, components):
    # OR connections can only be 8 digests long.
    # They CAN, however, be links of ORs.
    if len(components) > 8:
      raise ValueError("OR junctions cannot contain more than 8 sub-policies!")

    self.components = components

  def get_type(self):
    return 'or'

  def validate(self):
    result = True

    for component in self.components:
      # All components must be convertible into a policy.
      if not hasattr(component, 'get_policy'):
        result = False

      # All components must also be valid.
      if not hasattr(component, 'validate') or not component.validate():
        result = False

    return result

  def get_policy_buffer(self, hash_obj):
    concat_policy_buffer = b'\x00'*hash_obj.get_size()
    concat_policy_buffer += struct.pack(">L", t2d.TPM_CC_PolicyOR)
    concat_policy_buffer += b''.join([component.get_policy(hash_obj) for component in self.components])
    return concat_policy_buffer

  def get_policy(self, hash_obj):
    return hash_obj.hash(self.get_policy_buffer(hash_obj))


class PolicyTreeAnd(object):
  def __init__(self, components):
    # ANDs must only be composed of primitives. For simplicity, I guess.
    # Honestly, this has spiralled out of control, but something is better than nothing.
    for component in components:
      if not hasattr(component, 'get_buffer_for_digest'):
        raise ValueError("AND junctions must consist of primitives!")

    self.components = components

  def get_type(self):
    return 'and'

  def validate(self):
    return True

  def get_policy(self, hash_obj):
    current_digest = b'\x00'*hash_obj.get_size()
    for component in self.components:
      current_digest = hash_obj.hash(current_digest + component.get_buffer_for_digest())
    return current_digest


class PolicyTreeSolo(object):
  """This object should only be used to put a single policy claim under an OR"""
  def __init__(self, policy_obj):
    if not hasattr(policy_obj, 'get_buffer_for_digest'):
      raise ValueError("Supplied policy object is missing required functionality!")

    self.policy_obj = policy_obj

  def get_type(self):
    return 'solo'

  def validate(self):
    result = True

  def get_policy_buffer(self, hash_obj):
    return (b'\x00'*hash_obj.get_size()) + self.policy_obj.get_buffer_for_digest()

  def get_policy(self, hash_obj):
    return hash_obj.hash(self.get_policy_buffer(hash_obj))


##========================================================================================
##
## POLICY PRIMITIVES
## These classes are used to describe a single assertion (eg. PolicyLocality) and
## can be used with the PolicyTree classes to construct complex policies.
##
##========================================================================================


class PolicyLocality(object):

  def __init__(self, localities):
    # Update the bitfield with the requested localities.
    if localities is not None:
      self.bitfield = self.calc_bitfield_from_list(localities)
    else:
      self.bitfield = 0b00000000

  def get_bitfield(self):
    return self.bitfield

  def calc_bitfield_from_list(self, localities):
    bitfield = 0b00000000

    # First, we need to validate all of the localities in the list.
    for value in localities:
      # If the value is in a bad range, we're done here.
      if not (0 <= value < 5) and not (32 <= value < 256):
        raise ValueError("Invalid locality '%d'!" % value)
      # An "upper" locality must be individual. Cannot combine with 0-4.
      if (32 <= value < 256) and len(localities) > 1:
        raise ValueError("Cannot combine locality '%d' with others!" % value)

    # If the list is empty... well, we're done.
    if len(localities) == 0:
      pass

    # Now, if we're an "upper" locality, that's a simple value.
    elif len(localities) == 1 and (32 <= localities[0] < 256):
      bitfield = localities[0]

    # We have to actually "think" to calculate the "lower" localities.
    else:
      for value in localities:
        bitfield |= 1 << value

    return bitfield

  def get_buffer_for_digest(self):
    # NOTE: We force big-endian to match the marshalling in the TPM.
    return struct.pack(">LB", t2d.TPM_CC_PolicyLocality, self.bitfield)


class PolicyCommandCode(object):

  def __init__(self, command_code_string=None):
    # Check to make sure that a command_code can be found.
    str_command_code_string = str(command_code_string)
    command_code = t2d.CommandCode.get_code(str_command_code_string)
    if command_code is None:
      raise ValueError("Command code '%s' unknown!" % str_command_code_string)
    self.command_code_string = str_command_code_string

  def get_code(self):
    return self.command_code_string

  def get_buffer_for_digest(self):
    # NOTE: We force big-endian to match the marshalling in the TPM.
    return struct.pack(">LL", t2d.CommandCode.get_code('TPM_CC_PolicyCommandCode'),
                        t2d.CommandCode.get_code(self.command_code_string))
