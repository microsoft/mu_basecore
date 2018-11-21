# @file Utf8Test.py
# This tool supports checking files for encoding issues.
# file encoding is controlled by the EncodingMap but most
# are set to utf-8
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


import os
import logging
from MuEnvironment.PluginManager import IMuBuildPlugin

##
# map
EcodingMap = {
    ".md": 'utf-8',
    ".dsc": 'utf-8',
    ".dec": 'utf-8',
    ".c": 'utf-8',
    ".h": 'utf-8',
    ".asm": 'utf-8',
    ".masm": 'utf-8',
    ".nasm": 'utf-8',
    ".s": 'utf-8',
    ".inf": 'utf-8',
    ".asl": 'utf-8',
    ".uni": 'utf-8',
    ".py": 'utf-8'
}


class CharEncodingCheck(IMuBuildPlugin):

    def GetTestName(self, packagename, environment):
        return ("MuBuild CharEncodingCheck " + packagename, "MuBuild.CharEncodingCheck." + packagename)

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
        files_tested = 0

        abs_pkg_path = Edk2pathObj.GetAbsolutePathOnThisSytemFromEdk2RelativePath(packagename)

        if abs_pkg_path is None:
            tc.SetSkipped()
            tc.LogStdError("No Package folder {0}".format(abs_pkg_path))
            return 0

        for (ext, enc) in EcodingMap.items():
            files = self.WalkDirectoryForExtension([ext], abs_pkg_path)
            files = [Edk2pathObj.GetEdk2RelativePathFromAbsolutePath(x) for x in files]  # make edk2relative path so can process ignores

            if "IgnoreFiles" in pkgconfig:
                for a in pkgconfig["IgnoreFiles"]:
                    a = a.lower().replace(os.sep, "/")
                    try:
                        tc.LogStdOut("Ignoring File {0}".format(a))
                        files.remove(a)
                    except:
                        tc.LogStdError("CharEncodingCheck.IgnoreInf -> {0} not found in filesystem.  Invalid ignore file".format(a))
                        logging.info("CharEncodingCheck.IgnoreInf -> {0} not found in filesystem.  Invalid ignore file".format(a))

            files = [Edk2pathObj.GetAbsolutePathOnThisSytemFromEdk2RelativePath(x) for x in files]
            for a in files:
                files_tested += 1
                if(self.TestEncodingOk(a, enc)):
                    logging.debug("File {0} Passed Encoding Check {1}".format(a, enc))
                else:
                    tc.LogStdError("Encoding Failure in {0}.  Not {1}".format(a, enc))
                    overall_status += 1

        tc.LogStdOut("Tested Encoding on {0} files".format(files_tested))
        if overall_status is not 0:
            tc.SetFailed("CharEncoding {0} Failed.  Errors {1}".format(packagename, overall_status), "CHAR_ENCODING_CHECK_FAILED")
        else:
            tc.SetSuccess()
        return overall_status

    def TestEncodingOk(self, apath, encodingValue):
        try:
            with open(apath, "rb") as fobj:
                fobj.read().decode(encodingValue)
        except Exception as exp:
            logging.error("Encoding failure: file: {0} type: {1}".format(apath, encodingValue))
            logging.debug("EXCEPTION: while processing {1} - {0}".format(exp, apath))
            return False

        return True

    def ValidateConfig(self, config, name):
        validOptions = ["IgnoreFiles", "skip"]
        for key in config:
            if key not in validOptions:
                raise Exception("Invalid config option {0} in {1}".format(key, name))
