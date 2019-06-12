# @file Compiler_plugin.py
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
from MuPythonLibrary.Uefi.EdkII.Parsers.DscParser import DscParser
from MuEnvironment.PluginManager import IMuBuildPlugin
from MuEnvironment import MuLogging
from MuEnvironment.UefiBuild import UefiBuilder
import os
import re


class CompilerPlugin(IMuBuildPlugin):

    # gets the tests name
    def GetTestName(self, packagename, environment):
        target = environment.GetValue("TARGET")
        return ("MuBuild Compile " + target + " " + packagename, "MuBuild.CompileCheck." + target + "." + packagename)

    def IsTargetDependent(self):
        return True

    # External function of plugin.  This function is used to perform the task of the MuBuild Plugin
    #   - package is the edk2 path to package.  This means workspace/packagepath relative.
    #   - edk2path object configured with workspace and packages path
    #   - any additional command line args
    #   - RepoConfig Object (dict) for the build
    #   - PkgConfig Object (dict)
    #   - EnvConfig Object
    #   - Plugin Manager Instance
    #   - Plugin Helper Obj Instance
    #   - testcase Object used for outputing junit results
    #   - output_stream the StringIO output stream from this plugin
    def RunBuildPlugin(self, packagename, Edk2pathObj, args, repoconfig, pkgconfig, environment, PLM, PLMHelper, tc, output_stream = None):
        self._env = environment
        AP = Edk2pathObj.GetAbsolutePathOnThisSytemFromEdk2RelativePath(packagename)
        APDSC = self.get_dsc_name_in_dir(AP)
        AP_Path = Edk2pathObj.GetEdk2RelativePathFromAbsolutePath(APDSC)

        logging.info("Building {0}".format(AP_Path))
        if AP is None or AP_Path is None or not os.path.isfile(APDSC):
            tc.SetSkipped()
            tc.LogStdError("1 warning(s) in {0} Compile. DSC not found.".format(packagename))
            return 0

        self._env.SetValue("ACTIVE_PLATFORM", AP_Path, "Set in Compiler Plugin")

        # Parse DSC to check for SUPPORTED_ARCHITECTURES
        dp = DscParser()
        dp.SetBaseAbsPath(Edk2pathObj.WorkspacePath)
        dp.SetPackagePaths(Edk2pathObj.PackagePathList)
        dp.ParseFile(AP_Path)
        if "SUPPORTED_ARCHITECTURES" in dp.LocalVars:
            SUPPORTED_ARCHITECTURES = dp.LocalVars["SUPPORTED_ARCHITECTURES"].split('|')
            TARGET_ARCHITECTURES = environment.GetValue("TARGET_ARCH").split(' ')

            # Skip if there is no intersection between SUPPORTED_ARCHITECTURES and TARGET_ARCHITECTURES
            if len(set(SUPPORTED_ARCHITECTURES) & set(TARGET_ARCHITECTURES)) == 0:
                tc.SetSkipped()
                tc.LogStdError("No supported architecutres to build")
                return 0

        # WorkSpace, PackagesPath, PInManager, PInHelper, args, BuildConfigFile=None):
        uefiBuilder = UefiBuilder(Edk2pathObj.WorkspacePath, os.pathsep.join(Edk2pathObj.PackagePathList), PLM, PLMHelper, args)
        # do all the steps
        ret = uefiBuilder.Go()
        if ret != 0:  # failure:
            error_count = ""
            if output_stream is not None:
                # seek to the start of the output stream
                output_stream.seek(0, 0)
                problems = MuLogging.scan_compiler_output(output_stream)
                error_count = " with {} errors/warnings".format(len(problems))
                for level, problem_msg in problems:
                    if level == logging.ERROR:
                        message = "Compile: Error: {0}".format(problem_msg)
                        tc.LogStdError(message)
                        logging.error(message)
                    elif level == logging.WARNING:
                        message = "Compile: Warning: {0}".format(problem_msg)
                        tc.LogStdError(message)
                        logging.warning(message)
                    else:
                        message = "Compiler is unhappy: {0}".format(problem_msg)
                        tc.LogStdError(message)
                        logging.warning(message)
            tc.SetFailed("Compile failed for {0}".format(packagename) + error_count, "Compile_FAILED")
            tc.LogStdError("{0} Compile failed with error code {1} ".format(AP_Path, ret))
            return 1

        else:
            tc.SetSuccess()
            return 0
