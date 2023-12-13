##
# Tool to generate a report on Flash Usage and Module sizes for a UEFI build.
# This tool depends on EDK2 and will parse UEFI build reports, fdf files, and other 
# standard EDK2 assets
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


import os
import sys
import re
import logging
import datetime
from typing import Optional, Dict, Iterable, Tuple # MU_CHANGE - Add Multiple FD support


#
# for now i want to keep this file as both a command line tool and a plugin for the Uefi Build system. 
# To do this the plugin class is only defined if in the build environment where the plugin classes are importable. 
#
#
try:
    from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin

    class FdSizeReportGenerator(IUefiBuildPlugin):

        def do_post_build(self, thebuilder):

            #not an error...just means don't use it
            if(thebuilder.env.GetValue("BUILDREPORTING") != "TRUE"):
                logging.debug("FdSize Report Generator Post Build Not Active due to BuildReporting Turned Off")
                return 0

            #Not an error just means a builder wants build reports but is building a product without
            # an FDF file
            elif(thebuilder.env.GetValue("FLASH_DEFINITION") is None):
                logging.debug("FdSize Report Generator Post Build Not Active due to FDF file not being defined")
                return 0

            #Error - User has build reporting on but hasn't defined the build report output file
            elif(thebuilder.env.GetValue("BUILDREPORT_FILE") is None):
                logging.error("FdSize Report Generator Post Build failed because Build Report file not defined")
                return -1

            # - User has build reporting on but hasn't defined
            if(thebuilder.env.GetValue("FDSIZEREPORT_FILE") is None):
                logging.debug("FdSize Report Generator Post Build - Fd Size Report file not defined in build system.  Setting to plugin default.")
                thebuilder.env.SetValue("FDSIZEREPORT_FILE", os.path.join(thebuilder.env.GetValue("BUILD_OUTPUT_BASE"), "FD_REPORT.HTML"), "Value defined by plugin")

            #1 - Get the output path for report file
            OutF = thebuilder.env.GetValue("FDSIZEREPORT_FILE")
            #2 - Get the FDF path
            FdfF = thebuilder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(thebuilder.env.GetValue("FLASH_DEFINITION"))
            #3 - Get the product name
            Product = thebuilder.env.GetValue("PRODUCT_NAME")
            if Product is None:
                logging.debug("Environment variable PRODUCT_NAME is not set")
                Product = "not set"
            #4 - Get the Fw version
            FwVersion = thebuilder.env.GetBuildValue("BUILDID_STRING")
            if FwVersion is None:
                logging.debug("Environment variable BUILDID_STRING is not set")
                FwVersion = "not set"
            #5 - Get the build Report file
            BuildReportF = thebuilder.env.GetValue("BUILDREPORT_FILE")

            Report = FdReport()

            ret = Report.MakeReport(Product, FwVersion, OutF, FdfF, BuildReportF) # MU_CHANGE begin - Add Multiple FD support
            logging.debug("Build Report generation returned %d" % ret)
            return ret

except ImportError:
    pass

# MU_CHANGE begin - Add Multiple FD support
class FdfMiniParser(object):
    """A simple FDF parser that looks for comments describing a Region."""
    def __init__(self, fdfFileLines: Iterable[str]) -> None:
        """Initializes the FDF parser"""
        self.FileLines = fdfFileLines
        self.MaxLines = len(self.FileLines)
        self.RE_RegionDescLine = re.compile('^ *#Region_Description:')

    def GetRegionDescComment(self, BaseAddress: str) -> str:
        """Returns the comment for a region with the given base address.

        Args:
            BaseAddress (str): The base address (hex) of the region to find the comment for.

        Returns:
            str: The comment for the region, or an empty string if no comment was found.
        """
        current = 0
        while(current < self.MaxLines):
            if self.RE_RegionDescLine.match(self.FileLines[current]):
                al = self.FileLines[current+1]
                if al.count("|") == 1 and al.count("x") == 2:
                    #found a good base address. Now compare
                    add = int(al.strip().partition('|')[0], 0)
                    if hex(add) == BaseAddress:
                        return self.FileLines[current].partition(":")[2].strip()
            current = current + 1
        return ""


class SectionType:
    """A interface for section types that parse their type of section."""
    pass


