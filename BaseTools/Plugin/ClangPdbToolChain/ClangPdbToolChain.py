# @file ClangPdbToolChain.py
# Plugin to configures paths for the ClangPdb tool chain
##
# This plugin works in conjuncture with the tools_def
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
from io import StringIO
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toolext.environment import shell_environment
from edk2toolext.environment import version_aggregator
from edk2toollib.utility_functions import GetHostInfo
from edk2toollib.utility_functions import RunCmd


class ClangPdbToolChain(IUefiBuildPlugin):

    def do_post_build(self, thebuilder):
        return 0

    def do_pre_build(self, thebuilder):
        self.Logger = logging.getLogger("ClangPdbToolChain")
        
        ##
        # CLANGPBD
        # - Need to find the clang path.
        # - Report path and version for logging
        # 
        # if CLANG_BIN already set the plugin will confirm it exists and get the version of clang
        # If not set it will look for clang on the path.  If found it will configure for that.
        # if still not found it will try the default install directory.  
        # finally an error will be reported if not found
        ##
        if thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "CLANGPDB":
            
            HostInfo = GetHostInfo()
            ClangBin_Default = "UNDEFINED"
            clang_exe = "clang"

            if HostInfo.os == "Windows":
                ClangBin_Default = "C:\\Program Files\\LLVM\\bin\\\\"  #need to escape the last slash as it seems to be removed
                clang_exe += ".exe"
            elif HostInfo.os == "Linux":
                ClangBin_Default = "/LLVM/bin/"  #this isn't right
            else:
                pass
                # no defaults set

            ClangBin = shell_environment.GetEnvironment().get_shell_var("CLANG_BIN")
            if ClangBin is not None:
                self.Logger.info("CLANG_BIN is already set.")
            else:
                # see if clang is on path.
                for path_entry in os.getenv("PATH").split(os.pathsep):
                    path_entry = os.path.normpath(path_entry)
                    if os.path.isfile(os.path.join(path_entry, clang_exe)):
                        ClangBin = os.path.abspath(path_entry) + os.sep
                        break
                if ClangBin is None:
                    # Didn't find it on path - try the install default.
                    ClangBin = ClangBin_Default

                shell_environment.GetEnvironment().set_shell_var("CLANG_BIN", ClangBin)

            version_aggregator.GetVersionAggregator().ReportVersion(
                    "CLANG BIN", ClangBin, version_aggregator.VersionTypes.INFO)
            
            # now confirm it exists
            if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("CLANG_BIN")):
                self.Logger.error(f"Path for CLANGPDB toolchain is invalid.  {ClangBin}")
                return -2

            version_aggregator.GetVersionAggregator().ReportVersion(
                "CLANG Version", self._get_clang_version(ClangBin), version_aggregator.VersionTypes.TOOL)

        return 0

    ##
    ## Get the clang version to report
    ##
    ## clang --version
    ## clang version 12.0.0
    ##
    def _get_clang_version(self, clang_bin_path):
        return_buffer = StringIO()
        ret = RunCmd(os.path.join(clang_bin_path, "clang"), "--version", outstream=return_buffer)
        if (ret != 0):
            logging.warning("Failed to find version of clang")
            return -1
        line = return_buffer.getvalue().splitlines()[0].strip()
        return line[14:].strip()