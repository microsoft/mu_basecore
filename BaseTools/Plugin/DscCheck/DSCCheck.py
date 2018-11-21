# @file DSCCheck.py
# Simple Project Mu Build Plugin to support
# checking the package DSC to make sure
# all INFs are listed.
#
# Since many of Project Mu tests are based on content
# listed in DSC it is important that all modules are listed.
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
from MuEnvironment.PluginManager import IMuBuildPlugin
import os
from MuPythonLibrary.Uefi.EdkII.Parsers.DscParser import DscParser


class DSCCheck(IMuBuildPlugin):

    def GetTestName(self, packagename, environment):
        return ("MuBuild DscCheck " + packagename, "MuBuild.DscCheck." + packagename)

    #   - package is the edk2 path to package.  This means workspace/packagepath relative.
    #   - edk2path object configured with workspace and packages path
    #   - any additional command line args
    #   - RepoConfig Object (dict) for the build
    #   - PkgConfig Object (dict) for the pkg
    #   - EnvConfig Object
    #   - Plugin Manager Instance
    #   - Plugin Helper Obj Instance
    #   - testcalass Object used for outputing junit results
    #   - output_stream the StringIO output stream from this plugin
    def RunBuildPlugin(self, packagename, Edk2pathObj, args, repoconfig, pkgconfig, environment, PLM, PLMHelper, tc, output_stream = None):
        overall_status = 0

        abs_pkg_path = Edk2pathObj.GetAbsolutePathOnThisSytemFromEdk2RelativePath(packagename)
        abs_dsc_path = self.get_dsc_name_in_dir(abs_pkg_path)
        wsr_dsc_path = Edk2pathObj.GetEdk2RelativePathFromAbsolutePath(abs_dsc_path)

        if abs_dsc_path is None or wsr_dsc_path is "" or not os.path.isfile(abs_dsc_path):
            tc.SetSkipped()
            tc.LogStdError("No DSC file {0} in package {1}".format(abs_dsc_path, abs_pkg_path))
            return 0

        # Get INF Files
        INFFiles = self.WalkDirectoryForExtension([".inf"], abs_pkg_path)
        INFFiles = [Edk2pathObj.GetEdk2RelativePathFromAbsolutePath(x) for x in INFFiles]  # make edk2relative path so can compare with DSC

        # remove ignores

        if "IgnoreInf" in pkgconfig:
            for a in pkgconfig["IgnoreInf"]:
                a = a.replace(os.sep, "/")
                try:
                    tc.LogStdOut("Ignoring INF {0}".format(a))
                    INFFiles.remove(a)
                except:
                    tc.LogStdError("DscCheckConfig.IgnoreInf -> {0} not found in filesystem.  Invalid ignore file".format(a))
                    logging.info("DscCheckConfig.IgnoreInf -> {0} not found in filesystem.  Invalid ignore file".format(a))

        # DSC Parser
        # self.dp = Dsc()
        # TODO: modify the DSCObject to be a replacement for the EDK version?
        # Eventually this will just be a part of the environment we bring up?
        dp = DscParser()
        dp.SetBaseAbsPath(Edk2pathObj.WorkspacePath)
        dp.SetPackagePaths(Edk2pathObj.PackagePathList)
        dp.ParseFile(wsr_dsc_path)

        # lowercase for matching
        # dp.Libs = [x.lower() for x in dp.Libs]
        # dp.ThreeMods = [x.lower() for x in dp.ThreeMods]
        # dp.SixMods = [x.lower() for x in dp.SixMods]
        # dp.OtherMods = [x.lower() for x in dp.OtherMods]

        # Check if INF in component section
        for INF in INFFiles:
            if not any(INF.strip() in x for x in dp.ThreeMods) \
                    and not any(INF.strip() in x for x in dp.SixMods) and not any(INF.strip() in x for x in dp.OtherMods):
                logging.critical(INF + " not in " + wsr_dsc_path)
                tc.LogStdError("{0} not in {1}".format(INF, wsr_dsc_path))
                overall_status = overall_status + 1

        # If XML object esists, add result
        if overall_status is not 0:
            tc.SetFailed("DSCCheck {0} Failed.  Errors {1}".format(wsr_dsc_path, overall_status), "DSCCHECK_FAILED")
        else:
            tc.SetSuccess()
        return overall_status