class FirmwareDevice(SectionType):
    """A struct representing a "Firmware Device (FD)" Section in the build report.

    Will process the entire FirmwareDevice section, including sub-sections
    when initialized with __init__.

    !!! Note
        This should not be initialized manually. Use the SectionFactory to
        parse the section and produce the correct section type.
    """

    SECTION = r">-{118}<(.*?)(?=<-{118}>)"

    def __init__(self, name: str, base: str, size: str, raw_section: str) -> None:
        """Initializes a FD object.

        Args:
            name (str): The raw string parsed from the 'FD Name:' line
            base (str): The raw string parsed from the 'Base Address:' line
            size (str): The raw string parsed from the 'Size:' line
        """
        self.name = name.strip()
        self.base = base.strip()
        self.size = size.strip()
        self.regions: list[FdRegion] = []
        self.process_section(raw_section)

    def process_section(self, raw_section: str) -> None:
        """Processes the different FD regions.

        Create a list of regions that are inside the FD section.

        Args:
            raw_section (str): The raw string of the entire FD section
        """
        sections = re.findall(self.SECTION, raw_section, re.DOTALL)

        for section in sections:
            if section.strip().lower().startswith("fd region"):
                self.regions.append(FdRegion.from_raw(section, False))
            elif section.strip().lower().startswith("nested fv"):
                self.regions.append(FdRegion.from_raw(section, True))

    def to_json(self, module_summary_dict: Dict[str, 'ModuleSummary']) -> dict:
        """Creates a JSON representation of the FD object.

        Args:
            module_summary_dict (Dict[str, 'ModuleSummary']): A dictionary of
                ModuleSummary objects, which represent a Module Summary
                section in the build report. Key is the name of the module.
        """
        return {
            "base": self.base,
            "size": self.size,
            "regions": [region.to_json(self.base, module_summary_dict) for region in self.regions],
        }


