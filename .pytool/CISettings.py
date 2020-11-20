# @file
#
# Copyright (c) Microsoft Corporation.
# Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
# Copyright (c) 2020, ARM Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
from edk2toolext.environment import shell_environment
from edk2toolext.invocables.edk2_ci_build import CiBuildSettingsManager
from edk2toolext.invocables.edk2_ci_setup import CiSetupSettingsManager     # MU_CHANGE
from edk2toolext.invocables.edk2_setup import SetupSettingsManager, RequiredSubmodule
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toolext.invocables.edk2_pr_eval import PrEvalSettingsManager
from edk2toollib.utility_functions import GetHostInfo


# MU_CHANGE - Add CiSetupSettingsManager superclass.
class Settings(CiSetupSettingsManager, CiBuildSettingsManager, UpdateSettingsManager, SetupSettingsManager, PrEvalSettingsManager):

    def __init__(self):
        self.ActualPackages = []
        self.ActualTargets = []
        self.ActualArchitectures = []
        self.ActualToolChainTag = ""

    # ####################################################################################### #
    #                             Extra CmdLine configuration                                 #
    # ####################################################################################### #

    def AddCommandLineOptions(self, parserObj):
        pass

    def RetrieveCommandLineOptions(self, args):
        pass

    # ####################################################################################### #
    #                        Default Support for this Ci Build                                #
    # ####################################################################################### #

    def GetPackagesSupported(self):
        ''' return iterable of edk2 packages supported by this build.
        These should be edk2 workspace relative paths '''

        return ("BaseTools", # MU_CHANGE
                "MdePkg",
                "MdeModulePkg",
                "NetworkPkg",
                "PcAtChipsetPkg",
                "SecurityPkg",
                "UefiCpuPkg",
                "UnitTestFrameworkPkg"
                )

    def GetArchitecturesSupported(self):
        ''' return iterable of edk2 architectures supported by this build '''
        return (
                "IA32",
                "X64",
                "ARM",
                "AARCH64")

    def GetTargetsSupported(self):
        ''' return iterable of edk2 target tags supported by this build '''
        return ("DEBUG", "RELEASE", "NO-TARGET", "NOOPT")

    # ####################################################################################### #
    #                     Verify and Save requested Ci Build Config                           #
    # ####################################################################################### #

    def SetPackages(self, list_of_requested_packages):
        ''' Confirm the requested package list is valid and configure SettingsManager
        to build the requested packages.

        Raise UnsupportedException if a requested_package is not supported
        '''
        unsupported = set(list_of_requested_packages) - \
            set(self.GetPackagesSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Package Requested: " + " ".join(unsupported))
            raise Exception("Unsupported Package Requested: " +
                            " ".join(unsupported))
        self.ActualPackages = list_of_requested_packages

    def SetArchitectures(self, list_of_requested_architectures):
        ''' Confirm the requests architecture list is valid and configure SettingsManager
        to run only the requested architectures.

        Raise Exception if a list_of_requested_architectures is not supported
        '''
        unsupported = set(list_of_requested_architectures) - \
            set(self.GetArchitecturesSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Architecture Requested: " + " ".join(unsupported))
            raise Exception(
                "Unsupported Architecture Requested: " + " ".join(unsupported))
        self.ActualArchitectures = list_of_requested_architectures

    def SetTargets(self, list_of_requested_target):
        ''' Confirm the request target list is valid and configure SettingsManager
        to run only the requested targets.

        Raise UnsupportedException if a requested_target is not supported
        '''
        unsupported = set(list_of_requested_target) - \
            set(self.GetTargetsSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Targets Requested: " + " ".join(unsupported))
            raise Exception("Unsupported Targets Requested: " +
                            " ".join(unsupported))
        self.ActualTargets = list_of_requested_target

    # ####################################################################################### #
    #                         Actual Configuration for Ci Build                               #
    # ####################################################################################### #

    def GetActiveScopes(self):
        ''' return tuple containing scopes that should be active for this process '''
        scopes = ("cibuild", "edk2-build", "host-based-test")

        self.ActualToolChainTag = shell_environment.GetBuildVars().GetValue("TOOL_CHAIN_TAG", "")

        if GetHostInfo().os.upper() == "LINUX" and self.ActualToolChainTag.upper().startswith("GCC"):
            if "AARCH64" in self.ActualArchitectures:
                scopes += ("gcc_aarch64_linux",)
            if "ARM" in self.ActualArchitectures:
                scopes += ("gcc_arm_linux",)
            if "RISCV64" in self.ActualArchitectures:
                scopes += ("gcc_riscv64_unknown",)

        return scopes

    def GetRequiredSubmodules(self):
        ''' return iterable containing RequiredSubmodule objects.
        If no RequiredSubmodules return an empty iterable
        '''
        rs = []
        rs.append(RequiredSubmodule(
            "UnitTestFrameworkPkg/Library/CmockaLib/cmocka", False))
        rs.append(RequiredSubmodule(
            "MdeModulePkg/Universal/RegularExpressionDxe/oniguruma", False))
        rs.append(RequiredSubmodule(
            "MdeModulePkg/Library/BrotliCustomDecompressLib/brotli", False))
        rs.append(RequiredSubmodule(
            "BaseTools/Source/C/BrotliCompress/brotli", False))
        return rs

    def GetName(self):
        # MU_CHANGE
        return "Basecore"

    def GetDependencies(self):
        # MU_CHANGE BEGIN
        ''' Return Git Repository Dependencies

        Return an iterable of dictionary objects with the following fields
        {
            Path: <required> Workspace relative path
            Url: <required> Url of git repo
            Commit: <optional> Commit to checkout of repo
            Branch: <optional> Branch to checkout (will checkout most recent commit in branch)
            Full: <optional> Boolean to do shallow or Full checkout.  (default is False)
            ReferencePath: <optional> Workspace relative path to git repo to use as "reference"
        }
        '''
        return [
            {
                "Path": "Silicon/Arm/MU_TIANO",
                "Url": "https://github.com/Microsoft/mu_silicon_arm_tiano.git",
                "Branch": "dev/201908"
            },
            {
                "Path": "Common/MU_TIANO",
                "Url": "https://github.com/Microsoft/mu_tiano_plus.git",
                "Branch": "dev/201908"
            }
        ]
        # MU_CHANGE END

    def GetPackagesPath(self):
        # MU_CHANGE BEGIN
        ''' Return a list of workspace relative paths that should be mapped as edk2 PackagesPath '''
        result = []
        for a in self.GetDependencies():
            result.append(a["Path"])
        return result
        # MU_CHANGE END

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    def FilterPackagesToTest(self, changedFilesList: list, potentialPackagesList: list) -> list:
        ''' Filter potential packages to test based on changed files. '''
        build_these_packages = []
        possible_packages = potentialPackagesList.copy()
        for f in changedFilesList:
            # split each part of path for comparison later
            nodes = f.split("/")

            # python file change in .pytool folder causes building all
            if f.endswith(".py") and ".pytool" in nodes:
                build_these_packages = possible_packages
                break

            # BaseTools files that might change the build
            if "BaseTools" in nodes:
                if os.path.splitext(f) not in [".txt", ".md"]:
                    build_these_packages = possible_packages
                    break
        return build_these_packages
