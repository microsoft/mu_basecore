##
# Script to Build Shared Crypto Driver
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# This is to build the SharedTls binaries for NuGet publishing
##
import os
import sys
import logging
from MuEnvironment.UefiBuild import UefiBuilder
from MuPythonLibrary import UtilityFunctions
from MuEnvironment import SelfDescribingEnvironment
import shutil
import glob
from importlib import reload  # Python 3.4+ only.

# ==========================================================================
# PLATFORM BUILD ENVIRONMENT CONFIGURATION
#
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
WORKSPACE_PATH = os.path.dirname(os.path.dirname(SCRIPT_PATH))
REQUIRED_REPOS = ('Common/MU_TIANO', "Silicon/Arm/MU_TIANO")
PROJECT_SCOPE = ("corebuild", "Silicon/Arm/MU_TIANO")

MODULE_PKGS = ('Common/MU_TIANO', "Silicon/Arm/MU_TIANO")
MODULE_PKG_PATHS = ";".join(os.path.join(WORKSPACE_PATH, pkg_name) for pkg_name in MODULE_PKGS)
VERSION = "2019.03.1.2"
TARGET_INDEX = 0
TARGET = ["RELEASE", "DEBUG"]
API_KEY = None

# Because we reimport this module we need to grab the other version when we are in PlatformBuildworker namespace as opposed to main
if __name__ == "__main__":
    API_KEY = None
    TARGET = ""
else:
    pbw = __import__("__main__")
    API_KEY = pbw.API_KEY

def GetAPIKey():
    global API_KEY
    return API_KEY

def SetAPIKey(key):
    global API_KEY
    API_KEY = key

def GetBuildTarget():
    global TARGET_INDEX
    return TARGET[TARGET_INDEX]

def CopyFile(srcDir, destDir, file_name):
    shutil.copyfile(os.path.join(srcDir, file_name), os.path.join(destDir, file_name))


def GetArchitecture(path: str):
    path = os.path.normpath(path)
    path_parts = path.split(os.path.sep)
    return path_parts[1]


def GetTarget(path: str):
    path = os.path.normpath(path)
    path_parts = path.split(os.path.sep)
    target = path_parts[0]

    if "_" in target:
        return target.split("_")[0]
    return None


def PublishNuget():
    # get the root directory of mu_basecore
    scriptDir = SCRIPT_PATH
    rootDir = WORKSPACE_PATH
    # move the EFI's we generated to a folder to upload
    NugetPath = os.path.join(rootDir, "BaseTools", "NugetPublishing")
    NugetFilePath = os.path.join(NugetPath, "NugetPublishing.py")
    if not os.path.exists(NugetFilePath):
        raise FileNotFoundError(NugetFilePath)

    logging.info("Running NugetPackager")
    output_dir = os.path.join(rootDir, "Build", ".NugetOutput")

    try:
        if os.path.exists(output_dir):
            shutil.rmtree(output_dir, ignore_errors=True)
        os.makedirs(output_dir)
    except:
        logging.error("Ran into trouble getting Nuget Output Path setup")
        return False

    # copy the md file
    CopyFile(scriptDir, output_dir, "SharedTlsDxe.md")
    CopyFile(scriptDir, output_dir, "release_notes.md")

    shared_tls_build_dir = os.path.realpath(os.path.join(rootDir, "Build", "NetworkPkg"))
    shared_tls_build_dir_offset = len(shared_tls_build_dir) + 1
    build_dir_efi_search = os.path.join(shared_tls_build_dir, "**", "Tls*.efi")
    build_dir_dpx_search = os.path.join(shared_tls_build_dir, "**", "Tls*.depex")
    build_dir_pdb_search = os.path.join(shared_tls_build_dir, "**", "Tls*.pdb")
    logging.info("Searching {0}".format(build_dir_efi_search))
    for binary in glob.iglob(build_dir_efi_search, recursive=True):
        MoveArchTargetSpecificFile(binary, shared_tls_build_dir_offset, output_dir)

    for depex in glob.iglob(build_dir_dpx_search, recursive=True):
        MoveArchTargetSpecificFile(depex, shared_tls_build_dir_offset, output_dir)

    for pdb in glob.iglob(build_dir_pdb_search, recursive=True):
        MoveArchTargetSpecificFile(pdb, shared_tls_build_dir_offset, output_dir)

    API_KEY = GetAPIKey()

    params = "--Operation PackAndPush --ConfigFilePath SharedTls.config.json --Version {0} --InputFolderPath {1}  --ApiKey {2}".format(VERSION, output_dir, API_KEY)
    ret = UtilityFunctions.RunPythonScript(NugetFilePath, params, capture=True, workingdir=scriptDir)

    if ret == 0:
        logging.critical("Nuget Finished")
        return True
    else:
        logging.error("Error happened with Nuget")
        return False


