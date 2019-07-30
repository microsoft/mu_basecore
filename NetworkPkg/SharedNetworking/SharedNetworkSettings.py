##
# Script to Build Shared Crypto Driver
# Copyright Microsoft Corporation, 2019
#
# This is to build the SharedNetworking binaries for NuGet publishing
##
import os
from edk2toolext.environment import shell_environment
import logging
import shutil
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toolext.invocables.edk2_ci_setup import CiSetupSettingsManager
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toollib.utility_functions import RunCmd
from edk2toollib.utility_functions import RunPythonScript
from edk2toolext.environment.extdeptypes.nuget_dependency import NugetDependency
import glob
from io import StringIO
import re
import tempfile
try:
    from DriverBuilder import BinaryBuildSettingsManager
except Exception:
    class BinaryBuildSettingsManager:
        def __init__():
            raise RuntimeError("You shouldn't be including this")
    pass
from edk2toollib.utility_functions import GetHostInfo

#
# ==========================================================================
# PLATFORM BUILD ENVIRONMENT CONFIGURATION
#


def _CopyFile(srcDir, destDir, file_name, new_name=None):
    if new_name is None:
        new_name = file_name
    shutil.copyfile(os.path.join(srcDir, file_name),
                    os.path.join(destDir, new_name))


def _MoveArchTargetSpecificFile(binary, offset, output_dir):
    binary_path = binary[offset:]
    binary_name = os.path.basename(binary)
    binary_folder = os.path.dirname(binary)
    arch = _GetArchitecture(binary_path)
    target = _GetTarget(binary_path)
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
    _CopyFile(binary_folder, dest_path, binary_name)


def _GetArchitecture(path: str):
    path = os.path.normpath(path)
    path_parts = path.split(os.path.sep)
    return path_parts[1]


def _GetTarget(path: str):
    path = os.path.normpath(path)
    path_parts = path.split(os.path.sep)
    target = path_parts[0]

    if "_" in target:
        return target.split("_")[0]
    return None


def _GetCommitHashes(root_dir: os.PathLike):
    # Recursively looks at every .git and gets the commit from there
    search_path = os.path.join(root_dir, "**", ".git")
    logging.info(f"Searching {search_path} for git repos")
    search = glob.iglob(search_path, recursive=True)
    found_repos = {}
    cmd_args = "rev-parse HEAD"
    for git_path in search:
        git_path_dir = os.path.dirname(git_path)
        if git_path_dir == root_dir:
            git_repo_name = "MU_BASECORE"
        else:
            git_repo_name = os.path.relpath(git_path_dir, root_dir)
        git_repo_name = git_repo_name.upper()
        if git_repo_name in found_repos:
            raise RuntimeError(
                f"we've already found this repo before {git_repo_name} {git_path_dir}")
        # read the git hash for this repo
        return_buffer = StringIO()
        RunCmd("git", cmd_args, workingdir=git_path_dir, outstream=return_buffer)
        commit_hash = return_buffer.getvalue().strip()
        return_buffer.close()
        found_repos[git_repo_name] = commit_hash
    return found_repos

# Functions for getting the next nuget version


def _GetLatestNugetVersion(package_name, source=None):
    cmd = NugetDependency.GetNugetCmd()
    cmd += ["list"]
    cmd += [package_name]
    if source is not None:
        cmd += ["-Source", source]
    return_buffer = StringIO()
    if (RunCmd(cmd[0], " ".join(cmd[1:]), outstream=return_buffer) == 0):
        # Seek to the beginning of the output buffer and capture the output.
        return_buffer.seek(0)
        return_string = return_buffer.readlines()
        return_buffer.close()
        for line in return_string:
            line = line.strip()
            if line.startswith(package_name):
                return line.replace(package_name, "").strip()
    else:
        return "0.0.0.0"


