##
# Script to Build Shared Crypto Driver
# Copyright Microsoft Corporation, 2019
#
# This is to build the SharedNetworking binaries for NuGet publishing
##
import os
import sys
import logging
from MuEnvironment.UefiBuild import UefiBuilder
from MuPythonLibrary import UtilityFunctions
from MuEnvironment import CommonBuildEntry
from MuEnvironment.NugetDependency import NugetDependency
from MuPythonLibrary.UtilityFunctions import RunCmd
import shutil
import tempfile
import argparse
import glob
from io import StringIO
#from setuptools_scm import get_version
import re

try:
    from importlib import reload  # Python 3.4+ only.
except ImportError:
    print("You need at least Python 3.4+. Please upgrade!")
    raise


# ==========================================================================
# PLATFORM BUILD ENVIRONMENT CONFIGURATION
#
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
WORKSPACE_PATH = os.path.dirname(os.path.dirname(SCRIPT_PATH))
REQUIRED_REPOS = ('Common/MU_TIANO', "Silicon/Arm/MU_TIANO")
PROJECT_SCOPE = ("corebuild", "sharednetworking_build")

MODULE_PKGS = ("NetworkPkg/SharedNetworking/MU_ARM_TIANO_extdep/MU_ARM_TIANO", "NetworkPkg/SharedNetworking/MU_TIANO_extdep/MU_TIANO")
MODULE_PKG_PATHS = ";".join(os.path.join(WORKSPACE_PATH, pkg_name) for pkg_name in MODULE_PKGS)
SHOULD_DUMP_VERSION = False
RELEASE_NOTES_FILENAME ="release_notes.md"
PACKAGE_NAME="Mu-SharedNetworking"
BUILD_INDEX = 0
VERSION = None
TARGETS = ["RELEASE", "DEBUG"]
ARCHS = ["X64", "AARCH64", "IA32"]
OPTIONS = []
# combine options together
for target in TARGETS:
    for arch in ARCHS:
        OPTIONS.append({"target":target, "arch":arch})
API_KEY = None

output_dir = os.path.join(WORKSPACE_PATH, "Build", ".NugetOutput")

try:
    if os.path.exists(output_dir) and "--clean" in (" ".join(sys.argv)).lower():
        shutil.rmtree(output_dir, ignore_errors=True)
    os.makedirs(output_dir)
except:
    pass



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

def GetBuildOver():
    global BUILD_INDEX
    if BUILD_INDEX >= len(OPTIONS):
        return True
    else:
        return False

def GetBuildTarget():
    global OPTIONS
    global BUILD_INDEX
    return OPTIONS[BUILD_INDEX]["target"]

def GetBuildArch():
    global OPTIONS
    global BUILD_INDEX
    return OPTIONS[BUILD_INDEX]["arch"]

def ShouldDumpVersion():
    if __name__ == "__main__":
        global SHOULD_DUMP_VERSION
        return SHOULD_DUMP_VERSION
    else:
        pbw = __import__("__main__")
        return pbw.SHOULD_DUMP_VERSION

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


## Functions for getting the next nuget version
def GetLatestNugetVersion(package_name, source=None):
    cmd = NugetDependency.GetNugetCmd()
    cmd += ["list"]
    cmd += [package_name]
    if source is not None:
        cmd += ["-Source", source]
    return_buffer = StringIO()
    if (RunCmd(cmd[0], " ".join(cmd[1:]), outstream=return_buffer) == 0):
        # Seek to the beginning of the output buffer and capture the output.
        return_buffer.seek(0)
        return_string = return_buffer.read()
        return_buffer.close()
        return return_string.strip().strip(package_name).strip()
    else:
        return "0.0.0.0"


