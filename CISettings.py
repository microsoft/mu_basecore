# @file
#
# Copyright (c) 2018, Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
from edk2toolext.environment import shell_environment
from edk2toolext.invocables.edk2_ci_build import CiBuildSettingsManager
from edk2toolext.invocables.edk2_ci_setup import CiSetupSettingsManager
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toollib.utility_functions import GetHostInfo


class Settings(CiBuildSettingsManager, CiSetupSettingsManager, UpdateSettingsManager):

    def __init__(self):
        plugin_skip_list = ["DependencyCheck"]
        env = shell_environment.GetBuildVars()
        for plugin in plugin_skip_list:
            env.SetValue(plugin.upper(), "skip", "set from settings file")
        pass

    def AddCommandLineOptions(self, parserObj):
        parserObj.add_argument('--Tool_Chain', "--toolchain", "--tool_chain", dest='tool_chain_tag', type=str, help='tool chain tag to use for this build')

    def RetrieveCommandLineOptions(self, args):
        if args.tool_chain_tag is not None:
            shell_environment.GetBuildVars().SetValue("TOOL_CHAIN_TAG", args.tool_chain_tag, "Set as cli parameter")
        # cache this so usage within CISettings is consistant. 
        self.ToolChainTagCacheValue = args.tool_chain_tag

    def GetActiveScopes(self):
        ''' get scope '''
        scopes = ("corebuild", "project_mu")

        if (GetHostInfo().os == "Linux"
            and "AARCH64" in self.GetArchSupported() and
            self.ToolChainTagCacheValue is not None and
            self.ToolChainTagCacheValue.upper().startswith("GCC")):
            
            scopes += ("gcc_aarch64_linux",)

        return scopes

    def GetName(self):
        return "Basecore"

    def GetDependencies(self):
        return [
            {
                "Path": "Silicon/Arm/MU_TIANO",
                "Url": "https://github.com/Microsoft/mu_silicon_arm_tiano.git",
                "Branch": "dev/201905"
            },
            {
                "Path": "Common/MU_TIANO",
                "Url": "https://github.com/Microsoft/mu_tiano_plus.git",
                "Branch": "dev/201905"
            }
        ]

    def GetPackages(self):
        return ("MdeModulePkg",
            "MdePkg",
            "MsUnitTestPkg",
            "NetworkPkg",
            "PcAtChipsetPkg",
            "SecurityPkg",
            "UefiCpuPkg")

    def GetPackagesPath(self):
        return ()

    def GetArchSupported(self):
        return ("IA32",
                "X64",
                "AARCH64")

    def GetTargetsSupported(self):
        return ("DEBUG", "RELEASE")

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return os.path.dirname(os.path.abspath(__file__))