class FdRegion:
    """A struct representing a Firmware Device Region subsection in the build report.

    Will process a raw FD Region subsection (of the Firmware Device section). Certain
    types of region subsections will contain different information in the header and
    may contain a sub-sub section containing the modules in the region.

    Unlike the other classes representing the different parts of the build report,
    we use a "from_raw" function, which acts somewhat 
    !!! Note
        Different types of FD regions exist (specified by the "Type" line),
        which contain different information. Due to this, it may be necessary
        in the future to use another Factory class, but for now, we do not
        do this because the information we want from the FdRegion is available
        in all types of FD regions.

        Unlike other classes representing the different portions of the build
        report, we use "from_raw" rather than "__init__" to create the object
        as we can pseudo treat it like a factory class.
    """
    FD_REGION = r"Type:\s+(.*?)(?:\r?\n|\r)Base Address:\s+(.*?)(?:\r?\n|\r)Size:\s+(.*?)(?:\r?\n|\r)"

    TYPE_FV = r"Fv Name:\s+(.*?)(?:\r?\n|\r)Occupied Size:\s+(.*?)(?:\r?\n|\r)Free Size:\s+(.*?)(?:\r?\n|\r)"
    TYPE_CAPSULE = r"Capsule Name:\s+(.*?)(?:\r?\n|\r)?Capsule Size:\s+(.*?)"
    TYPE_FILE = r"File Name:\s+(.*?)(?:\r?\n|\r)?File Size:\s+(.*?)"

    MEM_LOC = r'(?i)(0x[0-9A-Fa-f]+)\s+([^\n(]+(?:\n[^\n(]+)*)\s*(?:\(([^)]*)\))?'

    def __init__(self):
        """Initializes an empty FD region. Does not parse the region."""
        self.nested = None
        self.type = None
        self.base = None
        self.size = None
        self.name = None
        self.used_size = None
        self.used_percent = None
        self.free_size = None
        self.free_percent = None
        self.raw_modules = []

    def parse_fv_type_region(nested: bool, match: re.Match) -> 'FdRegion':
        """Parses a FV type FD region header and the module list subsection.

        Args:
            nested (bool): Whether or not the region is nested in another FV region.
            match (re.Match): The match object from the regex search.
        """
        region = FdRegion()
        region.nested = nested

        region.type = match.group(1).strip()
        region.base = match.group(2).strip()

        # `0x348000 (3360K)` -> `0x348000`
        region.size = match.group(3).strip().split()[0]

        # `FVMAIN_COMPACT (79.9% Full)` -> (`FVMAIN_COMPACT`, `79.9`)
        (name, percent, _) = match.group(4).strip().split(' ', maxsplit=2)
        region.name = name
        region.used_percent = percent.strip('(%)')

        # `0x29ED18 (2683K)` -> `0x29ED18`
        region.used_size = match.group(5).strip().split()[0]

        # `0x6A000 (664K)` -> `0x6A000`
        region.free_size = match.group(6).strip().split()[0]

        region.free_percent = str(round(100 - float(region.used_percent), 2))
        return region

    def parse_capsule_file_type_region(match: re.Match) -> 'FdRegion':
        """Parses a Capsule or File type FD region header.

        Args:
            match (re.Match): The match object from the regex search.
        """
        region = FdRegion()
        region.type = match.group(1).strip()
        region.base = match.group(2).strip()

        # `0x348000 (3360K)` -> `0x348000`
        region.size = match.group(3).strip().split()[0]

        # "Capsule Name" or "File Name:""
        region.name = match.group(4).strip()

        # "Capsule Size:" or "File Size:", `0x348000 (3360K)` -> `0x348000`
        region.size = match.group(5).strip().split()[0]

    def parse_generic_region(match: re.match) -> 'FdRegion':
        """Parses a generic FD region header.

        Args:
            match (re.Match): The match object from the regex search.
        """
        region = FdRegion()

        region.type = match.group(1).strip()
        region.base = match.group(2).strip()

        # `0x348000 (3360K)` -> `0x348000`
        region.size = match.group(3).strip().split()[0]
        return region

    def from_raw(raw_region: str, nested):
        """Somewhat follows a Factory pattern to create a FdRegion object.

        Args:
            raw_region (str): The raw string of the FD region.
            nested (bool): Whether or not the region is nested in another FV region.
        """        
        # FV Type region
        match = re.search(FdRegion.FD_REGION+FdRegion.TYPE_FV, raw_region, re.DOTALL)
        if match:
            logging.debug("FV Type FD Region found.")
            region = FdRegion.parse_fv_type_region(nested, match)
            region.raw_modules = FdRegion.parse_module_memory_table(raw_region)
            return region

        # Capsule Type region
        match = re.search(FdRegion.FD_REGION+FdRegion.TYPE_CAPSULE, raw_region, re.DOTALL)
        if match:
            logging.debug("Capsule Type FD Region found.")
            return FdRegion.parse_capsule_file_type_region(match)

        # Fv Type Region
        match = re.search(FdRegion.FD_REGION+FdRegion.TYPE_FILE, raw_region, re.DOTALL)
        if match:
            logging.debug("File Type FD Region found.")
            return FdRegion.parse_capsule_file_type_region(match)

        # Generic FD Region
        match = re.search(FdRegion.FD_REGION, raw_region, re.DOTALL)
        if match:
            logging.debug("Generic FD Region found.")
            return FdRegion.parse_generic_region(match)

        logging.error("No match found for FD Region")
        return None

    def parse_module_memory_table(region: str) -> Iterable[Tuple[str, str, str]]:
        """Parses the table of modules in the FV region.

        Can be tricky as new lines will occur, typically in the module path. 
        The MEM_LOC regex is able to handle this, but the newline remains in
        the line.

        Args:
            region (str): The raw string of the FD region.

        Returns:
            Iterable[Tuple[str, str, str]]: (hex offset, module name, module path)

        !!! Example:
            Offset     Module
            -----------------------------------------------------------------
            0x00000078 PeiCore (c:\\src\\mu_tiano_platforms\\MU_BASECORE\\MdeModulePkg\\Core\\Pei\\PeiMain.inf)
        """
        table = region.split("------------------------------------------------------------------------------------------------------------------------")
        if len(table) == 1:
            return []
        table_rows = re.findall(FdRegion.MEM_LOC, table[1])

        return_rows = []
        for row in table_rows:
            return_rows.append(tuple(item.replace('\n', '') for item in row))
        return return_rows

    def to_json(self, base: int, module_summary_dict: Dict[str, 'ModuleSummary']) -> dict:
        """Converts the FD region to a JSON format.

        When converting the modules inside the FD to JSON, it looks up
        additional module information from the dict of ModuleSummary objects.

        Args:
            base (int): The base address of the FD, to calculate the system
                address of the FD.
            module_summary_dict (Dict[str, 'ModuleSummary']): A dictionary of
                ModuleSummary objects, to look up additional module
                information.

        Returns:
            dict: A dict with information about the FD region
        """
        mods = []
        for mod in self.raw_modules:
            mod_summary = module_summary_dict.get(mod[1].strip())
            if mod_summary:
                mods.append({
                    "name": mod_summary.name,
                    "size": mod_summary.size
                })
        return {
            "name": self.name or self.type,
            "description": FlashReportParser.FDF_MINI_PARSER.GetRegionDescComment(self.base),
            "base": self.base,
            "system_address": hex(int(base, 0) + int(self.base, 0)),
            "size": self.size or "NA",
            "used_percent": (self.used_percent or "NA") + '%',
            "free": self.free_size or "NA",
            "free_percent": (self.free_percent or "NA") + '%',
            "used": self.used_size or "NA",
            "nested": str(self.nested),
            "modules": mods
        }