def GetReleaseNote():
    cmd = "log --format=%B -n 1 HEAD"
    return_buffer = StringIO()
    if (RunCmd("git", cmd, outstream=return_buffer) == 0):
        # Seek to the beginning of the output buffer and capture the output.
        return_buffer.seek(0)
        return_string = return_buffer.read(155).replace("\n"," ")  # read the first 155 characters and replace the
        return_buffer.close()
        # TODO: figure out if there was more input and append a ... if needed
        return return_string.strip()
    else:
        raise RuntimeError("Unable to read release notes")


def CreateReleaseNotes(note:str, new_version:str, old_release_notes:list, hashes:dict):
    scriptDir = SCRIPT_PATH

    release_notes_path = os.path.join(scriptDir, RELEASE_NOTES_FILENAME)
    current_notes_file  = open(release_notes_path, "r")
    notes = current_notes_file.readlines()
    current_notes_file.close()

    notes += ["\n"]
    notes += ["## {}- {}\n".format(new_version, note), "\n"]
    for repo in hashes:
        repo_hash = hashes[repo]
        notes += ["{}@{}\n".format(repo, repo_hash)]
    notes += ["\n"]

    # write our notes out to the file
    current_notes_file  = open(release_notes_path, "w")
    current_notes_file.writelines(notes)
    current_notes_file.writelines(old_release_notes)
    current_notes_file.close()


def GetCommitHashes(root_dir:os.PathLike):
    # Recursively looks at every .git and gets the commit from there
    search_path = os.path.join(root_dir, "**", ".git")
    search = glob.iglob(search_path, recursive=True)
    found_repos = {}
    cmd_args = "rev-parse HEAD"
    for git_path in search:
        git_path_dir = os.path.dirname(git_path)
        _, git_repo_name = os.path.split(git_path_dir)
        git_repo_name = git_repo_name.upper()
        if git_repo_name in found_repos:
            raise RuntimeError("we've already found this repo before "+git_repo_name)
        # read the git hash for this repo
        return_buffer = StringIO()
        RunCmd("git", cmd_args, workingdir=git_path_dir, outstream=return_buffer)
        commit_hash = return_buffer.getvalue().strip()
        return_buffer.close()
        found_repos[git_repo_name] = commit_hash
    return found_repos


def GetOldReleaseNotesAndHashes(notes_path:os.PathLike):
    # Read in the text file
    if not os.path.isfile(notes_path):
        raise FileNotFoundError("Unable to find the old release notes")
    old_notes_file  = open(notes_path, "r")
    notes = old_notes_file.readlines()
    old_notes_file.close()
    version_re = re.compile(r'##\s*\d+\.\d+\.\d+\.\d+')
    while len(notes) > 0:  # go through the notes until we find a version
        if not version_re.match(notes[0]):
            del notes[0]
        else:
            break
    old_hashes = {}
    hash_re = re.compile(r'([\w_]+)@([\w\d]+)')
    for note_line in notes:
        hash_match = hash_re.match(note_line)
        if hash_match:
            repo, commit_hash = hash_match.groups()
            repo = str(repo).upper()
            if repo not in old_hashes:
                old_hashes[repo] = commit_hash

    return (notes, old_hashes)


def DownloadNugetPackageVersion(package_name:str, version:str, destination:os.PathLike, source=None):
    cmd = NugetDependency.GetNugetCmd()
    cmd += ["install", package_name]
    if source is not None:
        cmd += ["-Source", source]
    cmd += ["-ExcludeVersion"]
    cmd += ["-Version", version]
    cmd += ["-Verbosity", "detailed"]
    cmd += ["-OutputDirectory", '"' + destination + '"']
    ret = RunCmd(cmd[0], " ".join(cmd[1:]))
    if ret != 0:
        return False
    else:
        return True

