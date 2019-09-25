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
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toollib.utility_functions import RunCmd
from edk2toollib.windows.locate_tools import QueryVcVariables

PIP_PACKAGES_LIST = ["edk2-pytool-library", "edk2-pytool-extensions", "PyYaml"]


class ToolsBuildSettingsManager(UpdateSettingsManager):
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

    def GetPackagesSupported(self):
        ''' return iterable of edk2 packages supported by this build.
        These should be edk2 workspace relative paths '''
        return []

    def GetArchitecturesSupported(self):
        ''' return iterable of edk2 architectures supported by this build '''
        return ['IA32']

    def GetTargetsSupported(self):
        ''' return iterable of edk2 target tags supported by this build '''
        return []

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
        ph = os.path.dirname(sys.executable)
        if " " in ph:
            ph = '"' + ph + '"'
        shell_env.set_shell_var("PYTHON_HOME", ph)
        # PYTHON_COMMAND is required to be set for using edk2 python builds.
        pc = sys.executable
        if " " in pc:
            pc = '"' + pc + '"'
        shell_env.set_shell_var("PYTHON_COMMAND", pc)

        # # Update environment with required VCvars.
        interesting_keys = ["ExtensionSdkDir", "INCLUDE", "LIB"]
        interesting_keys.extend(["LIBPATH", "Path", "UniversalCRTSdkDir", "UCRTVersion", "WindowsLibPath", "WindowsSdkBinPath"])
        interesting_keys.extend(["WindowsSdkDir", "WindowsSdkVerBinPath", "WindowsSDKVersion","VCToolsInstallDir"])
        vc_vars = QueryVcVariables(interesting_keys, 'x86', vs_version = 'vs2017')
        for key in vc_vars.keys():
            if key.lower() == 'path':
                shell_env.insert_path(vc_vars[key])
            else:
                shell_env.set_shell_var(key, vc_vars[key])

        # # Update the EDK_TOOLS_PATH so that new tools are build to the correct location.
        shell_env.set_shell_var('EDK_TOOLS_PATH', shell_env.get_shell_var('BASE_TOOLS_PATH'))
        # We'll need to run 'antlr' after it's built, so we should know where that's going.
        shell_env.append_path(os.path.join(shell_env.get_shell_var('BASE_TOOLS_PATH'), 'Bin', 'Win32'))

        print(os.environ)

        # # Actually build the tools.
        if RunCmd('nmake.exe', None, workingdir=self.PlatformSettings.GetWorkspaceRoot()) != 0:
            raise Exception("Failed to build.")


def main():
    Edk2ToolsBuild().Invoke()

if __name__ == "__main__":
    main()
