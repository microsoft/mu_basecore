# @file MarkdownLintCheck.py
#
# An edk2-pytool based plugin wrapper for markdownlint
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import logging
import json
import yaml
from io import StringIO
import os
from typing import List
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toollib.utility_functions import RunCmd
from edk2toolext.environment.var_dict import VarDict
from edk2toolext.environment import version_aggregator


class MarkdownLintCheck(ICiBuildPlugin):
    """
    A CiBuildPlugin that uses the markdownlint-cli node module to scan the files
    from the package being tested for linter errors.  The plugin contains
    the configuration file for global rules.

    Configuration options:
    "MarkdownLintCheck": {
        "AuditOnly": False,          # If True, log all errors and then mark as skipped
        "IgnoreFiles": []            # package root relative file, folder, or glob pattern to ignore
    }
    """

    def GetTestName(self, packagename: str, environment: VarDict) -> tuple:
        """ Provide the testcase name and classname for use in reporting

            Args:
              packagename: string containing name of package to build
              environment: The VarDict for the test to run in
            Returns:
                a tuple containing the testcase name and the classname
                (testcasename, classname)
                testclassname: a descriptive string for the testcase can include whitespace
                classname: should be patterned <packagename>.<plugin>.<optionally any unique condition>
        """
        return ("Lint Markdown files in " + packagename, packagename + ".markdownlint")

    ##
    # External function of plugin.  This function is used to perform the task of the CiBuild Plugin
    #
    #   - package is the edk2 path to package.  This means workspace/packagepath relative.
    #   - edk2path object configured with workspace and packages path
    #   - PkgConfig Object (dict) for the pkg
    #   - EnvConfig Object
    #   - Plugin Manager Instance
    #   - Plugin Helper Obj Instance
    #   - Junit Logger
    #   - output_stream the StringIO output stream from this plugin via logging

    def RunBuildPlugin(self, packagename, Edk2pathObj, pkgconfig, environment, PLM, PLMHelper, tc, output_stream=None):
        Errors = []

        abs_pkg_path = Edk2pathObj.GetAbsolutePathOnThisSytemFromEdk2RelativePath(
            packagename)

        if abs_pkg_path is None:
            tc.SetSkipped()
            tc.LogStdError("No package {0}".format(packagename))
            return -1

        # check for node
        return_buffer = StringIO()
        ret = RunCmd("node", "--version", outstream=return_buffer)
        if (ret != 0):
            tc.SetSkipped()
            tc.LogStdError("NodeJs not installed. Test can't run")
            logging.warning("NodeJs not installed. Test can't run")
            return -1
        node_version = return_buffer.getvalue().strip()  # format vXX.XX.XX
        tc.LogStdOut(f"Node version: {node_version}")

        # Check for markdownlint-cli
        return_buffer = StringIO()
        ret = RunCmd("markdownlint", "--version", outstream=return_buffer)
        if (ret != 0):
            tc.SetSkipped()
            tc.LogStdError("markdownlint not installed.  Test can't run")
            logging.warning("markdownlint not installed.  Test can't run")
            return -1
        mdl_version = return_buffer.getvalue().strip()  # format XX.XX.XX
        tc.LogStdOut(f"MarkdownLint version: {mdl_version}")
        version_aggregator.GetVersionAggregator().ReportVersion(
            "MarkDownLint", mdl_version, version_aggregator.VersionTypes.INFO)

        # Get relative path for the root of package to use with ignore and path parameters
        relpath = os.path.relpath(abs_pkg_path)

        #
        # check for any package specific ignore patterns defined by package config
        #
        Ignores = []
        if("IgnoreFiles" in pkgconfig):
            for i in pkgconfig["IgnoreFiles"]:
                Ignores.append(f"{relpath}/{i}")

        #
        # Make the path string to check
        #
        path_to_check = f"{relpath}/**/*.md"

        # get path to config file
        mydir = os.path.dirname(os.path.abspath(__file__))
        config_file_path = os.path.join(mydir, ".markdownlint.yaml")


        results = self._check_markdown(path_to_check, config_file_path, Ignores)
        for r in results:
            tc.LogStdError(r.strip())

        # add result to test case
        overall_status = len(results)
        if overall_status != 0:
            if "AuditOnly" in pkgconfig and pkgconfig["AuditOnly"]:
                # set as skipped if AuditOnly
                tc.SetSkipped()
                return -1
            else:
                tc.SetFailed("Markdown Lint Check {0} Failed.  Errors {1}".format(
                    packagename, overall_status), "CHECK_FAILED")
        else:
            tc.SetSuccess()
        return overall_status

    def _check_markdown(self, rel_file_to_check: os.PathLike, abs_config_file_to_use: os.PathLike, Ignores: List) -> []:
        output = StringIO()
        param = f"--config {abs_config_file_to_use}"
        for a in Ignores:
            param += f' --ignore "{a}"'
        param += f' "{rel_file_to_check}"'

        ret = RunCmd(
            "markdownlint",  param, outstream=output)
        if ret == 0:
            return []
        else:
            return output.getvalue().strip().splitlines()
