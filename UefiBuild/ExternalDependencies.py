## @file ExternalDependencies.py
# This module contains helper objects that can manipulate,
# retrieve, validate, and clean external dependencies for the
# build envrionment.
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
import logging
from UtilityFunctions import RunCmd
import shutil
import json
import time
from io import StringIO


class ExternalDependency(object):
  def __init__(self, descriptor):
    super(ExternalDependency, self).__init__()

    #
    # Set the data for this object.
    #
    self.scope = descriptor['scope']
    self.type = descriptor['type']
    self.name = descriptor['name']
    self.source = descriptor['source']
    self.version = descriptor['version']
    self.flags = descriptor.get('flags', None)
    self.var_name = descriptor.get('var_name', None)

    self.descriptor_location = os.path.dirname(descriptor['descriptor_file'])
    self.contents_dir = os.path.join(self.descriptor_location, self.name + "_extdep")
    self.state_file_path = os.path.join(self.contents_dir, "extdep_state.json")
    self.published_path = self.contents_dir

    if self.flags and "include_separator" in self.flags:
      self.published_path += os.path.sep

  def _clean_directory(self, dir_path):
    retry = 1
    while True:
      try:
        shutil.rmtree(dir_path)
      except OSError:
        if not retry:
          # If we're out of retries, bail.
          raise
        time.sleep(5)
        retry -= 1
        continue
      break

  def clean(self):
    logging.debug("Cleaning dependency directory for '%s'..." % self.name)
    if os.path.isdir(self.contents_dir):
      self._clean_directory(self.contents_dir)

  def fetch(self):
    # The base class does not implement a fetch.
    logging.critical("Fetch() CALLED ON BASE EXTDEP CLASS!")
    pass

  def verify(self):
    # The base class does not implement a verify.
    logging.critical("Verify() CALLED ON BASE EXTDEP CLASS!")
    return False

  def update_state_file(self):
    with open(self.state_file_path, 'w+') as file:
      json.dump({'version': self.version}, file)


class NugetDependency(ExternalDependency):
  global_cache_path = None

  @staticmethod
  def normalize_version(version):
    version_parts = tuple(int(num) for num in version.split('.'))
    if len(version_parts) > 4:
      raise RuntimeError("Unparsable version '%s'!")

    # Remove extra trailing zeros (beyond 3 elements).
    if len(version_parts) == 4 and version_parts[3] == 0:
      version_parts = version_parts[0:2]

    # Add missing trailing zeros (below 3 elements).
    if len(version_parts) < 3:
      version_parts = version_parts + (0,) * (3 - len(version_parts))

    # Return reformed version.
    return ".".join((str(num) for num in version_parts))

  def _fetch_from_cache(self, package_name):
    result = False

    #
    # We still need to use Nuget to figure out where the
    # "global-packages" cache is on this machine.
    #
    if NugetDependency.global_cache_path is None:
      cmd = ["nuget.exe", "locals", "global-packages", "-list"]
      cmd_string = " ".join(cmd)
      return_buffer = StringIO()
      if (RunCmd(cmd_string, outstream=return_buffer) == 0):
        # Seek to the beginning of the output buffer and capture the output.
        return_buffer.seek(0)
        return_string = return_buffer.read()
        NugetDependency.global_cache_path = return_string.strip().strip("global-packages: ")

    #
    # If the path couldn't be found, we can't do anything else.
    #
    if not os.path.isdir(NugetDependency.global_cache_path):
      logging.info("Could not determine Nuget global packages cache location.")
      return False

    #
    # Now, try to locate our actual cache path
    nuget_version = NugetDependency.normalize_version(self.version)
    cache_search_path = os.path.join(NugetDependency.global_cache_path, package_name.lower(), nuget_version, package_name)
    if os.path.isdir(cache_search_path):
      logging.info("Local Cache found for Nuget package '%s'. Skipping fetch." % package_name)
      shutil.copytree(cache_search_path, self.contents_dir)
      self.update_state_file()
      result = True

    return result

  def fetch(self):
    package_name = self.name

    #
    # Before trying anything with Nuget feeds,
    # check to see whether the package is already in
    # our local cache. If it is, we avoid a lot of
    # time and network cost by copying it directly.
    #
    if self._fetch_from_cache(package_name):
      # We successfully found the package in the cache.
      # Bail.
      return

    #
    # If we are still here, the package wasn't in the cache.
    # We need to ask Nuget to find it.
    #

    #
    # First, fetch the contents of the package.
    #
    temp_directory = self.get_temp_dir()
    cmd = ["nuget.exe", "install", package_name]
    cmd += ["-Source", self.source]
    cmd += ["-ExcludeVersion"]
    cmd += ["-Version", self.version]
    # cmd += ["-DirectDownload", "-NoCache"]  #avoid cache -- not sure this is good
    #cmd += ["-NonInteractive"]
    cmd += ["-Verbosity", "detailed"]
    cmd += ["-OutputDirectory", '"'+temp_directory+'"']
    cmd_string = " ".join(cmd)
    RunCmd(cmd_string)

    #
    # Next, copy the contents of the package to the
    # final resting place.
    #
    # Depending on packaging, the package content will be in one of two
    # possible locations:
    # 1. temp_directory\package_name\package_name\
    # 2. temp_directory\package_name\
    #
    source_dir = os.path.join(temp_directory, package_name, package_name)
    if not os.path.isdir(source_dir):
      source_dir = os.path.join(temp_directory, package_name)
    shutil.move(source_dir, self.contents_dir)

    #
    # Add a file to track the state of the dependency.
    #
    self.update_state_file()

    #
    # Finally, delete the temp directory.
    #
    self._clean_directory(temp_directory)

  def get_temp_dir(self):
    return self.contents_dir + "_temp"

  def clean(self):
    super(NugetDependency, self).clean()
    if os.path.isdir(self.get_temp_dir()):
      self._clean_directory(self.get_temp_dir())

  def verify(self):
    result = True
    state_data = None

    # See whether or not the state file exists.
    if not os.path.isfile(self.state_file_path):
      result = False

    # Attempt to load the state file.
    if result:
      with open(self.state_file_path, 'r') as file:
        try:
          state_data = json.load(file)
        except:
          pass
    if state_data is None:
      result = False

    # If loaded, check the version.
    if result and state_data['version'] != self.version:
      result = False

    logging.debug("Verify '%s' returning '%s'." % (self.name, result))
    return result


def ExtDepFactory(descriptor):
  if descriptor['type'] == 'nuget':
    return NugetDependency(descriptor)
  else:
    raise ValueError("Unknown extdep type '%s' requested!" % descriptor['type'])