def MoveArchTargetSpecificFile(binary, offset, output_dir):
    binary_path = binary[offset:]
    binary_name = os.path.basename(binary)
    binary_folder = os.path.dirname(binary)
    arch = GetArchitecture(binary_path)
    target = GetTarget(binary_path)
    if target is None:
        raise FileExistsError("Unknown file {0}".format(binary))
    dest_path = os.path.join(output_dir, target, arch)
    dest_filepath = os.path.join(dest_path, binary_name)
    if os.path.exists(dest_filepath):
        logging.warning("Skipping {0}: {1} from {2}".format(binary_name, dest_filepath, binary))
        return

    if not os.path.exists(dest_path):
        os.makedirs(dest_path)
    # logging.warning("Copying {0}: {1}".format(binary_name,binary))
    CopyFile(binary_folder, dest_path, binary_name)


# --------------------------------------------------------------------------------------------------------
# Subclass the UEFI builder and add platform specific functionality.
#
class PlatformBuilder(UefiBuilder):

    def __init__(self, WorkSpace, PackagesPath, PInManager, PInHelper, args):
        super(PlatformBuilder, self).__init__(
            WorkSpace, PackagesPath, PInManager, PInHelper, args)

    def SetPlatformEnv(self):
        # we have to checkpoint our environment since we have to make target non-overridable and ShellEnvironment is a singleton
        self.shell_checkpoint = self.env.internal_shell_env.checkpoint()
        self.env.SetValue("ACTIVE_PLATFORM", "NetworkPkg/NetworkPkg.dsc", "Platform Hardcoded")
        self.env.SetValue("TARGET_ARCH", "IA32 X64 AARCH64", "Platform Hardcoded")
        self.env.SetValue("TARGET", GetBuildTarget(), "Platform Hardcoded")
        self.env.SetValue("CONF_TEMPLATE_DIR", "NetworkPkg", "Conf template directory hardcoded - temporary and should go away")

        self.env.SetValue("LaunchBuildLogProgram", "Notepad", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnSuccess", "False", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnError", "False", "default - will fail if already set", True)
        self.FlashImage = True  # we need to flash the image to restore the checkpoint

        return 0

    def PlatformPostBuild(self):

        global TARGET_INDEX
        if TARGET_INDEX == 1:  # if we are on the last target
            logging.critical("--Publishing NUGET --")
            PublishNuget()
        TARGET_INDEX += 1

        return 0

    # ------------------------------------------------------------------
    #
    # Method for the platform to check if a gated build is needed
    # This is part of the build flow.
    # return:
    #  True -  Gated build is needed (default)
    #  False - Gated build is not needed for this platform
    # ------------------------------------------------------------------
    def PlatformGatedBuildShouldHappen(self):
        return True

    #
    # Platform defined flash method
    #
    def PlatformFlashImage(self):
        # we have to restore checkpoint in flash as there are certain steps that run inbetween PostBuild and Flash (build report, etc)
        self.env.internal_shell_env.restore_checkpoint(self.shell_checkpoint) # revert the last checkpoint
        return 0
#
# ==========================================================================
#


# Smallest 'main' possible. Please don't add unnecessary code.
if __name__ == '__main__':
    # If CommonBuildEntry is not found, the mu_environment pip module has not been installed correctly

    if len(sys.argv) < 2:
        raise RuntimeError("You need to include your API key as the first argument")
    api_key = sys.argv[1]
    args = [sys.argv[0]]
    if len(sys.argv) > 2:
        args.extend(sys.argv[2:])
    sys.argv = args

    try:
        from MuEnvironment import CommonBuildEntry
    except ImportError:
        print("Running Python version {0} from {1}".format(sys.version, sys.executable))
        raise RuntimeError("Please run \"python -m pip install --upgrade mu_build\".\nContact Microsoft Project Mu team if you run into any problems.")

    # set the API key
    SetAPIKey(api_key)
    # Now that we have access to the entry, hand off to the common code.
    try:
        CommonBuildEntry.build_entry(SCRIPT_PATH, WORKSPACE_PATH, REQUIRED_REPOS, PROJECT_SCOPE, MODULE_PKGS, MODULE_PKG_PATHS, worker_module='DriverBuilder')
    except SystemExit:  # catch the sys.exit call from uefibuild
        print("Success")

    # this is hacky to close logging and restart it back to a default state
    logging.disable(logging.NOTSET)
    logging.shutdown()
    reload(logging)  # otherwise we get errors trying to talk to closed handlers

    try:
        CommonBuildEntry.build_entry(SCRIPT_PATH, WORKSPACE_PATH, REQUIRED_REPOS, PROJECT_SCOPE, MODULE_PKGS, MODULE_PKG_PATHS, worker_module='DriverBuilder')
    except SystemExit:
        print("Success")
