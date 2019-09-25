# @file edk2_platform_build
# Invocable classs that does a build.
# Needs a child of UefiBuilder for pre/post build steps.
##
# Copyright (c) Microsoft Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import sys
import logging
import pkg_resources
from edk2toolext import edk2_logging
from edk2toolext.environment import plugin_manager
from edk2toolext.environment.plugintypes.uefi_helper_plugin import HelperFunctions
from edk2toolext.environment import version_aggregator
from edk2toolext.environment import self_describing_environment
from edk2toolext.edk2_invocable import Edk2Invocable
from edk2toolext.invocables.edk2_setup import SetupSettingsManager
from edk2toolext.invocables.edk2_platform_build import BuildSettingsManager

PIP_PACKAGES_LIST = ["edk2-pytool-library", "edk2-pytool-extensions", "PyYaml"]


class ToolsBuildSettingsManager(SetupSettingsManager, BuildSettingsManager):
    ''' Platform settings will be accessed through this implementation. '''

    def GetActiveScopes(self):
        ''' get scope '''
        return ()

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
        # WORKSPACE_PATH = os.path.dirname(SCRIPT_PATH)
        WORKSPACE_PATH = SCRIPT_PATH

        return WORKSPACE_PATH

    def GetModulePkgsPath(self):
        raise NotImplementedError()

    def AddCommandLineOptions(self, parserObj):
        ''' Implement in subclass to add command line options to the argparser '''
        pass

    def RetrieveCommandLineOptions(self, args):
        '''  Implement in subclass to retrieve command line options from the argparser '''
        pass

    def GetLoggingLevel(self, loggerType):
        ''' Get the logging level for a given type
        base == lowest logging level supported
        con  == Screen logging
        txt  == plain text file logging
        md   == markdown file logging
        '''
        if loggerType in ('txt', 'md'):
            return None
        return logging.DEBUG


#
# Pass in a list of pip package names and they will be printed as well as
# reported to the global version_aggregator
def display_pip_package_info(package_list):
    for package in package_list:
        version = pkg_resources.get_distribution(package).version
        logging.info("{0} version: {1}".format(package, version))
        version_aggregator.GetVersionAggregator().ReportVersion(package, version, version_aggregator.VersionTypes.TOOL)


class Edk2ToolsBuild(Edk2Invocable):

    def ParseCommandLineOptions(self):
        self.PlatformSettings = ToolsBuildSettingsManager()
        self.Verbose = True

    def AddCommandLineOptions(self, parserObj):
        ''' adds command line options to the argparser '''
        pass

    def RetrieveCommandLineOptions(self, args):
        '''  Retrieve command line options from the argparser '''

        # If PlatformBuilder and PlatformSettings are seperate, give args to PlatformBuilder
        if self.PlatformBuilder is not self.PlatformSettings:
            self.PlatformBuilder.RetrieveCommandLineOptions(args)

    def GetSettingsClass(self):
        '''  Providing ToolsBuildSettingsManager  '''
        return ToolsBuildSettingsManager

    def GetLoggingLevel(self, loggerType):
        return self.PlatformSettings.GetLoggingLevel(loggerType)

    def GetLoggingFileName(self, loggerType):
        return None

    def Go(self):
        logging.info("Running Python version: " + str(sys.version_info))

        display_pip_package_info(PIP_PACKAGES_LIST)

        (build_env, shell_env) = self_describing_environment.BootstrapEnvironment(
            self.GetWorkspaceRoot(), self.GetActiveScopes())

        # # Bind our current execution environment into the shell vars.
        # ph = os.path.dirname(sys.executable)
        # if " " in ph:
        #     ph = '"' + ph + '"'
        # shell_env.set_shell_var("PYTHON_HOME", ph)
        # # PYTHON_COMMAND is required to be set for using edk2 python builds.
        # # todo: work with edk2 to remove the bat file and move to native python calls
        # pc = sys.executable
        # if " " in pc:
        #     pc = '"' + pc + '"'
        # shell_env.set_shell_var("PYTHON_COMMAND", pc)

        # # Load plugins
        # logging.log(edk2_logging.SECTION, "Loading Plugins")
        # pm = plugin_manager.PluginManager()
        # failedPlugins = pm.SetListOfEnvironmentDescriptors(
        #     build_env.plugins)
        # if failedPlugins:
        #     logging.critical("One or more plugins failed to load. Halting build.")
        #     for a in failedPlugins:
        #         logging.error("Failed Plugin: {0}".format(a["name"]))
        #     raise Exception("One or more plugins failed to load.")

        # helper = HelperFunctions()
        # if(helper.LoadFromPluginManager(pm) > 0):
        #     raise Exception("One or more helper plugins failed to load.")
        # #
        # # Now we can actually kick off a build.
        # #
        # logging.log(edk2_logging.SECTION, "Kicking off build")
        # return self.PlatformBuilder.Go(self.GetWorkspaceRoot(),
        #                                self.PlatformSettings.GetModulePkgsPath(),
        #                                helper, pm)


def main():
    Edk2ToolsBuild().Invoke()

if __name__ == "__main__":
    main()