class SectionFactory(object):
    """A factory class for parsing a BuildReport section, and returning the correct section type."""

    FD_SECTION = r"FD Name:\s+(.*?)(?:\r?\n|\r)Base Address:\s+(.*?)(?:\r?\n|\r)Size:\s+(.*?)(?:\r?\n|\r|$)"
    MODULE_SUMMARY = r"Module Name:\s+(.*?)(?:\r?\n|\r)Module Arch:\s+(.*?)(?:\r?\n|\r)Module INF Path:\s+(.*?)(?:\r?\n|\r)File GUID:\s+(.*?)(?:\r?\n|\r)"

    def parse_section(self, raw_section: str) -> Optional[SectionType]:
        """Attempts to determine the section type, and parses the section accordingly.

        Args:
            raw_section (str): The raw string of the section to parse.
        """
        match = re.search(self.FD_SECTION, raw_section, re.DOTALL)
        if match:
            return FirmwareDevice(match.group(1), match.group(2), match.group(3), raw_section)
        match = re.search(self.MODULE_SUMMARY, raw_section, re.DOTALL)
        if match:
            return ModuleSummary(match.group(1), match.group(2), match.group(3), match.group(4), raw_section)
        return None


class ModuleSummary(SectionType):
    """A struct representing a Module Summary Section in the build report.

    Will process the entire Module Summary section, including sub-sections
    when initialized with __init__.

    !!! Note
        This should not be initialized manually. Use the SectionFactory to
        parse the section and produce the correct section type.
    """
    DRIVER_PATTERN = r"Driver Type:\s*(.*?)(?:\r?\n|\r|$)"
    SIZE_PATTERN = r"Size:\s+(.*?)(?:\r?\n|\r)"

    def __init__(self, name: str, arch: str, path: str, guid: str, raw_section: str) -> None:
        """Initializes a Module Summary object.

        size and driver_type are set to default values until the rest of the
        section is processed. These values are not always available (suc as if)
        the module built is a library.
 
        Args:
            name (str): The raw string parsed from the 'Module Name:' line
            arch (str): The raw string parsed from the 'Module Arch:' line
            path (str): The raw string parsed from the 'Module INF Path:' line
            guid (str): The raw string parsed from the 'File GUID:' line
        """
        self.name = name.strip()
        self.arch = arch.strip()
        self.path = path.strip()
        self.guid = guid.strip()
        self.size = "0x0"
        self.driver_type = ""
        self.process_raw(raw_section)

    def process_raw(self, raw):
        match = re.search(self.DRIVER_PATTERN, raw)
        if match:
            # ex: `0x7 (DRIVER)` -> `DRIVER`
            self.driver_type = match.group(1).strip().split()[1].strip('()')

        match = re.search(self.SIZE_PATTERN, raw)
        if match:
            # ex: `0x3C00 (15.00K)` -> `0x3C00`
            self.size = match.group(1).strip().split()[0]


class FlashReportParser(object):
    """"A class for parsing a build report and generating a flash report."""

    # Warning, poor design. If you need to instantiate this class multiple
    # time for some reason, the FDF mini parser will break as this is global
    # to any class.
    FDF_MINI_PARSER = None

    def __init__(self, ReportFileLines, FdfFileLines = None, ToolVersion="1.00", ProductName="Unset", ProductVersion="Unset"):
        """Initializes the object."""
        self.fd_list: list[FirmwareDevice] = []
        self.module_summary: Dict[str, ModuleSummary] = {}

        self.ReportFile = ReportFileLines

        self.FdfMiniParser = None
        self.ToolVersion = ToolVersion
        self.ProductVersion = ProductVersion
        self.ProductName = ProductName

        if FdfFileLines is not None and len(FdfFileLines) > 0:
            FlashReportParser.FDF_MINI_PARSER = FdfMiniParser(FdfFileLines)

    def parse_report_sections(self):
        """Splits the build report into sections and parses each section depending on the detected section type.

        A section is all content between a >=..=< and a <=..=>. line.
        """
        section_pattern = r">={118}<(.*?)(?=<={118}>)"
        factory = SectionFactory()

        sections = re.findall(section_pattern, "\n".join(self.ReportFile), re.DOTALL)
        for section in sections:
            parsed_section = factory.parse_section(section)

            if isinstance(parsed_section, FirmwareDevice):
                self.fd_list.append(parsed_section)
            elif isinstance(parsed_section, ModuleSummary):
                self.module_summary[parsed_section.name] = parsed_section

    def write_report(self, out_path: str):
        """Writes the report to a file."""
        data = {}
        for fd in self.fd_list:
            data[fd.name] = fd.to_json(self.module_summary)

        all_modules = [{
            "name": module.name,
            "type": module.driver_type or "",
            "size": module.size or "0"
        } for module in self.module_summary.values()]

        env = {
            "product_name": self.ProductName,
            "product_version": self.ProductVersion,
            "product_date": datetime.datetime.now().strftime("%A, %B %d, %Y %I:%M %p")
        }

        f = open(out_path, "w")
        template = open(os.path.join(FdReport.MY_FOLDER, "FdReport_Template.html"), "r")
        for line in template.readlines():
            if "%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%" in line:
                new_str = f'''
                    var all_mods = {all_modules};
                    var fds = {data};
                    var env = {env};
                '''
                line = line.replace("%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%", new_str)
            f.write(line)
        template.close()
        f.close()


