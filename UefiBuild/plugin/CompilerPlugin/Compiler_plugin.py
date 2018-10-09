## @file Compiler_plugin.py
# Simple Project Mu Build Plugin to support
# compiling code
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
### 

import logging
from PluginManager import IMuBuildPlugin
import time
from UefiBuild import UefiBuilder
import os
import sys

class Compiler_plugin(IMuBuildPlugin):


    def GetTestName(self, packagename, environment):
        target = environment.GetValue("TARGET")
        return ("MuBuild Compile " + target + " " + packagename, "MuBuild.CompileCheck." + target + "." + packagename)
    
    def IsTargetDependent(self):
        return True
    ##
    # External function of plugin.  This function is used to perform the task of the MuBuild Plugin
    # 
    #   - package is the edk2 path to package.  This means workspace/packagepath relative.  
    #   - edk2path object configured with workspace and packages path
    #   - any additional command line args
    #   - RepoConfig Object (dict) for the build
    #   - PkgConfig Object (dict)
    #   - EnvConfig Object 
    #   - Plugin Manager Instance
    #   - Plugin Helper Obj Instance
    #   - testcase Object used for outputing junit results
    def RunBuildPlugin(self, packagename, Edk2pathObj, args, repoconfig, pkgconfig, environment, PLM, PLMHelper, tc):
        self._env = environment
        AP = Edk2pathObj.GetAbsolutePathOnThisSytemFromEdk2RelativePath(packagename)
        APDSC = self.get_dsc_name_in_dir(AP)
        AP_Path= Edk2pathObj.GetEdk2RelativePathFromAbsolutePath(APDSC)

        logging.info("Building {0}".format(AP_Path))
        if AP is None or AP_Path is None or not os.path.isfile(APDSC):
            tc.SetSkipped()
            tc.LogStdError("1 warning(s) in {0} Compile. DSC not found.".format(packagename))
            return 0

        self._env.SetValue("ACTIVE_PLATFORM", AP_Path, "Set in Compiler Plugin") 
        #WorkSpace, PackagesPath, PInManager, PInHelper, args, BuildConfigFile=None):
        uefiBuilder = UefiBuilder(Edk2pathObj.WorkspacePath, os.pathsep.join(Edk2pathObj.PackagePathList), PLM, PLMHelper, args)
        #do all the steps
        ret = uefiBuilder.Go()
        if ret != 0: #failure:     
            tc.SetFailed("Compile failed for {0}".format(packagename), "Compile_FAILED")
            tc.LogStdError("{0} Compile failed with error code {1}".format(AP_Path, ret))
            return 1

        else:
            tc.SetSuccess()
            return 0
