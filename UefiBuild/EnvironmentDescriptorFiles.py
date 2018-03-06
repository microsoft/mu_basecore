## @file EnvironmentDescriptorFiles.py
# This module contains code for working with the JSON environment
# descriptor files. It can parse the files, validate them, and return
# objects representing their contents.
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
import os
import json
import sys


class PathEnv(object):
  def __init__(self, descriptor):
    super(PathEnv, self).__init__()

    #
    # Set the data for this object.
    #
    self.scope = descriptor['scope']
    self.flags = descriptor['flags']
    self.var_name = descriptor.get('var_name', None)

    self.descriptor_location = os.path.dirname(descriptor['descriptor_file'])
    self.published_path = self.descriptor_location


class DescriptorFile(object):
  def __init__(self, file_path):
    super(DescriptorFile, self).__init__()

    self.file_path = file_path
    self.descriptor_contents = None

    with open(file_path, 'r') as file:
      try:
        self.descriptor_contents = json.load(file)
      except:
        pass  # We'll pick up this error when looking at the data.

    #
    # Make sure that we loaded the file successfully.
    #
    if self.descriptor_contents is None:
      raise ValueError("Could not load contents of descriptor file '%s'!" % file_path)

    # The file path is an implicit descriptor field.
    self.descriptor_contents['descriptor_file'] = self.file_path

    # All files require a scope.
    if 'scope' not in self.descriptor_contents:
      raise ValueError("File '%s' missing required field '%s'!" % (self.file_path, 'scope'))

    # If a file has flags, make sure they're sane.
    if 'flags' in self.descriptor_contents:
      # If a flag requires a name, make sure a name is provided.
      for name_required in ('set_shell_var', 'set_build_var'):
        if name_required in self.descriptor_contents['flags']:
          if 'var_name' not in self.descriptor_contents:
            raise ValueError("File '%s' has a flag requesting a var, but does not provide 'var_name'!" % self.file_path)
    
    # clean up each string item for more reliable processing
    for (k,v) in self.descriptor_contents.items():
      if(self.is_string(v)):
        self.descriptor_contents[k] = self.sanitize_string(v)
    
  #
  # Check if input is a string type
  #
  def is_string(self, s):
    # if we use Python 3
    if (sys.version_info[0] >= 3):
        return isinstance(s, str)
    # we use Python 2
    return isinstance(s, basestring)

  #
  # Clean up a string "value" in the descriptor file.
  #
  def sanitize_string(self, s):
    # Perform any actions needed to clean the string.
    return s.strip()


class PathEnvDescriptor(DescriptorFile):
  def __init__(self, file_path):
    super(PathEnvDescriptor, self).__init__(file_path)

    #
    # Validate file contents.
    #
    # Make sure that the required fields are present.
    for required_field in ('flags',):
      if required_field not in self.descriptor_contents:
        raise ValueError("File '%s' missing required field '%s'!" % (self.file_path, required_field))


class ExternDepDescriptor(DescriptorFile):
  def __init__(self, file_path):
    super(ExternDepDescriptor, self).__init__(file_path)

    #
    # Validate file contents.
    #
    # Make sure that the required fields are present.
    for required_field in ('scope', 'type', 'name', 'source', 'version'):
      if required_field not in self.descriptor_contents:
        raise ValueError("File '%s' missing required field '%s'!" % (self.file_path, required_field))

class PluginDescriptor(DescriptorFile):
  def __init__(self, file_path):
    super(PluginDescriptor, self).__init__(file_path)

    #
    # Validate file contents.
    #
    # Make sure that the required fields are present.
    for required_field in ('scope', 'name', 'module'):
      if required_field not in self.descriptor_contents:
        raise ValueError("File '%s' missing required field '%s'!" % (self.file_path, required_field))
      
    # Make sure the module item doesn't have .py on the end
    if(self.descriptor_contents["module"].lower().endswith(".py")):
        self.descriptor_contents["module"] = self.descriptor_contents["module"][:-3]  #remove last 3 chars