class FdReport(object):
    """A Class representing a Flash Report."""
    MY_FOLDER = os.path.dirname(os.path.realpath(__file__))
    VERSION = "3.00"

    def MakeReport(self, ProductName, ProductVersion, OutputReport, InputFdf, InputBuildReport):
        """Creates a Flash Report."""
        if not os.path.isfile(InputFdf):
            logging.critical("Invalid path to input FDF file")
            return -2

        if not os.path.isfile(InputBuildReport):
            logging.critical("Invalid path to input Build Report file")
            return -1

        f = open(InputBuildReport, "r")
        lines = f.readlines()
        f.close()

        f = open(InputFdf, "r")
        fdfLines = f.readlines()
        f.close()

        rep = FlashReportParser(lines, fdfLines, FdReport.VERSION, ProductName, ProductVersion)
        rep.parse_report_sections()
        rep.write_report(OutputReport)
        return 0
# MU_CHANGE end - Add Multiple FD support


################################################
# This plugin python file is also
# a command line tool
#
################################################
if __name__ == '__main__':
    from optparse import OptionParser
    #setup main console as logger
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    console.setFormatter(formatter)
    logger.addHandler(console)

    #Do real work cmdline work here
    parser = OptionParser()
    #Output debug log
    parser.add_option("-l", dest="OutputReport", help="Create an output log file: ie -l out.txt", default=None)
    parser.add_option("-o", dest="OutputFile", help="Output file (Will be HTML file) : ie -o MyOutput.html", default=None)
    parser.add_option("-i", dest="InputReport", help="Input Report File." , default=None)
    parser.add_option("-f", dest="FdfFile", help="Input Fdf File used for additional information gathering (optional).", default=None)
    parser.add_option("--product", dest="ProductName", help="Name of product for report", default="Unknown")
    parser.add_option("--fwVersion", dest="FirmwareVersion", help="Firmware Version", default="Unknown")
    #Turn on dubug level logging
    parser.add_option("--debug", action="store_true", dest="debug", help="turn on debug logging level for file log",  default=False)

    (options, args) = parser.parse_args()
    #setup file based logging if outputReport specified
    if(options.OutputReport):
        if(len(options.OutputReport) < 2):
            logging.critical("the output report file parameter is invalid")
            sys.exit(-2)
        else:
            #setup file based logging
            filelogger = logging.FileHandler(filename=options.OutputReport, mode='w')
            if(options.debug):
                filelogger.setLevel(logging.DEBUG)
            else:
                filelogger.setLevel(logging.INFO)

            filelogger.setFormatter(formatter)
            logging.getLogger('').addHandler(filelogger)

    logging.info("Log Started: " + datetime.datetime.strftime(datetime.datetime.now(), "%A, %B %d, %Y %I:%M%p" ))

    #parse report and generate output
    if not options.InputReport:
        logging.critical("No Input report file")
        sys.exit(-1)

    if not os.path.isfile(options.InputReport):
        logging.critical("Invalid path to input report")
        sys.exit(-2)

    if not options.OutputFile:
        logging.critical("No Output file")
        sys.exit(-3)

    Report = FdReport()
    ret = Report.MakeReport(options.ProductName, options.FirmwareVersion, options.OutputFile, options.FdfFile, options.InputReport) # MU_CHANGE begin - Add Multiple FD support
    logging.debug("Build Report generation returned %d" % ret)

    if ret != 0:
        logging.critical("Tool Failed.  Return Code: %i" % ret)
    #end logging
    logging.shutdown()
    sys.exit(ret)
