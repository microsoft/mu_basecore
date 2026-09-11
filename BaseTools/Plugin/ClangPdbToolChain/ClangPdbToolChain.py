# @file ClangPdbToolChain.py
# Plugin to configures paths for the ClangPdb tool chain
##
# This plugin works in conjuncture with the tools_def
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import logging
import os
import shutil
from io import StringIO

from edk2toolext.environment import shell_environment, version_aggregator
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toollib.utility_functions import GetHostInfo, RunCmd


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
                # need to escape the last slash as it seems to be removed
                ClangBin_Default = "C:\\Program Files\\LLVM\\bin\\\\"
                clang_exe += ".exe"
            elif HostInfo.os == "Linux":
                ClangBin_Default = "/LLVM/bin/"  # this isn't right
            else:
                pass
                # no defaults set

            ClangBin = shell_environment.GetEnvironment().get_shell_var("CLANG_BIN")
            if ClangBin is not None:
                self.Logger.info("CLANG_BIN is already set.")
            else:
                # see if clang is on path.
                clang_path = shutil.which(clang_exe)
                if clang_path is not None:
                    ClangBin = os.path.dirname(os.path.realpath(clang_path)) + os.sep
                if ClangBin is None:
                    # Didn't find it on path - try the install default.
                    ClangBin = ClangBin_Default

                shell_environment.GetEnvironment().set_shell_var("CLANG_BIN", ClangBin)

            version_aggregator.GetVersionAggregator().ReportVersion(
                "CLANG BIN", ClangBin, version_aggregator.VersionTypes.INFO
            )

            # now confirm it exists
            if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("CLANG_BIN")):
                self.Logger.error("Clang was not found!")
                if ClangBin == ClangBin_Default:
                    self.Logger.error(f"The default clang bin path does not exist: {ClangBin.rstrip(os.sep)}.")
                    self.Logger.error("Set the CLANG_BIN variable in your shell to the installation bin directory.")
                else:
                    self.Logger.error(f"The path provided in CLANG_BIN does not exist: {ClangBin}.")
                return -2

            version_aggregator.GetVersionAggregator().ReportVersion(
                "CLANG Version", self._get_clang_version(ClangBin), version_aggregator.VersionTypes.TOOL
            )

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
        if ret != 0:
            logging.warning("Failed to find version of clang")
            return -1
        line = return_buffer.getvalue().splitlines()[0].strip()
        return line[14:].strip()
