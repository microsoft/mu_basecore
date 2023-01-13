# @file LineEndingCheck.py
#
# An edk2-pytool based plugin that checks line endings.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import glob
from io import StringIO
import logging
import os
import shutil
from pathlib import Path
from typing import Any, Callable, Dict, List, Tuple

from edk2toolext.environment.plugin_manager import PluginManager
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toolext.environment.plugintypes.uefi_helper_plugin import \
    HelperFunctions
from edk2toolext.environment.var_dict import VarDict
from edk2toollib.gitignore_parser import parse_gitignore_lines
from edk2toollib.log.junit_report_format import JunitReportTestCase
from edk2toollib.uefi.edk2.path_utilities import Edk2Path
from edk2toollib.utility_functions import RunCmd
from git import Repo


PLUGIN_NAME = "LineEndingCheck"

LINE_ENDINGS = [
    b'\r\n',
    b'\n\r',
    b'\n',
    b'\r'
]

ALLOWED_LINE_ENDING = b'\r\n'

#
# Based on a solution for binary file detection presented in
# https://stackoverflow.com/a/7392391.
#
_TEXT_CHARS = bytearray(
    {7, 8, 9, 10, 12, 13, 27} | set(range(0x20, 0x100)) - {0x7f})


def _is_binary_string(_bytes: bytes) -> bool:
    return bool(_bytes.translate(None, _TEXT_CHARS))


class LineEndingCheckBadLineEnding(Exception):
    pass

class LineEndingCheckGitIgnoreFileException(Exception):
    pass