def _GetReleaseNote():
    cmd = "log --format=%B -n 1 HEAD"
    return_buffer = StringIO()
    if (RunCmd("git", cmd, outstream=return_buffer) == 0):
        # Seek to the beginning of the output buffer and capture the output.
        return_buffer.seek(0)
        # read the first 155 characters and replace the
        return_string = return_buffer.read(155).replace("\n", " ")
        return_buffer.close()
        # TODO: figure out if there was more input and append a ... if needed
        return return_string.strip()
    else:
        raise RuntimeError("Unable to read release notes")


def _GetSubVersions(old_version: str, current_release: str, curr_hashes: dict, old_hashes: dict):
    year, month, major, minor = old_version.split(".")
    if len(current_release) < 4:
        raise RuntimeError(f"Invalid version of {current_release}")
    curr_year = int(current_release[0:4])
    curr_mon = int(current_release[4:])
    differences = []
    same = []
    for repo in curr_hashes:
        if repo not in old_hashes:
            logging.warning("Skipping comparing " + repo)
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
    elif len(same) == 0:  # if don't share any of the same repos as the old version
        minor = int(minor) + 1
    return "{}.{}".format(major, minor)


class SettingsManager(UpdateSettingsManager, CiSetupSettingsManager, BinaryBuildSettingsManager):
    def __init__(self):
        SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))

        WORKSPACE_PATH = os.path.dirname(os.path.dirname(SCRIPT_PATH))
        REQUIRED_REPOS = ['Common/MU_TIANO',
                          'Common/MU_PLUS',
                          "Silicon/Arm/MU_TIANO"]  # todo fix this

        MODULE_PKG_PATHS = os.pathsep.join(os.path.join(
            WORKSPACE_PATH, pkg_name) for pkg_name in REQUIRED_REPOS)

        self.OUTPUT_DIR = os.path.join(WORKSPACE_PATH, "Build", ".NugetOutput")
        self.ws = WORKSPACE_PATH
        self.pp = MODULE_PKG_PATHS
        self.rr = REQUIRED_REPOS
        self.sp = SCRIPT_PATH
        self.api_key = None
        self.should_dump_version = False
        self.release_notes_filename = "release_notes.md"
        self.nuget_package_name = "Mu-SharedNetworking"
        self.nuget_version = None
        pass

    def _CreateReleaseNotes(self, note: str, new_version: str, old_release_notes: list, hashes: dict):
        scriptDir = self.sp

        release_notes_path = os.path.join(
            scriptDir, self.release_notes_filename)
        current_notes_file = open(release_notes_path, "r")
        notes = current_notes_file.readlines()
        current_notes_file.close()

        notes += ["\n"]
        notes += ["## {}- {}\n".format(new_version, note), "\n"]
        for repo in hashes:
            repo_hash = hashes[repo]
            notes += ["{}@{}\n".format(repo, repo_hash)]
        notes += ["\n"]

        # write our notes out to the file
        current_notes_file = open(release_notes_path, "w")
        current_notes_file.writelines(notes)
        current_notes_file.writelines(old_release_notes)
        current_notes_file.close()

    def _GetOldReleaseNotesAndHashes(self, notes_path: os.PathLike):
        # Read in the text file
        if not os.path.isfile(notes_path):
            raise FileNotFoundError("Unable to find the old release notes")
        old_notes_file = open(notes_path, "r")
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

    def _DownloadNugetPackageVersion(self, package_name: str, version: str, destination: os.PathLike, source=None):
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

    def _GetReleaseForCommit(self, commit_hash: str, n: int = 100):
        if n > 2000:
            logging.error("We couldn't find the release branch that we correspond to")
            return "0.0.0.0"
        git_dir = os.path.dirname(self.sp)
        cmd_args = ["log", '--format="%h %D"', "-n " + str(n)]
        return_buffer = StringIO()
        RunCmd("git", " ".join(cmd_args),
               workingdir=git_dir, outstream=return_buffer)
        return_buffer.seek(0)
        results = return_buffer.readlines()
        return_buffer.close()
        log_re = re.compile(r'(release|dev)/(\d{6})')
        for log_item in results:
            commit = log_item[:11]
            branch = log_item[11:].strip()
            if len(branch) == 0:
                continue
            match = log_re.search(branch)
            if match:
                logging.info(
                    "Basing our release commit off of commit " + commit)
                return match.group(2)
        return self._GetReleaseForCommit(commit_hash, n * 2)

    def _GetNextVersion(self, force_version=None):
        # first get the last version
        # TODO: get the source from the JSON file that configures this
        old_version_raw = _GetLatestNugetVersion(self.nuget_package_name)
        old_version = NugetDependency.normalize_version(old_version_raw)
        logging.info(f"Found old version {old_version}")
        # Get a temporary folder to download the nuget package into
        temp_nuget_path = tempfile.mkdtemp()
        # Download the Nuget Package
        self._DownloadNugetPackageVersion(
            self.nuget_package_name, old_version, temp_nuget_path)
        # Unpack and read the previous release notes, skipping the header, also get hashes
        old_notes, old_hashes = self._GetOldReleaseNotesAndHashes(os.path.join(
            temp_nuget_path, self.nuget_package_name, self.nuget_package_name, self.release_notes_filename))
        # Get the current hashes of open ssl and ourself
        curr_hashes = _GetCommitHashes(self.ws)
        # Figure out what release branch we are in
        logging.info(curr_hashes)
        logging.info("OLD")
        logging.info(old_hashes)
        current_release = self._GetReleaseForCommit(curr_hashes["MU_BASECORE"])
        # Put that as the first two pieces of our version
        if force_version is not None:
            new_version = force_version
        else:
            new_version = current_release[0:4] + \
                "." + current_release[4:] + "."
            # Calculate the newest version
            new_version += _GetSubVersions(old_version,
                                           current_release, curr_hashes, old_hashes)
        # make sure to normalize the version
        new_version = NugetDependency.normalize_version(new_version)
        if new_version == old_version:
            raise RuntimeError(
                "We are unable to republish the same version that was published last")
        # Create the release note from this branch, currently the commit message on the head?
        release_note = _GetReleaseNote()
        # Create our release notes, appending the old ones
        self._CreateReleaseNotes(
            release_note, new_version, old_notes, curr_hashes)
        # Clean up the temporary nuget file?
        logging.critical("Creating new version:" + new_version)
        return new_version

    def GetActiveScopes(self):
        ''' get scope '''
        scopes = ("corebuild", "sharednetworking_build", "project_mu", 'edk2-build' )
        # if (GetHostInfo().os == "Linux"):
        #    scopes += ("gcc_aarch64_linux",)

        return scopes

    def _PublishNuget(self):
        # otherwise do the upload
        logging.critical("PUBLISHING TO NUGET")
        # get the root directory of mu_basecore
        scriptDir = self.sp
        rootDir = self.ws
        build_dir = os.path.join(rootDir, "Build")

        if (self.should_dump_version):
            print(
                "##vso[task.setvariable variable=NugetPackageVersion;isOutput=true]" + self.nuget_version)

        config_file = "SharedNetworking.config.json"

        if self.api_key is not None:
            logging.info("Will attempt to publish as well")
            params = "--Operation PackAndPush --ConfigFilePath {0} --Version {1} --InputFolderPath {2}  --ApiKey {3}".format(
                config_file, self.nuget_version, self.OUTPUT_DIR, self.api_key)
        else:
            params = "--Operation Pack --ConfigFilePath {0} --Version {1} --InputFolderPath {2} --OutputFolderPath {3}".format(
                config_file, self.nuget_version, self.OUTPUT_DIR, build_dir)
        # TODO: change this from a runcmd to directly invoking nuget publishing
        ret = RunCmd("nuget-publish", params,
                     capture=True, workingdir=scriptDir)
        if ret != 0:
            logging.error("Unable to pack/publish nuget package")
            return False
        logging.critical(
            "Finished packaging/publishing Nuget version {0}".format(self.nuget_version))
        return True

    def _CollectNuget(self):
        # get the root directory of mu_basecore
        scriptDir = self.sp
        rootDir = self.ws
        # move the EFI's we generated to a folder to upload
        logging.info("Running NugetPackager")
        output_dir = self.OUTPUT_DIR
        shared_networking_build_dir = os.path.realpath(
            os.path.join(rootDir, "Build", "SharedNetworkPkg"))

        if not os.path.exists(shared_networking_build_dir):
            logging.error(
                "We were unable to find the build directory, skipping collecting Nuget build files")
            return 1

        # copy the md file
        _CopyFile(scriptDir, output_dir, "SharedNetworking.md")
        _CopyFile(scriptDir, output_dir, "release_notes.md")

        list_to_get = ["Tls*", "DnsDxe*", "Http*", "Arp*", "IScsi*", "Mnp*", "Snp*",
                       "Vlan*", "VConfig*", "Ip*", "Tcp*", "Udp*", "Dhcp*", "Mtft*", "Arp*"]
        ending_list = [".efi", ".depex", ".pdb"]

        shared_networking_build_dir_offset = len(
            shared_networking_build_dir) + 1

        for list_item in list_to_get:
            for item_ending in ending_list:
                item = list_item + item_ending
                item_search = os.path.join(
                    shared_networking_build_dir, "**", self.target + "*", "**", item)
                logging.info(f"Searching for {item} = {item_search}")
                for found_item in glob.iglob(item_search, recursive=True):
                    _MoveArchTargetSpecificFile(
                        found_item, shared_networking_build_dir_offset, output_dir)

        build_dir_txt_search = os.path.join(
            shared_networking_build_dir, "**", "BUILD_REPORT.txt")
        for txt in glob.iglob(build_dir_txt_search, recursive=True):
            file_name = os.path.basename(txt)
            srcDir = os.path.dirname(txt)
            shutil.copyfile(os.path.join(srcDir, file_name), os.path.join(
                output_dir, "{}_{}_{}".format(self.target, self.arch, file_name)))

        fv_search = os.path.join(shared_networking_build_dir,
                                 self.target + "*", "FV", "FVDXE.Fv")
        dest_path = os.path.join(output_dir, self.target, self.arch)
        # copy the FV files
        version = self.nuget_version
        for found_item in glob.iglob(fv_search, recursive=True):
            src_dir = os.path.dirname(found_item)
            fv_name = os.path.basename(found_item)
            #fv_name_parts = os.path.splitext(fv_name)
            #fv_new_name = fv_name_parts[0] + "_" + version + fv_name_parts[1]
            logging.info("Copying {} to {}".format(found_item, dest_path))
            _CopyFile(src_dir, dest_path, fv_name)  # , fv_new_name)

        return 0

    def _GetOutputDir(self):
        return os.path.join(self.ws, "Build", ".NugetOutput")

    def PreFirstBuildHook(self):
        output_dir = self.OUTPUT_DIR
        try:
            if os.path.exists(output_dir):
                logging.warning(f"Deleting {output_dir}")
                shutil.rmtree(output_dir, ignore_errors=True)
            os.makedirs(output_dir)
        except:
            pass

        self.nuget_version = self._GetNextVersion(self.nuget_version)

        return 0

    def PostBuildHook(self, ret):
        if ret == 0:
            ret = self._CollectNuget()
        if ret != 0:
            logging.error("Error occured in post build hook")
            raise RuntimeError("Error occured in post build hook")
        return ret

    def PostFinalBuildHook(self, ret):
        if ret != 0:
            logging.error(
                "There was failure along the way aborting NUGET publish")
            return
        self._PublishNuget()

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return self.ws

    def GetModulePkgsPath(self):
        ''' get module packages path '''
        return self.pp

    def GetRequiredRepos(self):
        ''' get required repos '''
        return self.rr

    def GetName(self):
        return "SharedNetworking"

    def GetPackagesSupported(self):
        return "NetworkPkg"

    def GetArchitecturesSupported(self):
        return ["X64", "IA32", "AARCH64"]

    def GetTargetsSupported(self):
        return ["DEBUG", "RELEASE"]

    def GetConfigurations(self):
        TARGETS = self.GetTargetsSupported()
        ARCHS = self.GetArchitecturesSupported()
        # combine options together
        for target in TARGETS:
            for arch in ARCHS:
                self.target = target
                self.arch = arch
                yield({"TARGET": target, "TARGET_ARCH": arch, "ACTIVE_PLATFORM": "NetworkPkg/SharedNetworking/SharedNetworkPkg.dsc"})

    def GetDependencies(self):
        return [
            {
                "Path": "Silicon/Arm/MU_TIANO",
                "Url": "https://github.com/Microsoft/mu_silicon_arm_tiano.git",
                "Branch": "release/202002"
            },
            {
                "Path": "Common/MU_TIANO",
                "Url": "https://github.com/Microsoft/mu_tiano_plus.git",
                "Branch": "release/202002"
            },
            {
                "Path": "Common/MU_PLUS",
                "Url": "https://github.com/Microsoft/mu_plus.git",
                "Branch": "release/202002"
            }
        ]

    def AddCommandLineOptions(self, parserObj):
        ''' Add command line options to the argparser '''
        # TODO  Add this parameter to adjust the scopes so we don't pull in the realtek ext dep
        parserObj.add_argument('-a' '--api_key', dest='api_key',
                               type=str, help='nuget api key used to upload the package')

        parserObj.add_argument('-d', '--dump_version', '--dump-version', dest='dump_version',
                               type=bool, default=False, help='Should I dump nuget information?')
        parserObj.add_argument("-nv", "--nuget_version", "--nuget-version", dest="nug_ver",
                               type=str, default=None, help="Nuget Version for package")

    def RetrieveCommandLineOptions(self, args):
        '''  Retrieve command line options from the argparser '''
        shell_environment.GetBuildVars().SetValue("TOOL_CHAIN_TAG", "VS2017", "Set default")
        if args.api_key is not None:
            self.api_key = args.api_key
            print("Using API KEY")
        self.nuget_version = args.nug_ver

        self.should_dump_version = args.dump_version


