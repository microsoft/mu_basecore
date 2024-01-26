# @file HostBasedUnitTestRunner.py
# Plugin to located any host-based unit tests in the output directory and execute them.
##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
import os
import logging
import glob
import stat
import xml.etree.ElementTree
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toolext import edk2_logging
import edk2toollib.windows.locate_tools as locate_tools
from edk2toolext.environment import shell_environment
from edk2toollib.utility_functions import RunCmd
from edk2toollib.utility_functions import GetHostInfo
from edk2toollib.database import Edk2DB  # MU_CHANGE - reformat coverage data
from edk2toollib.database.tables import EnvironmentTable, SourceTable, PackageTable, InfTable  # MU_CHANGE - reformat coverage data
from textwrap import dedent


class HostBasedUnitTestRunner(IUefiBuildPlugin):

    def do_pre_build(self, thebuilder):
        '''
        Run Prebuild
        '''

        return 0

    def do_post_build(self, thebuilder):
        '''
        After a build, will automatically locate and run all host-based unit tests. Logs any
        failures with Warning severity and will return a count of the failures as the return code.

        EXPECTS:
        - Build Var 'CI_BUILD_TYPE' - If not set to 'host_unit_test', will not do anything.

        UPDATES:
        - Shell Var 'CMOCKA_XML_FILE'
        '''
        ci_type = thebuilder.env.GetValue('CI_BUILD_TYPE')
        if ci_type != 'host_unit_test':
            return 0

        shell_env = shell_environment.GetEnvironment()
        logging.log(edk2_logging.get_section_level(),
                    "Run Host based Unit Tests")
        path = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")

        failure_count = 0

        # Set up the reporting type for Cmocka.
        shell_env.set_shell_var('CMOCKA_MESSAGE_OUTPUT', 'xml')

        for arch in thebuilder.env.GetValue("TARGET_ARCH").split():
            logging.log(edk2_logging.get_subsection_level(),
                        "Testing for architecture: " + arch)
            cp = os.path.join(path, arch)

            # If any old results XML files exist, clean them up.
            for old_result in glob.iglob(os.path.join(cp, "*.result.xml")):
                os.remove(old_result)

            # Find and Run any Host Tests
            if GetHostInfo().os.upper() == "LINUX":
                testList = glob.glob(os.path.join(cp, "*Test*"))
                for a in testList[:]:
                    p = os.path.join(cp, a)
                    # It must be a file
                    if not os.path.isfile(p):
                        testList.remove(a)
                        logging.debug(f"Remove directory file: {p}")
                        continue
                    # It must be executable
                    if os.stat(p).st_mode & (stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH) == 0:
                        testList.remove(a)
                        logging.debug(f"Remove non-executable file: {p}")
                        continue

                    logging.info(f"Test file found: {p}")

            elif GetHostInfo().os.upper() == "WINDOWS":
                testList = glob.glob(os.path.join(cp, "*Test*.exe"))
            else:
                raise NotImplementedError("Unsupported Operating System")

            if not testList:
                logging.warning(dedent("""
                    UnitTest Coverage:
                      No unit tests discovered. Test coverage will not be generated.

                      Prevent this message by:
                      1. Adding host-based unit tests to this package
                      2. Ensuring tests have the word "Test" in their name
                      3. Disabling HostUnitTestCompilerPlugin in the package CI YAML file
                    """).strip())
                return 0

            for test in testList:
                # Configure output name if test uses cmocka.
                shell_env.set_shell_var(
                    'CMOCKA_XML_FILE', test + ".CMOCKA.%g." + arch + ".result.xml")
                # Configure output name if test uses gtest.
                shell_env.set_shell_var(
                    'GTEST_OUTPUT', "xml:" + test + ".GTEST." + arch + ".result.xml")

                # Run the test.
                ret = RunCmd('"' + test + '"', "", workingdir=cp)
                if ret != 0:
                    logging.error("UnitTest Execution Error: " +
                                  os.path.basename(test))
                else:
                    logging.info("UnitTest Completed: " +
                                 os.path.basename(test))
                    file_match_pattern = test + ".*." + arch + ".result.xml"
                    xml_results_list = glob.glob(file_match_pattern)
                    for xml_result_file in xml_results_list:
                        root = xml.etree.ElementTree.parse(
                            xml_result_file).getroot()
                        for suite in root:
                            for case in suite:
                                for result in case:
                                    if result.tag == 'failure':
                                        logging.warning(
                                            "%s Test Failed" % os.path.basename(test))
                                        logging.warning(
                                            "  %s - %s" % (case.attrib['name'], result.text))
                                        failure_count += 1

            if thebuilder.env.GetValue("CODE_COVERAGE") != "FALSE":
                if thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "GCC5":
                    ret = self.gen_code_coverage_gcc(thebuilder)
                    if ret != 0:
                        failure_count += 1
                elif thebuilder.env.GetValue("TOOL_CHAIN_TAG").startswith ("VS"):
                    ret = self.gen_code_coverage_msvc(thebuilder)
                    if ret != 0:
                        failure_count += 1
                else:
                    logging.info("Skipping code coverage. Currently, support GCC and MSVC compiler.")
                    return failure_count # MU_CHANGE - reformat coverage data

                # MU_CHANGE begin - reformat coverage data
                if thebuilder.env.GetValue("CC_REORGANIZE", "TRUE") == "TRUE":
                    ret = self.organize_coverage(thebuilder)
                    if ret != 0:
                        logging.error("Failed to reorganize coverage data by INF.")
                        return -1
                # MU_CHANGE end - reformat coverage data

        return failure_count

    def gen_code_coverage_gcc(self, thebuilder):
        logging.info("Generating UnitTest code coverage")

        buildOutputBase = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")
        workspace = thebuilder.env.GetValue("WORKSPACE")

        # Generate base code coverage for all source files
        ret = RunCmd("lcov", f"--no-external --capture --initial --directory {buildOutputBase} --output-file {buildOutputBase}/cov-base.info --rc lcov_branch_coverage=1")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to build initial coverage data.")
            return 1

        # Coverage data for tested files only
        ret = RunCmd("lcov", f"--capture --directory {buildOutputBase}/ --output-file {buildOutputBase}/coverage-test.info --rc lcov_branch_coverage=1")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to build coverage data for tested files.")
            return 1

        # Aggregate all coverage data
        ret = RunCmd("lcov", f"--add-tracefile {buildOutputBase}/cov-base.info --add-tracefile {buildOutputBase}/coverage-test.info --output-file {buildOutputBase}/total-coverage.info --rc lcov_branch_coverage=1")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to aggregate coverage data.")
            return 1

        # Filter out auto-generated and test code
        # MU_CHANGE begin - reformat coverage data
        file_out = thebuilder.env.GetValue("CI_PACKAGE_NAME", "") + "_coverage.xml"
        ret = RunCmd("lcov_cobertura",f"{buildOutputBase}/total-coverage.info --excludes ^.*UnitTest\|^.*MU\|^.*Mock\|^.*DEBUG -o {buildOutputBase}/{file_out}")
        # MU_CHANGE end - reformat coverage data
        if ret != 0:
            logging.error("UnitTest Coverage: Failed generate filtered coverage XML.")
            return 1

        # Generate all coverage file
        testCoverageList = glob.glob (f"{workspace}/Build/**/total-coverage.info", recursive=True)

        coverageFile = ""
        for testCoverage in testCoverageList:
            coverageFile += " --add-tracefile " + testCoverage
        ret = RunCmd("lcov", f"{coverageFile} --output-file {workspace}/Build/all-coverage.info --rc lcov_branch_coverage=1")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed generate all coverage file.")
            return 1

        # Generate and XML file if requested.for all package
        if os.path.isfile(f"{workspace}/Build/coverage.xml"):
            os.remove(f"{workspace}/Build/coverage.xml")
        ret = RunCmd("lcov_cobertura",f"{workspace}/Build/all-coverage.info --excludes ^.*UnitTest\|^.*MU\|^.*Mock\|^.*DEBUG -o {workspace}/Build/coverage.xml")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed generate all coverage XML.")
            return 1

        return 0


    def gen_code_coverage_msvc(self, thebuilder):
        logging.info("Generating UnitTest code coverage")


        buildOutputBase = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")
        testList = glob.glob(os.path.join(buildOutputBase, "**","*Test*.exe"), recursive=True)
        workspace = thebuilder.env.GetValue("WORKSPACE")
        workspace = (workspace + os.sep) if workspace[-1] != os.sep else workspace
        # Generate coverage file
        # MU_CHANGE begin - reformat coverage data
        pkg_cfg_file = os.path.join(buildOutputBase, "pkg-opencppcoverage.cfg")
        if os.path.isfile(pkg_cfg_file):
            os.remove(pkg_cfg_file)
        
        with open(pkg_cfg_file, "w") as f:
            for testFile in testList:
                ret = RunCmd("OpenCppCoverage", f"--source {workspace} --export_type binary:{testFile}.cov -- {testFile}", workingdir=f"{workspace}Build/")
                f.write(f"input_coverage={testFile}.cov\n")
                if ret != 0:
                    logging.error("UnitTest Coverage: Failed to collect coverage data.")
                    return 1

        # Generate and XML file if requested.by each package

        file_out = thebuilder.env.GetValue("CI_PACKAGE_NAME", "") + "_coverage.xml"
        ret = RunCmd("OpenCppCoverage", f"--export_type cobertura:{os.path.join(buildOutputBase, file_out)} --config_file={pkg_cfg_file}", workingdir=f"{workspace}Build/")
        os.remove(pkg_cfg_file)
       
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to generate cobertura format xml in single package.")
            return 1

        # Generate total report XML file for all package
        testCoverageList = glob.glob(os.path.join(workspace, "Build", "**","*Test*.exe.cov"), recursive=True)
        total_cfg_file = os.path.join(buildOutputBase, "total-opencppcoverage.cfg")
        if os.path.isfile(total_cfg_file):
            os.remove(total_cfg_file)
        
        with open(total_cfg_file, "w") as f:
            for testCoverage in testCoverageList:
                f.write(f"input_coverage={testCoverage}\n")

        ret = RunCmd("OpenCppCoverage", f"--export_type cobertura:{workspace}Build/coverage.xml --config_file={total_cfg_file}", workingdir=f"{workspace}Build/")
        os.remove(total_cfg_file)

        if ret != 0:
            logging.error("UnitTest Coverage: Failed to generate cobertura format xml.")
            return 1

        return 0

    def organize_coverage(self, thebuilder) -> int:
        """Organize the generated coverage file by INF."""
        db_path = self.parse_workspace(thebuilder)

        workspace = thebuilder.env.GetValue("WORKSPACE")
        buildOutputBase = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")
        package = thebuilder.env.GetValue("CI_PACKAGE_NAME", "")
        file_out = package + "_coverage.xml"
        cov_file = os.path.join(buildOutputBase, file_out)

        params = f"--database {db_path} coverage {cov_file} -o {cov_file} --by-package -ws {workspace}"

        params += f" -p {package}" * int(package != "")
        params += " --full" * int(thebuilder.env.GetValue("CC_FULL", "FALSE") == "TRUE")
        params += " --flatten" * int(thebuilder.env.GetValue("CC_FLATTEN", "FALSE") == "TRUE")

        return RunCmd("stuart_report", params)

    def parse_workspace(self, thebuilder) -> str:
        """Parses the workspace with Edk2DB with the tables necessarty to run stuart_report."""
        db_path = os.path.join(thebuilder.env.GetValue("BUILD_OUTPUT_BASE"), "DATABASE.db")
        db = Edk2DB(db_path, thebuilder.edk2path)
        db.register(EnvironmentTable(), SourceTable(), PackageTable(), InfTable())
        env_dict = thebuilder.env.GetAllBuildKeyValues() | thebuilder.env.GetAllNonBuildKeyValues()
        db.parse(env_dict)
        
        return db_path
    # MU_CHANGE end - reformat coverage data
