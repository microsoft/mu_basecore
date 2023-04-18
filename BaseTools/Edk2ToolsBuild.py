# @file Edk2ToolsBuild.py
# Invocable class that builds the basetool c files.
#
# Supports VS2017, VS2019, and GCC5
##
# Copyright (c) Microsoft Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import sys
import logging
import argparse
import multiprocessing
import shutil
from edk2toolext import edk2_logging
from edk2toolext.environment import self_describing_environment, shell_environment
from edk2toolext.base_abstract_invocable import BaseAbstractInvocable
from edk2toollib.utility_functions import RunCmd
from edk2toollib.utility_functions import GetHostInfo # MU_CHANGE: Need to check if this is cross compilation
from edk2toollib.windows.locate_tools import QueryVcVariables


class Edk2ToolsBuild(BaseAbstractInvocable):

    def ParseCommandLineOptions(self):
        ''' parse arguments '''
        ParserObj = argparse.ArgumentParser()
        ParserObj.add_argument("-t", "--tool_chain_tag", dest="tct", default="VS2017",
                               help="Set the toolchain used to compile the build tools")
        # MU_CHANGE
        ParserObj.add_argument("-s", "--skip_path_env", dest="skip_env", default=False, action='store_true',
                               help="Skip the creation of the path_env descriptor file")
        ParserObj.add_argument("-a", "--target_arch", dest="arch", default=None, choices=[None, 'IA32', 'X64', 'ARM', 'AARCH64'],
                               help="Specify the architecture of the built base tools")
        args = ParserObj.parse_args()
        self.tool_chain_tag = args.tct
        self.target_arch = args.arch
        # MU_CHANGE
        self.skip_path_env = args.skip_env

    def GetWorkspaceRoot(self):
        ''' Return the workspace root for initializing the SDE '''

        # this is the bastools dir...not the traditional EDK2 workspace root
        return os.path.dirname(os.path.abspath(__file__))

    def GetActiveScopes(self):
        ''' return tuple containing scopes that should be active for this process '''

        scopes = ('global',)
        if GetHostInfo().os == "Linux" and self.tool_chain_tag.lower().startswith("gcc"):
            if self.target_arch is None:
                return scopes
            if "AARCH64" in self.target_arch:
                scopes += ("gcc_aarch64_linux",)
            if "ARM" in self.target_arch:
                scopes += ("gcc_arm_linux",)
        return scopes

    def GetLoggingLevel(self, loggerType):
        ''' Get the logging level for a given type (return Logging.Level)
        base == lowest logging level supported
        con  == Screen logging
        txt  == plain text file logging
        md   == markdown file logging
        '''
        if(loggerType == "con"):
            return logging.ERROR
        else:
            return logging.DEBUG

    def GetLoggingFolderRelativeToRoot(self):
        ''' Return a path to folder for log files '''
        return "BaseToolsBuild"

    def GetVerifyCheckRequired(self):
        ''' Will call self_describing_environment.VerifyEnvironment if this returns True '''
        return True

    def GetLoggingFileName(self, loggerType):
        ''' Get the logging file name for the type.
        Return None if the logger shouldn't be created

        base == lowest logging level supported
        con  == Screen logging
        txt  == plain text file logging
        md   == markdown file logging
        '''
        return "BASETOOLS_BUILD"

    def WritePathEnvFile(self, OutputDir):
        ''' Write a PyTool path env file for future PyTool based edk2 builds'''
        content = '''##
# Set shell variable EDK_TOOLS_BIN to this folder
#
# Autogenerated by Edk2ToolsBuild.py
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
{
  "id": "You-Built-BaseTools",
  "scope": "edk2-build",
  "flags": ["set_shell_var", "set_path"],
  "var_name": "EDK_TOOLS_BIN"
}
'''
        with open(os.path.join(OutputDir, "basetoolsbin_path_env.yaml"), "w") as f:
            f.write(content)

    def Go(self):
        logging.info("Running Python version: " + str(sys.version_info))

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

        if self.tool_chain_tag.lower().startswith("vs"):
            # MU_CHANGE: Specify target architecture
            if self.target_arch is not None:
                # Put a default as IA32
                self.target_arch = "IA32"

            if self.target_arch == "IA32":
                VcToolChainArch = "x86"
                TargetInfoArch = "x86"
            elif self.target_arch == "ARM":
                VcToolChainArch = "x86_arm"
                TargetInfoArch = "ARM"
            else:
                raise NotImplementedError()
            # MU_CHANGE

            # # Update environment with required VC vars.
            interesting_keys = ["ExtensionSdkDir", "INCLUDE", "LIB"]
            interesting_keys.extend(
                ["LIBPATH", "Path", "UniversalCRTSdkDir", "UCRTVersion", "WindowsLibPath", "WindowsSdkBinPath"])
            interesting_keys.extend(
                ["WindowsSdkDir", "WindowsSdkVerBinPath", "WindowsSDKVersion", "VCToolsInstallDir"])
            vc_vars = QueryVcVariables(
                interesting_keys, VcToolChainArch, vs_version=self.tool_chain_tag.lower()) # MU_CHANGE
            for key in vc_vars.keys():
                logging.debug(f"Var - {key} = {vc_vars[key]}")
                if key.lower() == 'path':
                    shell_env.insert_path(vc_vars[key])
                else:
                    shell_env.set_shell_var(key, vc_vars[key])

            # MU_CHANGE: Specify target architecture
            # Note: This HOST_ARCH is in respect to the BUILT base tools, not the host arch where
            # this script is BUILDING the base tools.
            shell_env.set_shell_var('HOST_ARCH', self.target_arch)

            self.OutputDir = os.path.join(
                shell_env.get_shell_var("EDK_TOOLS_PATH"), "Bin", "Win32")

            # compiled tools need to be added to path because antlr is referenced
            # MU_CHANGE: Added logic to support cross compilation scenarios
            HostInfo = GetHostInfo()
            if TargetInfoArch == HostInfo.arch:
                # not cross compiling
                shell_env.insert_path(self.OutputDir)
            else:
                # cross compiling
                shell_env.insert_path(shell_env.get_shell_var("EDK_TOOLS_BIN"))

            # Actually build the tools.
            ret = RunCmd('nmake.exe', None,
                         workingdir=shell_env.get_shell_var("EDK_TOOLS_PATH"))
            if ret != 0:
                raise Exception("Failed to build.")

            # MU_CHANGE
            if not self.skip_path_env:
                self.WritePathEnvFile(self.OutputDir)
            return ret

        elif self.tool_chain_tag.lower().startswith("gcc"):
            # MU_CHANGE: Specify target architecture
            # Note: This HOST_ARCH is in respect to the BUILT base tools, not the host arch where
            # this script is BUILDING the base tools.
            prefix = None
            TargetInfoArch = None
            if self.target_arch is not None:
                shell_env.set_shell_var('HOST_ARCH', self.target_arch)

                if "AARCH64" in self.target_arch:
                    # now check for install dir.  If set then set the Prefix
                    install_path = shell_environment.GetEnvironment().get_shell_var("GCC5_AARCH64_INSTALL")

                    # make GCC5_AARCH64_PREFIX to align with tools_def.txt
                    prefix = os.path.join(install_path, "bin", "aarch64-none-linux-gnu-")
                    shell_environment.GetEnvironment().set_shell_var("GCC_PREFIX", prefix)
                    TargetInfoArch = "ARM"

                elif "ARM" in self.target_arch:
                    # now check for install dir.  If set then set the Prefix
                    install_path = shell_environment.GetEnvironment().get_shell_var("GCC5_ARM_INSTALL")

                    # make GCC5_ARM_PREFIX to align with tools_def.txt
                    prefix = os.path.join(install_path, "bin", "arm-none-linux-gnueabihf-")
                    shell_environment.GetEnvironment().set_shell_var("GCC_PREFIX", prefix)
                    TargetInfoArch = "ARM"

                else:
                    TargetInfoArch = "x86"

            # Otherwise, the built binary arch will be consistent with the host system

            # Added logic to support cross compilation scenarios
            HostInfo = GetHostInfo()
            if TargetInfoArch != HostInfo.arch:
                # this is defaulting to the version that comes with Ubuntu 20.04
                ver = shell_environment.GetBuildVars().GetValue("LIBUUID_VERSION", "2.34")
                work_dir = os.path.join(shell_env.get_shell_var("EDK_TOOLS_PATH"), self.GetLoggingFolderRelativeToRoot())
                pack_name = f"util-linux-{ver}"
                unzip_dir = os.path.join(work_dir, pack_name)

                if os.path.isfile(os.path.join(work_dir, f"{pack_name}.tar.gz")):
                    os.remove(os.path.join(work_dir, f"{pack_name}.tar.gz"))
                if os.path.isdir(unzip_dir):
                    shutil.rmtree(unzip_dir)

                # cross compiling, need to rebuild libuuid for the target
                ret = RunCmd("wget", f"https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v{ver}/{pack_name}.tar.gz", workingdir=work_dir)
                if ret != 0:
                    raise Exception(f"Failed to download libuuid version {ver} - {ret}")

                ret = RunCmd("tar", f"xvzf {pack_name}.tar.gz", workingdir=work_dir)
                if ret != 0:
                    raise Exception(f"Failed to untar the downloaded file {ret}")

                # configure the source to use the cross compiler
                pack_name = f"util-linux-{ver}"
                if "AARCH64" in self.target_arch:
                    ret = RunCmd("sh", f"./configure --host=aarch64-linux  -disable-all-programs --enable-libuuid CC={prefix}gcc", workingdir=unzip_dir)
                elif "ARM" in self.target_arch:
                    ret = RunCmd("sh", f"./configure --host=arm-linux  -disable-all-programs --enable-libuuid CC={prefix}gcc", workingdir=unzip_dir)
                if ret != 0:
                    raise Exception(f"Failed to configure the util-linux to build with our gcc {ret}")

                ret = RunCmd("make", "", workingdir=unzip_dir)
                if ret != 0:
                    raise Exception(f"Failed to build the libuuid with our gcc {ret}")

                shell_environment.GetEnvironment().set_shell_var("CROSS_LIB_UUID", unzip_dir)
                shell_environment.GetEnvironment().set_shell_var("CROSS_LIB_UUID_INC", os.path.join(unzip_dir, "libuuid", "src"))

            ret = RunCmd("make", "clean", workingdir=shell_env.get_shell_var("EDK_TOOLS_PATH"))
            if ret != 0:
                raise Exception("Failed to build.")

            cpu_count = self.GetCpuThreads()
            ret = RunCmd("make", f"-C .  -j {cpu_count}", workingdir=shell_env.get_shell_var("EDK_TOOLS_PATH"))
            if ret != 0:
                raise Exception("Failed to build.")

            self.OutputDir = os.path.join(
                shell_env.get_shell_var("EDK_TOOLS_PATH"), "Source", "C", "bin")

            # MU_CHANGE
            if not self.skip_path_env:
                self.WritePathEnvFile(self.OutputDir)
            return ret

        logging.critical("Tool Chain not supported")
        return -1

    def GetCpuThreads(self) -> int:
        ''' Function to return number of cpus. If error return 1'''
        cpus = 1
        try:
            cpus = multiprocessing.cpu_count()
        except:
            # from the internet there are cases where cpu_count is not implemented.
            # will handle error by just doing single proc build
            pass
        return cpus



def main():
    Edk2ToolsBuild().Invoke()


if __name__ == "__main__":
    main()