class LineEndingCheck(ICiBuildPlugin):
    """
    A CiBuildPlugin that checks whether line endings are a certain format.

    By default, the plugin runs against all files in a package unless a
    specific file or file extension is excluded.

    Configuration options:
    "LineEndingCheck": {
        "IgnoreFiles": [],            # File patterns to ignore.
    }
    """

    def GetTestName(self, packagename: str, environment: VarDict) -> Tuple:
        """ Provide the testcase name and classname for use in reporting

            Args:
              packagename: String containing name of package to build.
              environment: The VarDict for the test to run in.

            Returns:
                A tuple containing the testcase name and the classname
                (testcasename, classname)
                testclassname: a descriptive string for the testcase can
                               include whitespace
                classname: Should be patterned <packagename>.<plugin>
                           .<optionally any unique condition>
        """
        return ("Check line endings in " + packagename, packagename +
                "." + PLUGIN_NAME)

    # Note: This function access git via the command line
    #
    #   function to check and warn if git config reports that 
    #   autocrlf is configured to TRUE
    def _check_autocrlf(self):
        r = Repo(".")
        try:
            result = r.config_reader().get_value("core", "autocrlf")
            if result:
                logging.warning(f"git config core.autocrlf is set to {result} "
                                f"recommended setting is false "
                                f"git config --global core.autocrlf false")
        except Exception:
            logging.warning(f"git config core.autocrlf is not set "
                            f"recommended setting is false "
                            f"git config --global core.autocrlf false")
        return

    # Note: This function currently accesses git via the git command to prevent
    #       introducing a new Python git module dependency in mu_basecore
    #       on gitpython.
    #
    #       After gitpython is adopted by edk2-pytool-extensions, this
    #       implementation can be updated to use the gitpython interface.
    def _get_git_ignored_paths(self) -> List[Path]:
        """"
        Gets paths ignored by git.

        Returns:
            List[str]: A list of file absolute path strings to all files
            ignored in this git repository.

            If git is not found, an empty list will be returned.
        """
        if not shutil.which("git"):
            logging.warn(
                "Git is not found on this system. Git submodule paths will "
                "not be considered.")
            return []

        outstream_buffer = StringIO()
        exit_code = RunCmd("git", "ls-files --other",
                           workingdir=self._abs_workspace_path,
                           outstream=outstream_buffer,
                           logging_level=logging.NOTSET)
        if (exit_code != 0):
            raise LineEndingCheckGitIgnoreFileException(
                f"An error occurred reading git ignore settings. This will "
                f"prevent LineEndingCheck from running against the expected "
                f"set of files.")

        # Note: This will potentially be a large list, but at least sorted
        rel_paths = outstream_buffer.getvalue().strip().splitlines()
        abs_paths = []
        for path in rel_paths:
            abs_paths.append(Path(
                os.path.normpath(os.path.join(self._abs_workspace_path, path))))
        return abs_paths

    # Note: This function currently accesses git via the git command to prevent
    #       introducing a new Python git module dependency in mu_basecore
    #       on gitpython.
    #
    #       After gitpython is adopted by edk2-pytool-extensions, this
    #       implementation can be updated to use the gitpython interface.
    def _get_git_submodule_paths(self) -> List[Path]:
        """
        Gets submodule paths recognized by git.

        Returns:
            List[str]: A list of directory absolute path strings to the root
            of each submodule in the workspace repository.

            If git is not found, an empty list will be returned.
        """
        if not shutil.which("git"):
            logging.warn(
                "Git is not found on this system. Git submodule paths will "
                "not be considered.")
            return []

        if os.path.isfile(os.path.join(self._abs_workspace_path, ".gitmodules")):
            logging.info(
                ".gitmodules file found. Excluding submodules in "
                "LineEndingCheck.")

            outstream_buffer = StringIO()
            exit_code = RunCmd("git",
                            "config --file .gitmodules --get-regexp path",
                            workingdir=self._abs_workspace_path,
                            outstream=outstream_buffer,
                            logging_level=logging.NOTSET)
            if (exit_code != 0):
                raise LineEndingCheckGitIgnoreFileException(
                    f".gitmodule file detected but an error occurred reading "
                    f"the file. Cannot proceed with unknown submodule paths.")

            submodule_paths = []
            for line in outstream_buffer.getvalue().strip().splitlines():
                submodule_paths.append(Path(
                    os.path.normpath(os.path.join(self._abs_workspace_path, line.split()[1]))))

            return submodule_paths
        else:
            return []

    def _get_files_ignored_in_config(self,
                                     pkg_config: Dict[str, List[str]],
                                     base_dir: str) -> Callable[[str], bool]:
        """"
        Returns a function that returns true if a given file string path is
        ignored in the plugin configuration file and false otherwise.

        Args:
            pkg_config: Dictionary with the package configuration
            base_dir: Base directory of the package

        Returns:
            Callable[[None], None]: A test case function.
        """
        ignored_files = []
        if "IgnoreFiles" in pkg_config:
            ignored_files = pkg_config["IgnoreFiles"]

        # Pass "Package configuration file" as the source file path since
        # the actual configuration file name is unknown to this plugin and
        # this provides a generic description of the file that provided
        # the ignore file content.
        #
        # This information is only used for reporting (not used here) and
        # the ignore lines are being passed directly as they are given to
        # this plugin.
        return parse_gitignore_lines(ignored_files,
                                     "Package configuration file",
                                     base_dir)

    def RunBuildPlugin(self, package_rel_path: str, edk2_path: Edk2Path,
                       package_config: Dict[str, List[str]],
                       environment_config: Any,
                       plugin_manager: PluginManager,
                       plugin_manager_helper: HelperFunctions,
                       tc: JunitReportTestCase, output_stream=None) -> int:
        """
        External function of plugin. This function is used to perform the task
        of the CiBuild Plugin.

        Args:
          - package_rel_path: edk2 workspace relative path to the package
          - edk2_path: Edk2Path object with workspace and packages paths
          - package_config: Dictionary with the package configuration
          - environment_config: Environment configuration
          - plugin_manager: Plugin Manager Instance
          - plugin_manager_helper: Plugin Manager Helper Instance
          - tc: JUnit test case
          - output_stream: The StringIO output stream from this plugin
            (logging)

        Returns:
          >0 : Number of errors found
          0  : Ran successfully
          -1 : Skipped due to a missing pre-requisite
        """
        self._check_autocrlf()
        self._abs_workspace_path = \
            edk2_path.GetAbsolutePathOnThisSystemFromEdk2RelativePath('.')
        self._abs_pkg_path = \
            edk2_path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                        package_rel_path)

        if self._abs_pkg_path is None:
            tc.SetSkipped()
            tc.LogStdError(f"Package folder not found {self._abs_pkg_path}")
            return 0

        all_files = [Path(n) for n in glob.glob(
                        os.path.join(self._abs_pkg_path, '**/*.*'),
                        recursive=True)]
        ignored_files = list(filter(
                            self._get_files_ignored_in_config(
                                package_config, self._abs_pkg_path), all_files))
        ignored_files = [Path(f) for f in ignored_files]

        all_files = list(set(all_files) - set(ignored_files))
        if not all_files:
            tc.SetSuccess()
            return 0

        all_files_before_git_removal = set(all_files)
        git_ignored_paths = set(self._get_git_ignored_paths() + self._get_git_submodule_paths())
        all_files = list(all_files_before_git_removal - git_ignored_paths)
        git_ignored_paths = git_ignored_paths - (all_files_before_git_removal - set(all_files))
        if not all_files:
            tc.SetSuccess()
            return 0

        git_ignored_paths = {p for p in git_ignored_paths if p.is_dir()}

        ignored_files = []
        for file in all_files:
            for ignored_path in git_ignored_paths:
                if Path(file).is_relative_to(ignored_path):
                    ignored_files.append(file)
                    break

        all_files = list(set(all_files) - set(ignored_files))
        if not all_files:
            tc.SetSuccess()
            return 0

        file_count = 0
        line_ending_count = dict.fromkeys(LINE_ENDINGS, 0)

        for file in all_files:
            if file.is_dir():
                continue
            with open(file.resolve(), 'rb') as fb:
                if not fb.readable() or _is_binary_string(fb.read(1024)):
                    continue
                fb.seek(0)

                for lineno, line in enumerate(fb):
                    try:
                        for e in LINE_ENDINGS:
                            if line.endswith(e):
                                line_ending_count[e] += 1

                                if e is not ALLOWED_LINE_ENDING:
                                    file_path = file.relative_to(
                                        Path(self._abs_workspace_path)).as_posix()
                                    file_count += 1

                                    tc.LogStdError(
                                        f"Line ending on Line {lineno} in "
                                        f"{file_path} is not allowed.\nLine "
                                        f"ending is {e} and should be "
                                        f"{ALLOWED_LINE_ENDING}.")
                                    logging.error(
                                        f"Line ending on Line {lineno} in "
                                        f"{file_path} is not allowed.\nLine "
                                        f"ending is {e} and should be "
                                        f"{ALLOWED_LINE_ENDING}.")
                                    raise LineEndingCheckBadLineEnding
                                break
                    except LineEndingCheckBadLineEnding:
                        break

        del line_ending_count[ALLOWED_LINE_ENDING]

        if any(line_ending_count.values()):
            tc.SetFailed(
                f"{PLUGIN_NAME} failed due to {file_count} files with "
                f"incorrect line endings.",
                "CHECK_FAILED")
        else:
            tc.SetSuccess()

        return sum(line_ending_count.values())
