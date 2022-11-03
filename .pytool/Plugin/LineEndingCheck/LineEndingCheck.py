# @file LineEndingCheck.py
#
# An edk2-pytool based plugin that checks line endings.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import glob
import logging
import os
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

PLUGIN_NAME = "LineEndingCheck"

LINE_ENDINGS = [
    b'\r\n',
    b'\n\r',
    b'\n',
    b'\r'
]

ALLOWED_LINE_ENDING = b'\r\n'


class LineEndingCheckBadLineEnding(Exception):
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

        abs_pkg_path = \
            edk2_path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                        package_rel_path)

        if abs_pkg_path is None:
            tc.SetSkipped()
            tc.LogStdError(f"Package folder not found {abs_pkg_path}")
            return 0

        all_files = [n for n in glob.glob(os.path.join(abs_pkg_path, '*.*'),
                     recursive=True)]

        ignored_files = list(filter(
                            self._get_files_ignored_in_config(
                                package_config, abs_pkg_path), all_files))

        for file in ignored_files:
            if file in all_files:
                logging.info(f"  File ignored in plugin config file: "
                             f"{Path(file).name}")
                all_files.remove(file)

        file_count = 0
        line_ending_count = dict.fromkeys(LINE_ENDINGS, 0)

        for file in all_files:
            with open(file, 'rb') as fb:
                for lineno, line in enumerate(fb):
                    try:
                        for e in LINE_ENDINGS:
                            if line.endswith(e):
                                line_ending_count[e] += 1

                                if e is not ALLOWED_LINE_ENDING:
                                    file_name = Path(file).name
                                    file_count += 1

                                    tc.LogStdError(
                                        f"Line ending on Line {lineno} in "
                                        f"{file_name} is not allowed.\nLine "
                                        f"ending is {e} and should be "
                                        f"{ALLOWED_LINE_ENDING}.")
                                    logging.error(
                                        f"Line ending on Line {lineno} in "
                                        f"{file_name} is not allowed.\nLine "
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