def GetReleaseForCommit(commit_hash:str):
    git_dir = os.path.dirname(SCRIPT_PATH)
    cmd_args = ["log", '--format="%h %D"', "-n 250"]
    return_buffer = StringIO()
    RunCmd("git", " ".join(cmd_args), workingdir=git_dir, outstream=return_buffer)
    return_buffer.seek(0)
    results = return_buffer.readlines()
    return_buffer.close()
    log_re = re.compile(r'release/(\d{6})')
    for log_item in results:
        commit = log_item[:11]
        branch = log_item[11:].strip()
        if len(branch) == 0:
            continue
        match = log_re.search(branch)
        if match:
            logging.info("Basing our release commit off of commit "+commit)
            return match.group(1)

    raise RuntimeError("We couldn't find the release branch that we correspond to")


def GetSubVersions(old_version: str, current_release: str, curr_hashes: dict, old_hashes: dict):
    year, month, major, minor = old_version.split(".")
    curr_year = int(current_release[0:4])
    curr_mon = int(current_release[4:])
    differences = []
    same = []
    for repo in curr_hashes:
        if repo not in old_hashes:
            logging.warning("Skipping comparing "+repo)
            continue
        if curr_hashes[repo] != old_hashes[repo]:
            differences.append(repo)
        else:
            same.append(repo)
    if curr_year != int(year) or curr_mon != int(month):
        major = 1
        minor = 1
    elif "OPENSSL" in differences or "MU_TIANO" in differences:
        major = int(major) + 1
        minor = 1
    elif len(differences) > 0:
        minor = int(minor) + 1
    elif len(same) == 0: # if don't share any of the same repos as the old version
        minor = int(minor) + 1
    return "{}.{}".format(major, minor)


def GetNextVersion():
    # first get the last version
    old_version = GetLatestNugetVersion(PACKAGE_NAME)  # TODO: get the source from the JSON file that configures this
    # Get a temporary folder to download the nuget package into
    temp_nuget_path = tempfile.mkdtemp()
    # Download the Nuget Package
    DownloadNugetPackageVersion(PACKAGE_NAME, old_version, temp_nuget_path)
    # Unpack and read the previous release notes, skipping the header, also get hashes
    old_notes, old_hashes = GetOldReleaseNotesAndHashes(os.path.join(temp_nuget_path, PACKAGE_NAME, PACKAGE_NAME, RELEASE_NOTES_FILENAME))
    # Get the current hashes of open ssl and ourself
    curr_hashes = GetCommitHashes(WORKSPACE_PATH)
    # Figure out what release branch we are in
    print(curr_hashes)
    current_release = GetReleaseForCommit(curr_hashes["MU_BASECORE"])
    # Put that as the first two pieces of our version
    new_version = current_release[0:4]+"."+current_release[4:]+"."
    # Calculate the newest version
    new_version += GetSubVersions(old_version, current_release, curr_hashes, old_hashes)
    if new_version == old_version:
        raise RuntimeError("We are unable to republish the same version that was published last")
    # Create the release note from this branch, currently the commit message on the head?
    release_note = GetReleaseNote()
    # Create our release notes, appending the old ones
    CreateReleaseNotes(release_note, new_version, old_notes, curr_hashes)
    # Clean up the temporary nuget file?
    logging.critical("Creating new version:"+new_version)
    return new_version

