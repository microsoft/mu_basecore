##
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##


import os
import logging
import json

try:
    from MuEnvironment import PluginManager

    class BuildToolsReportGenerator(PluginManager.IUefiBuildPlugin):
        def do_report(self, thebuilder):
            try:
                from MuEnvironment import VersionAggregator
            except ImportError:
                logging.critical("Loading BuildToolsReportGenerator failed, please update mu_environment pip module")
                return 0

            OutputReport = os.path.join(thebuilder.env.GetValue("BUILD_OUTPUT_BASE"), "BUILD_TOOLS_REPORT")
            OutputReport = os.path.normpath(OutputReport)
            if not os.path.isdir(os.path.dirname(OutputReport)):
                os.makedirs(os.path.dirname(OutputReport))

            Report = BuildToolsReport()
            Report.MakeReport(VersionAggregator.GetVersionAggregator().GetAggregatedVersionInformation(), OutputReport=OutputReport)

        def do_pre_build(self, thebuilder):
            self.do_report(thebuilder)
            return 0

        def do_post_build(self, thebuilder):
            self.do_report(thebuilder)
            return 0

except ImportError:
    pass


class BuildToolsReport(object):
    MY_FOLDER = os.path.dirname(os.path.realpath(__file__))
    VERSION = "1.0"

    def __init__(self):
        pass

    def MakeReport(self, BuildTools, OutputReport="BuildToolsReport"):
        logging.info("Writing BuildToolsReports to {0}".format(OutputReport))
        versions_list = []
        for key, value in BuildTools.items():
            versions_list.append(value)
        versions_list = sorted(versions_list, key=lambda k: k['type'])
        json_dict = {"modules": versions_list}

        htmlfile = open(OutputReport + ".html", "w")
        jsonfile = open(OutputReport + ".json", "w")
        template = open(os.path.join(BuildToolsReport.MY_FOLDER, "BuildToolsReport_Template.html"), "r")

        for line in template.readlines():
            if "%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%" in line:
                line = line.replace("%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%", json.dumps(json_dict))
            htmlfile.write(line)

        jsonfile.write(json.dumps(versions_list, indent=4))

        jsonfile.close()
        template.close()
        htmlfile.close()
