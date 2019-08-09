## @file WinRcPath.py
# Plugin to find Windows SDK Resource Compiler rc.exe
##
# This plugin works in conjucture with the tools_def to support rc.exe
# Copyright (c) Microsoft Corporation
#
##
import os
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
import edk2toollib.windows.locate_tools as locate_tools
from edk2toolext.environment import shell_environment

class WinRcPath(IUefiBuildPlugin):

    def do_post_build(self, thebuilder):
        return 0

    def do_pre_build(self, thebuilder):
        #get the locate tools module
        path = locate_tools.FindToolInWinSdk("rc.exe")
        if path is None:
            thebuilder.logging.warning("Failed to find rc.exe")
        else:
            p = os.path.abspath(os.path.dirname(path))
            shell_environment.GetEnvironment().set_shell_var("WINSDK_PATH_FOR_RC_EXE", p)
        return 0