def main():
    import argparse
    import sys
    import os
    from edk2toolext.invocables.edk2_update import Edk2Update
    from edk2toolext.invocables.edk2_ci_setup import Edk2CiBuildSetup
    from edk2toolext.invocables.edk2_platform_build import Edk2PlatformBuild
    import DriverBuilder
    print("Invoking Stuart")
    print("     ) _     _")
    print("    ( (^)-~-(^)")
    print("__,-.\_( 0 0 )__,-.___")
    print("  'W'   \   /   'W'")
    print("         >o<")
    SCRIPT_PATH = os.path.relpath(__file__)
    parser = argparse.ArgumentParser(add_help=False)
    parse_group = parser.add_mutually_exclusive_group()
    parse_group.add_argument("--update", "--UPDATE",
                             action='store_true', help="Invokes stuart_update")
    parse_group.add_argument("--setup", "--SETUP",
                             action='store_true', help="Invokes stuart_setup")
    args, remaining = parser.parse_known_args()
    new_args = ["stuart", "-c", SCRIPT_PATH]
    new_args = new_args + remaining
    sys.argv = new_args
    if args.setup:
        print("Running stuart_ci_setup -c " + SCRIPT_PATH)
        Edk2CiBuildSetup().Invoke()
    elif args.update:
        print("Running stuart_update -c " + SCRIPT_PATH)
        Edk2Update().Invoke()
    else:
        print("Running DriverBuilder -c " + SCRIPT_PATH)
        DriverBuilder.Edk2BinaryBuild().Invoke()

if __name__ == "__main__":
    import SharedNetworkSettings
    SharedNetworkSettings.main()  # otherwise we're in __main__ context