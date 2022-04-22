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

            for test in testList:
                # Configure output name.
                shell_env.set_shell_var(
                    'CMOCKA_XML_FILE', test + ".%g." + arch + ".result.xml")

                # Run the test.
                ret = RunCmd('"' + test + '"', "", workingdir=cp)
                if(ret != 0):
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

            if thebuilder.env.GetValue("CODE_COVERAGE") == "TRUE":
                if thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "GCC5":
                    self.gen_code_coverage(thebuilder)
                else:
                    logging.info("Skipping code coverage. Only supported on GCC.")

        return failure_count

    def gen_code_coverage(self, thebuilder):
        logging.info("Generating UnitTest code coverage")

        # Generate base code coverage for all source files
        ret = RunCmd("lcov", "--no-external --capture --initial --directory ./ --output-file Build/cov-base.info --rc lcov_branch_coverage=1")
        if(ret != 0):
            logging.error("UnitTest Coverage: Failed to build initial coverage data.")
            return

        # Coverage data for tested files only
        ret = RunCmd("lcov", "--capture --directory Build/ --output-file Build/coverage-test.info --rc lcov_branch_coverage=1")
        if(ret != 0):
            logging.error("UnitTest Coverage: Failed to build coverage data for tested files.")
            return

        # Aggregate all coverage data
        ret = RunCmd("lcov", "--add-tracefile Build/cov-base.info --add-tracefile Build/coverage-test.info --output-file Build/total-coverage.info --rc lcov_branch_coverage=1")
        if(ret != 0):
            logging.error("UnitTest Coverage: Failed to aggregate coverage data.")
            return

        # Generate coverage XML
        ret = RunCmd("lcov_cobertura","Build/total-coverage.info -o Build/compare.xml")
        if(ret != 0):
            logging.error("UnitTest Coverage: Failed to generate coverage XML.")
            return

        # Filter out auto-generated and test code
        ret = RunCmd("lcov_cobertura","Build/total-coverage.info --excludes ^.*UnitTest\|^.*MU\|^.*Mock\|^.*DEBUG -o Build/coverage.xml")
        if(ret != 0):
            logging.error("UnitTest Coverage: Failed generate filtered coverage XML.")
            return

        # Generate and HTML file if requested.
        if thebuilder.env.GetValue("CC_HTML") == "TRUE":
            ret = RunCmd("pycobertura", "show --format html --output Build/coverage.html Build/coverage.xml --source .")
            if(ret != 0):
                logging.error("UnitTest Coverage: Failed to generate HTML.")

        return
