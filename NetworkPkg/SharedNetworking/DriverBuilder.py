# @file Edk2BinaryBuild.py
# This module contains code that supports building of binary files
# This is the main entry for the build and test process of binary builds
##
# Copyright (c) Microsoft Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import sys
import logging
from edk2toolext.environment import plugin_manager
from edk2toolext.environment.plugintypes.uefi_helper_plugin import HelperFunctions
from edk2toolext.edk2_invocable import Edk2Invocable
from edk2toolext.environment import self_describing_environment
from edk2toolext.environment import shell_environment
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toolext import edk2_logging
# import pkg_resources
import DriverBuilder  # this is a little weird


class BinaryBuildSettingsManager():
    ''' Platform settings will be accessed through this implementation. '''

    def GetActiveScopes(self):
        ''' get scope '''
        raise NotImplementedError()

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        raise NotImplementedError()

    def GetPackagesPath(self):
        pass

    def GetConfigurations(self):
        '''
        Gets the next configuration of this run
        This is a generator pattern - use yield
        '''
        raise NotImplementedError()

    def PreFirstBuildHook(self):
        ''' Called after the before the first build '''
        return 0

    def PostFinalBuildHook(self, ret):
        ''' Called after the final build with the summed return code '''
        return 0

    def PostBuildHook(self, ret):
        ''' Called after each build with the return code '''
        return 0

    def PreBuildHook(self):
        ''' Called before each build '''
        return 0

    def GetName(self):
        ''' Get the name of the repo, platform, or product being build by CI '''
        raise NotImplementedError()

    def AddCommandLineOptions(self, parserObj):
        ''' Implement in subclass to add command line options to the argparser '''
        pass

    def RetrieveCommandLineOptions(self, args):
        '''  Implement in subclass to retrieve command line options from the argparser '''
        pass


class Edk2BinaryBuild(Edk2Invocable):
    def GetLoggingLevel(self, loggerType):
        ''' Get the logging level for a given type
        base == lowest logging level supported
        con  == Screen logging
        txt  == plain text file logging
        md   == markdown file logging
        '''
        if(loggerType == "con") and not self.Verbose:
            return logging.WARNING
        return logging.DEBUG

    def AddCommandLineOptions(self, parser):
        pass

    def RetrieveCommandLineOptions(self, args):
        '''  Retrieve command line options from the argparser '''
        pass

    def GetSettingsClass(self):
        return BinaryBuildSettingsManager

    def GetLoggingFileName(self, loggerType):
        return "BINARY_BUILDLOG"

    def Go(self):
        ret = 0
        env = shell_environment.GetBuildVars()
        env.SetValue("PRODUCT_NAME",
                     self.PlatformSettings.GetName(), "Platform Hardcoded")
        env.SetValue("BLD_*_BUILDID_STRING", "201905", "Current Version")
        env.SetValue("BUILDREPORTING", "TRUE", "Platform Hardcoded")
        env.SetValue("BUILDREPORT_TYPES",
                     'PCD DEPEX LIBRARY BUILD_FLAGS', "Platform Hardcoded")
        
        # make sure python_command is set
        python_command = sys.executable
        if " "in python_command:
            python_command = '"' + python_command + '"'
        shell_environment.GetEnvironment().set_shell_var("PYTHON_COMMAND", python_command)

        # Run pre build hook
        ret += self.PlatformSettings.PreFirstBuildHook()
        ws = self.GetWorkspaceRoot()
        pp = self.PlatformSettings.GetModulePkgsPath()
        # run each configuration
        ret = 0
        try:
            for config in self.PlatformSettings.GetConfigurations():
                pre_ret = self.PlatformSettings.PreBuildHook()  # run pre build hook
                if pre_ret != 0:
                    ret = pre_ret
                    raise RuntimeError("We failed in prebuild hook")
                edk2_logging.log_progress(f"--Running next configuration--")
                logging.info(config)
                shell_environment.CheckpointBuildVars()  # checkpoint our config
                env = shell_environment.GetBuildVars()
                # go through the config and apply to environement
                for key in config:
                    env.SetValue(key, config[key], "provided by configuration")
                # make sure to set this after in case the config did
                env.SetValue("TOOL_CHAIN_TAG", "VS2017", "provided by builder")
                platformBuilder = UefiBuilder()  # create our builder
                build_ret = platformBuilder.Go(ws, pp, self.helper, self.plugin_manager)
                # we always want to run the post build hook
                post_ret = self.PlatformSettings.PostBuildHook(ret)
                if build_ret != 0:
                    ret = build_ret
                    raise RuntimeError("We failed in build")
                if post_ret != 0:
                    ret = post_ret
                    raise RuntimeError("We failed in postbuild hook")
                shell_environment.RevertBuildVars()
        except RuntimeError:
            pass
        finally:
            # make sure to do our final build hook
            self.PlatformSettings.PostFinalBuildHook(ret)
        return ret


def main():
    Edk2BinaryBuild().Invoke()


if __name__ == "__main__":
    DriverBuilder.main()  # otherwise we're in __main__ context
