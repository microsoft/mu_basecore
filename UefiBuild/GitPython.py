## @file GitPython.py
# This module contains code that supports simple git operations.  This should 
# not be used as an extensive git lib but as what is needed for CI/CD builds
#
##
# Copyright (c) 2018, Microsoft Corporation
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

# this attemps to replicate some of the interface of gitpython
import os
import logging
import subprocess
from UtilityFunctions import RunCmd

try:
    from io import StringIO
except ImportError:
    from StringIO import StringIO

class GitCommand(object):
    def __init__(self,command=""):
        self.command = command

class Repo(object):

    def __init__(self,path=None, gitCommand=GitCommand(),bare=True):
        self._path = path
        self.active_branch = None
        self.bare = bare
        self.url = None
        self.head = None
        self._update_from_git()
        
    # Updates the .git file
    def _update_from_git(self):
        if os.path.isdir(os.path.join(self._path,".git")):
            self.bare = False
        self.active_branch = self._get_branch()
        self.url = self._get_url()
        self.head = self._get_head()

    def _get_url(self):
        return_buffer = StringIO()
        cmd = "git config --get remote.origin.url"
        RunCmd(cmd, workingdir=self._path,outstream=return_buffer)

        p1 = return_buffer.getvalue().strip()
        return_buffer.close()        
        return p1
    
    def _get_branch(self):        
        return_buffer = StringIO()
        cmd = "git rev-parse --abbrev-ref HEAD"
        RunCmd(cmd, workingdir=self._path,outstream=return_buffer)

        p1 = return_buffer.getvalue().strip()
        return_buffer.close()        
        return p1
    
    def _get_head(self):        
        return_buffer = StringIO()
        cmd = "git rev-parse HEAD"
        RunCmd(cmd, workingdir=self._path,outstream=return_buffer)

        p1 = return_buffer.getvalue().strip()
        return_buffer.close()        
        return p1

    def checkout(self,branch):
        return_buffer = StringIO()
        cmd = "git checkout %s" % branch
        ret = RunCmd(cmd, workingdir=self._path,outstream=return_buffer)

        p1 = return_buffer.getvalue().strip()
        if ret != 0:
            logging.debug(p1)
            return False

        return True

    @classmethod
    def clone_from(self,url, to_path, progress=None, env=None,shallow =False, **kwargs):
        logging.debug("Cloning {0} into {1}".format(url,to_path))
        #make sure we get the commit if 
        # use run command from utilities
        cmd = ""
        if shallow:
            cmd = "git clone --depth 1 --shallow-submodules --recurse-submodules %s %s " % (url, to_path)
        else:
            cmd = "git clone --recurse-submodules %s %s " % (url, to_path)
        RunCmd(cmd)

        return Repo(to_path)

    