def PublishNuget():
    # otherwise do the upload
    logging.critical("PUBLISHING TO NUGET")
    # get the root directory of mu_basecore
    scriptDir = SCRIPT_PATH
    rootDir = WORKSPACE_PATH
    build_dir = os.path.join(rootDir, "Build")
    API_KEY = GetAPIKey()
    DUMP_VERSION = ShouldDumpVersion()

    if (DUMP_VERSION):
        print("##vso[task.setvariable variable=NugetPackageVersion;isOutput=true]"+VERSION)

    config_file = "SharedNetworking.config.json"

    if API_KEY is not None:
        logging.info("Will attempt to publish as well")
        params = "--Operation PackAndPush --ConfigFilePath {0} --Version {1} --InputFolderPath {2}  --ApiKey {3}".format(config_file, VERSION, output_dir, API_KEY)
    else:
        params = "--Operation Pack --ConfigFilePath {0} --Version {1} --InputFolderPath {2} --OutputFolderPath {3}".format(config_file, VERSION, output_dir, build_dir)
    # TODO: change this from a runcmd to directly invoking nuget publishing
    ret = UtilityFunctions.RunCmd("nuget-publish", params, capture=True, workingdir=scriptDir)
    if ret == 0:
        logging.critical("Finished packaging/publishing Nuget version {0}".format(VERSION))
    else:
        logging.error("Unable to pack/publish nuget package")
        return False

    return True

    params = "--Operation PackAndPush --ConfigFilePath SharedNetworking.config.json --Version {0} --InputFolderPath {1}  --ApiKey {2}".format(VERSION, output_dir, API_KEY)
    ret = UtilityFunctions.RunPythonScript(NugetFilePath, params, capture=True, workingdir=scriptDir)

    if ret == 0:
        logging.critical("Nuget Finished")
        return True
    else:
        logging.error("Error happened with Nuget")
        return False


