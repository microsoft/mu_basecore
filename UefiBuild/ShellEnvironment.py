## @file ShellEnvironment.py
# This module contains code that helps to manage the build environment
# including PATH, PYTHONPATH, and ENV variables.
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
import sys
import logging
import VarDict


# NOTE: It's entirely possible that this should be a singleton.
#       See here for more ideas...
#       https://stackoverflow.com/questions/6760685/creating-a-singleton-in-python
class ShellEnvironment(object):
  def __init__(self):
    super(ShellEnvironment, self).__init__()

    self.global_path = None
    self.global_pypath = None
    self.build_var_dict = GetBuildVars()

    self.import_environment()

    self.original_path = self.global_path
    self.original_pypath = self.global_pypath

  def set_path(self, path_elements):
    logging.debug("Overriding PATH with new value.")
    self.global_path = list(path_elements)
    os.environ["PATH"] = ";".join(path_elements)
    self.export_environment()

  def set_pypath(self, path_elements):
    logging.debug("Overriding PYTHONPATH with new value.")
    self.global_pypath = list(path_elements)
    self.export_environment()

  def append_path(self, path_element):
    logging.debug("Appending PATH element '%s'." % path_element)
    if path_element not in self.global_path:
      self.global_path.append(path_element)
      self.export_environment()

  def append_pypath(self, path_element):
    logging.debug("Appending PYTHONPATH element '%s'." % path_element)
    if path_element not in self.global_pypath:
      self.global_pypath.append(path_element)
      self.export_environment()

  def insert_path(self, path_element):
    logging.debug("Inserting PATH element '%s'." % path_element)
    if path_element not in self.global_path:
      self.global_path.insert(0, path_element)
      self.export_environment()

  def insert_pypath(self, path_element):
    logging.debug("Inserting PYTHONPATH element '%s'." % path_element)
    if path_element not in self.global_path:
      self.global_pypath.insert(0, path_element)
      self.export_environment()

  def get_build_var(self, var_name):
    return self.build_var_dict.GetValue(var_name)

  def set_build_var(self, var_name, var_data):
    logging.debug("Updating BUILD VAR element '%s': '%s'." % (var_name, var_data))
    self.build_var_dict.SetValue(var_name, var_data, '', overridable=True)

  def get_shell_var(self, var_name):
    return os.environ.get(var_name, None)

  def set_shell_var(self, var_name, var_data):
    logging.debug("Updating SHELL VAR element '%s': '%s'." % (var_name, var_data))
    os.environ[var_name] = var_data

  def import_environment(self):
    # Record the PATH elements of the current environment.
    path = os.environ.get('PATH', "")
    # Filter removes empty elements.
    # List creates an actual list rather than a generator.
    self.global_path = list(filter(None, path.split(';')))  # Filter removes empty strings.

    # Record the PYTHONPATH elements of the current environment.
    # When reading PYTHONPATH, try reading the live path from sys.
    self.global_pypath = sys.path

  def export_environment(self):
    os.environ["PATH"] = ";".join(self.global_path)
    os.environ["PYTHONPATH"] = ";".join(self.global_pypath)
    sys.path = self.global_pypath

  def log_environment(self):
    logging.debug("FINAL PATH:")
    logging.debug(", ".join(self.global_path))

    logging.debug("FINAL PYTHONPATH:")
    logging.debug(", ".join(self.global_pypath))


rootEnvironment = None
def GetEnvironment():
  global rootEnvironment

  if not rootEnvironment:
    rootEnvironment = ShellEnvironment()
  return rootEnvironment


rootVarDict = None
def GetBuildVars():
  global rootVarDict

  if not rootVarDict:
    rootVarDict = VarDict.VarDict()
  return rootVarDict