def CollectNuget():
    # get the root directory of mu_basecore
    scriptDir = SCRIPT_PATH
    rootDir = WORKSPACE_PATH
    # move the EFI's we generated to a folder to upload
    logging.info("Running NugetPackager")
    output_dir = os.path.join(rootDir, "Build", ".NugetOutput")
    shared_networking_build_dir = os.path.realpath(os.path.join(rootDir, "Build", "SharedNetworkPkg"))

    if not os.path.exists(shared_networking_build_dir):
        logging.error("We were unable to find the build directory, skipping collecting Nuget build files")
        return False


    # copy the md file
    CopyFile(scriptDir, output_dir, "SharedNetworking.md")
    CopyFile(scriptDir, output_dir, "release_notes.md")

    list_to_get = ["Tls*", "DnsDxe*", "Http*","Ip*","Tcp*","Udp*","Dhcp*","Mtft*"]
    ending_list = [".efi", ".depex", ".pdb"]

    shared_networking_build_dir_offset = len(shared_networking_build_dir) + 1


    for list_item in list_to_get:
        for item_ending in ending_list:
            item = list_item + item_ending
            logging.info("Searching {0}".format(item))
            item_search = os.path.join(shared_networking_build_dir, "**", item)
            for found_item in glob.iglob(item_search, recursive=True):
                MoveArchTargetSpecificFile(found_item, shared_networking_build_dir_offset, output_dir)


    target = GetBuildTarget()
    arch = GetBuildArch()

    build_dir_txt_search = os.path.join(shared_networking_build_dir, "**", "BUILD_REPORT.txt")
    for txt in glob.iglob(build_dir_txt_search, recursive=True):
        file_name = os.path.basename(txt)
        srcDir = os.path.dirname(txt)
        shutil.copyfile(os.path.join(srcDir, file_name), os.path.join(output_dir, "{}_{}_{}".format(target, arch, file_name)))

    fv_search = os.path.join(shared_networking_build_dir, target+"*","FV", "FVDXE*")
    dest_path = os.path.join(output_dir, target, arch)
    # copy the FV files
    for found_item in glob.iglob(fv_search, recursive=True):
        src_dir = os.path.dirname(found_item)
        fv_name = os.path.basename(found_item)
        logging.info("Copying {} to {}".format(found_item, dest_path))
        CopyFile(src_dir, dest_path, fv_name)

    return True


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
        #logging.warning("Skipping {0}: {1} from {2}".format(binary_name, dest_filepath, binary))
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
        global VERSION
        # we have to checkpoint our environment since we have to make target non-overridable and ShellEnvironment is a singleton
        if GetBuildOver():
            return 1

        if VERSION is None:
            VERSION = GetNextVersion()

        # we have to set the build id to generate the build report since we have an FV
        self.env.SetValue("BLD_*_BUILDID_STRING", VERSION, "Current Version")
        self.shell_checkpoint = self.env.internal_shell_env.checkpoint()
        self.env.SetValue("PRODUCT_NAME", "SharedNetworking", "Platform Hardcoded")
        self.env.SetValue("ACTIVE_PLATFORM", "NetworkPkg/SharedNetworkPkg.dsc", "Platform Hardcoded")
        self.env.SetValue("TARGET_ARCH", GetBuildArch(), "Platform Hardcoded")
        self.env.SetValue("TARGET", GetBuildTarget(), "Platform Hardcoded")

        self.env.SetValue("BUILDREPORTING", "TRUE", "Platform Hardcoded")
        self.env.SetValue("BUILDREPORT_TYPES", 'PCD DEPEX LIBRARY BUILD_FLAGS', "Platform Hardcoded")
        # We need to use flash image as it runs after post Build to restore the shell environment
        self.FlashImage = True  # we need to flash the image to restore the checkpoint


        return 0

    def PlatformPreBuild(self):
        if self.env.GetValue("TARGET") != GetBuildTarget():
            logging.warning("We weren't able to set the target. It should be " + GetBuildTarget())
            return 1
        if self.env.GetValue("TARGET_ARCH") != GetBuildArch():
            logging.warning("We weren't able to set the target_arch. It should be " + GetBuildArch())
            return 1
        logging.critical("Building {} for {}".format(self.env.GetValue("TARGET"), self.env.GetValue("TARGET_ARCH")))
        logging.critical("Building {} for {}".format(GetBuildArch(), GetBuildTarget()))

        return 0

    def PlatformPostBuild(self):
        global BUILD_INDEX

        if self.env.GetValue("TARGET_ARCH") != GetBuildArch():
            logging.warning("We weren't able to set the target_arch. It should be " + GetBuildArch())
            return 1

        if not CollectNuget():
            return 1

        BUILD_INDEX += 1
        if GetBuildOver() and PublishNuget():
            return 1

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

    args = [sys.argv[0]]
    parser = argparse.ArgumentParser(description='Grab API Key')
    parser.add_argument('--api-key', dest="api_key", help='API key for NuGet')
    parser.add_argument('--dump-version', '--dumpversion', '--dump_version', dest="dump_version", action='store_true', default=False, help='whether or not to dump the version onto the command-line')
    parsed_args, remaining_args = parser.parse_known_args()  # this strips the first argument off
    if remaining_args is not None:  # any arguments that remain need to be tacked on
        args.extend(remaining_args)  # tack them on after the first argument

    SetAPIKey(parsed_args.api_key)  # Set the API ley
    SHOULD_DUMP_VERSION = parsed_args.dump_version  # set the dump versions
    try:
        from MuEnvironment import CommonBuildEntry
    except ImportError:
        print("Running Python version {0} from {1}".format(sys.version, sys.executable))
        raise RuntimeError("Please run \"python -m pip install --upgrade mu_build\".\nContact Microsoft Project Mu team if you run into any problems.")

    doing_an_update = False
    if "--update" in (" ".join(sys.argv)).lower():
        print("Doing an update")  # logging isn't setup yet
        doing_an_update = True

    finished_building = False
    while not finished_building:
        sys.argv = args  # restore our arguments
        # Now that we have access to the entry, hand off to the common code.
        try:
            CommonBuildEntry.build_entry(SCRIPT_PATH, WORKSPACE_PATH, REQUIRED_REPOS, PROJECT_SCOPE, MODULE_PKGS, MODULE_PKG_PATHS, worker_module='DriverBuilder')
        except SystemExit as e:  # catch the sys.exit call from uefibuild
            if e.code != 0:
                finished_building = True
                print("Exiting build")
            else:
                print("Success")
        finally:
            if doing_an_update:
                print("Exiting because we're updating")
                finished_building = True

        # this is hacky to close logging and restart it back to a default state
        logging.disable(logging.NOTSET)
        logging.shutdown()
        reload(logging)  # otherwise we get errors trying to talk to closed handlers