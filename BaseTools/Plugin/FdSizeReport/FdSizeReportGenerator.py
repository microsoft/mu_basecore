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
from decimal import Decimal
from typing import Optional, Dict
from jinja2 import Environment, FileSystemLoader


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

            ret = Report.MakeReport(Product, FwVersion, OutF, FdfF, BuildReportF, thebuilder.ws, thebuilder.pp)
            logging.debug("Build Report generation returned %d" % ret)
            return ret

except ImportError:
    pass


##
# Support code/classes for parsing and creating report
##

class FvModule(object):

    def __init__(self):
        self.Path = ''
        self.Name = ''


class FdRegion(object):

    def __init__(self):
        self.Type = ''
        self.Base = ''
        self.Size = ''
        self.Desc = 'Unknown'
        self.Used = ''
        self.Free = ''
        self.SystemAddress = '0'
        self.FvModuleList = []
        self.Nested = "False"

    def GetFreePercent(self):
        try:
            v = Decimal(int(self.Free, 0)) *100 / Decimal(int(self.Size, 0))
            return "%0.2f%%" % v
        except:
            return 'NA'

    def GetUsedPercent(self):
        try:
            v = Decimal(int(self.Used, 0)) *100 / Decimal(int(self.Size, 0))
            return "%0.2f%%" % v
        except:
            return "NA"

class ModuleSummary(object):

    def __init__(self):
        self.Name = ''
        self.Size = 0
        self.SizeString = ''
        self.Guid = ''
        self.InfPath = ''
        self.Type = ''
        self.MapFilePath = ''
        self.FvList = []

class FdfMiniParser(object):


    def __init__(self, fdfFileLines):
        self.FileLines = fdfFileLines
        self.MaxLines = len(self.FileLines)
        self.RE_RegionDescLine = re.compile('^ *#Region_Description:')


    def GetRegionDescComment(self, BaseAddress):
        current = 0
        while(current < self.MaxLines):
            if self.RE_RegionDescLine.match(self.FileLines[current]):
                al = self.FileLines[current+1]
                if al.count("|") == 1 and al.count("x") == 2:
                    #found a good base address. Now compare
                    add = int(al.strip().partition('|')[0], 0)
                    if add == BaseAddress:
                        return self.FileLines[current].partition(":")[2].strip()
            current = current + 1
        return ""

# Understanding a Build Report
###############################################################################
# Section Type: Firmware Device (FD). One or more Firmware Devices (FDs).     #
# Each FD is a separate file.                                                 #
###############################################################################
#                                                                             #
#  #########################################################################  #
#  # Subsection Type: FD Region. One or more FD Regions. Each region is    #  #
#  # has a type. We mainly care about FV Types, but there are also capsule #  #
#  # and file types. This will contain Modules and their offsets.          #  #
#  #########################################################################  #
#  #                                                                       #  #
#  #  ###################################################################  #  #
#  #  # Sub-subsection Type: One or more modules. This section is used  #  #  #
#  #  # when the FD region is of type FV. This will contain a list of   #  #  #
#  #  # modules and their relative offsets to the FD region. Must look  #  #  #
#  #  # up the module information in the Module Summary section, which  #  #  #
#  #  # is a Section Type.                                              #  #  #
#  #  ###################################################################  #  #
#  #                                                                       #  #
#  #########################################################################  #
#                                                                             #
###############################################################################
# Section Type: Module Summary. One  or more Module Summary sections. Each    #
# section contains generic information about a compiled module. Associate     #
# this information with the modules in the FD Regions.                        #
###############################################################################
#                                                                             #
#  #########################################################################  #
#  # Subsection Type: Module Summary. Contains Generic information about a #  #
#  # module, including name, arch, path, and guid. Contains additional     #  #
#  # subsections or more information about the module, such as PCD values, #  #
#  # build flags, specific libraries used, dependency expressions, etc.    #  #
#  #########################################################################  #
#                                                                             #
###############################################################################
class _FdRegion:    
    FD_REGION = r"Type:\s+(.*?)(?:\r?\n|\r)Base Address:\s+(.*?)(?:\r?\n|\r)Size:\s+(.*?)(?:\r?\n|\r)"

    TYPE_FV = r"Fv Name:\s+(.*?)(?:\r?\n|\r)Occupied Size:\s+(.*?)(?:\r?\n|\r)Free Size:\s+(.*?)(?:\r?\n|\r)"
    TYPE_CAPSULE = r"Capsule Name:\s+(.*?)(?:\r?\n|\r)?Capsule Size:\s+(.*?)"
    TYPE_FILE = r"File Name:\s+(.*?)(?:\r?\n|\r)?File Size:\s+(.*?)"

    MEM_LOC = r'(?i)(0x[0-9A-Fa-f]+)\s+([^\n(]+(?:\n[^\n(]+)*)\s*(?:\(([^)]*)\))?'
    MEM_LOC2  = r'([0-9xA-F]+)\s+([\w\s]+)\s+(?:\(([^)]*)\))?\r?\n'

    def __init__(self):
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

    def parse_mem_regions(self, region: str) -> list:
        nregion = region.split("------------------------------------------------------------------------------------------------------------------------")
        if len(nregion) == 1:
            return []
        return re.findall(_FdRegion.MEM_LOC, nregion[1])
           
    def from_raw(raw_region: str, nested):
        fd_region = _FdRegion()
        fd_region.nested = nested
        
        # FV Type region
        match = re.search(_FdRegion.FD_REGION+_FdRegion.TYPE_FV, raw_region, re.DOTALL)
        if match:
            logging.debug("FV Type FD Region found.")
            fd_region.type = match.group(1).strip()
            fd_region.base = match.group(2).strip()
            fd_region.size = match.group(3).strip().split()[0]
            (name, percent, _) = match.group(4).strip().split(' ', maxsplit=2)
            fd_region.name = name
            fd_region.used_percent = percent.strip('(%)')
            fd_region.used_size = match.group(5).strip().split()[0]
            fd_region.free_size = match.group(6).strip().split()[0]
            fd_region.free_percent = str(round(100 - float(fd_region.used_percent), 2))
            fd_region.raw_modules = fd_region.parse_mem_regions(raw_region)
            return fd_region
        
        # Capsule or File Type region (Treated the same) //TODO
        match = re.search(_FdRegion.FD_REGION+_FdRegion.TYPE_CAPSULE, raw_region, re.DOTALL) or re.search(_FdRegion.FD_REGION+_FdRegion.TYPE_FILE, raw_region, re.DOTALL)
        if match:
            logging.debug("Capsule Type FD Region found.")
            fd_region.type = match.group(1).strip()
            fd_region.base = match.group(2).strip()
            fd_region.size = match.group(3).strip()
            fd_region.name = match.group(4).strip()
            fd_region.used_percent = match.group(5).strip()
            fd_region.free_size = str("0x%X" % (int(fd_region.size, 0) - int(fd_region.occupied_size, 0)))
            fd_region.raw_modules = fd_region.parse_mem_regions(raw_region)
            return fd_region

        # Generic FD Region
        match = re.search(_FdRegion.FD_REGION, raw_region, re.DOTALL)
        if match:
            logging.debug("Generic FD Region found.")
            fd_region.type = match.group(1).strip()
            fd_region.base = match.group(2).strip()
            fd_region.size = match.group(3).strip()
            fd_region.raw_modules = fd_region.parse_mem_regions(raw_region)
            return fd_region
        
        print("No match found for FD Region")
        return fd_region

    def to_json(self, base: int, module_summary_dict: Dict[str, '_ModuleSummary']) -> dict:
        
        mods = []
        for mod in self.raw_modules:
            mod_summary = module_summary_dict.get(mod[1].strip())
            if mod_summary:
                mods.append({
                    "name": mod_summary.name,
                    "size": mod_summary.size
                })
        
        return {
            "description": self.name or self.type,
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


class SectionType:
    """A interface for section types that parse their type of section."""
    pass


class FirmwareDevice(SectionType):
    SECTION = r">-{118}<(.*?)(?=<-{118}>)"

    def __init__(self, name, base, size, raw_section):
        self.name = name.strip()
        self.base = base.strip()
        self.size = size.strip()
        self.regions: list[_FdRegion] = []
        self.process_section(raw_section)

    def process_section(self, raw_section):
        sections = re.findall(self.SECTION, raw_section, re.DOTALL)
        
        for section in sections:
            if section.strip().lower().startswith("fd region"):
                self.regions.append(_FdRegion.from_raw(section, False))
            elif section.strip().lower().startswith("nested fv"):
                self.regions.append(_FdRegion.from_raw(section, True))

    def to_json(self, module_summary_dict: Dict[str, '_ModuleSummary']) -> dict:
        return {
            "base": self.base,
            "size": self.size,
            "regions": [region.to_json(self.base, module_summary_dict) for region in self.regions],
        }


class _ModuleSummary(SectionType):
    DRIVER_PATTERN = r"Driver Type:\s*(.*?)(?:\r?\n|\r|$)"
    SIZE_PATTERN = r"Size:\s+(.*?)(?:\r?\n|\r)"

    def __init__(self, name, arch, path, guid, raw_section):
        self.name = name.strip()
        self.arch = arch.strip()
        self.path = path.strip()
        self.guid = guid.strip()
        self.size = 0
        self.driver_type = ""
        self.process_raw(raw_section)

    def process_raw(self, raw):
        match = re.search(self.DRIVER_PATTERN, raw)
        if match:
            self.driver_type = match.group(1).strip().split()[1].strip('()')
        
        match = re.search(self.SIZE_PATTERN, raw)
        if match:
            self.size = match.group(1).strip().split()[0]


class SectionFactory(object):
    FD_SECTION = r"FD Name:\s+(.*?)(?:\r?\n|\r)Base Address:\s+(.*?)(?:\r?\n|\r)Size:\s+(.*?)(?:\r?\n|\r|$)"
    MODULE_SUMMARY = r"Module Name:\s+(.*?)(?:\r?\n|\r)Module Arch:\s+(.*?)(?:\r?\n|\r)Module INF Path:\s+(.*?)(?:\r?\n|\r)File GUID:\s+(.*?)(?:\r?\n|\r)"

    def parse_section(self, raw_section: str) -> Optional[SectionType]:
        """Attempts to determine the section type, and parses the section accordingly."""
        match = re.search(self.FD_SECTION, raw_section, re.DOTALL)
        if match:
            return FirmwareDevice(match.group(1), match.group(2), match.group(3), raw_section)
        match = re.search(self.MODULE_SUMMARY, raw_section, re.DOTALL)
        if match:
            return _ModuleSummary(match.group(1), match.group(2), match.group(3), match.group(4), raw_section)
        return None

class FlashReportParser(object):
    SECTION_START = ">======================================================================================================================<"
    SECTION_END = "<======================================================================================================================>"
    def __init__(self, ReportFileLines, FdfFileLines = None, ToolVersion="1.00", ProductName="Unset", ProductVersion="Unset"):
        self.fd_list: list[FirmwareDevice] = []
        self.module_summary: Dict[str, _ModuleSummary] = {}

        self.ReportFile = ReportFileLines
        self.Parsed = False
        self.FdRegions = []
        self.Modules = []
        self.TotalLines = len(ReportFileLines)

        self.FdBase = ''
        self.FdName = ''
        self.FdSize = ''
        self.FdSizeNum = 0
        self.FdBaseNum = 0
        self.FdfMiniParser = None
        self.ToolVersion = ToolVersion
        self.ProductVersion = ProductVersion
        self.ProductName = ProductName

        if FdfFileLines != None and len(FdfFileLines) > 0:
            self.FdfMiniParser = FdfMiniParser(FdfFileLines)

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
            elif isinstance(parsed_section, _ModuleSummary):
                self.module_summary[parsed_section.name] = parsed_section
    
    def write_report(self):
        """Writes the report to a file."""
        env = Environment(loader=FileSystemLoader(searchpath=os.path.join(os.path.dirname(__file__))))
        template = env.get_template("FdReport_Template2.html")
        
        data = {}
        for fd in self.fd_list:
            data[fd.name] = fd.to_json(self.module_summary)

        all_modules = [{
            "name": module.name,
            "type": module.driver_type or "",
            "size": module.size or "0"
        } for module in self.module_summary.values()]

        import pprint
        pprint.pprint(data)
        Embedded = {
            "modules": []
        }

        env = {
            "product_name": self.ProductName,
            "product_version": self.ProductVersion,
            "date": datetime.datetime.now()
        }
        
        with open("myreport.html", "w") as f:
            #f.write(template.render(results=results, tool_version=self.ToolVersion, product_name=self.ProductName, product_version=self.ProductVersion, date=datetime.datetime.now().strftime("%A, %B %d, %Y %I:%M%p")))
            f.write(template.render(EmbeddedJd = Embedded, fds = data, all_modules=all_modules, env=env))
        exit()
    def ParseFdInfo(self):
        self.parse_report_sections()
        self.write_report()

        CurrentLine = 0
        FoundAll = False
        FoundIt = False
        while(CurrentLine < self.TotalLines) and not FoundAll:
            line = self.ReportFile[CurrentLine]

            if FoundIt:
                tokens = line.strip().split(':')
                if len(tokens) == 2:
                    if(tokens[0].strip().lower() == "fd name"):
                        self.FdName = tokens[1]
                    elif(tokens[0].strip().lower() == 'base address'):
                        self.FdBase=tokens[1]
                        self.FdBaseNum = int(self.FdBase, 0)
                    elif(tokens[0].strip().lower() == 'size'):
                        self.FdSize = tokens[1].split()[0].strip()
                        self.FdSizeNum = int(self.FdSize, 0)
                        FoundAll = True
            elif line.strip().lower() == 'firmware device (fd)':
                FoundIt = True

            CurrentLine = CurrentLine + 1

    def ParseAllModules(self):
        InModSummary = False
        CurrentMod = 0
        CurrentLine = 0
        while(CurrentLine < self.TotalLines):
            line = self.ReportFile[CurrentLine]

            #look for module summary section region
            if  line.strip().lower() == 'module summary':
                #new summary
                InModSummary = True
                mod = ModuleSummary()
                self.Modules.append(mod)
                CurrentMod = len(self.Modules) - 1

            #parse within mod summary
            elif InModSummary:
                tokens = line.strip().split()
                if len(tokens) > 0:
                    if tokens[0].strip() == 'Module':
                        if tokens[1].strip() == 'Name:':
                            (self.Modules[CurrentMod]).Name = tokens[2].strip()
                        elif tokens[1].strip() == 'INF':
                            self.Modules[CurrentMod].InfPath = tokens[3].strip().replace("\\", "/")
                            if(not tokens[3].strip().lower().endswith(".inf")):
                                #get next line
                                CurrentLine += 1
                                line = self.ReportFile[CurrentLine]
                                self.Modules[CurrentMod].InfPath += line.strip().replace("\\", "/")

                    elif (tokens[0].strip() == 'File'):
                        if(tokens[1].strip() == 'GUID:'):
                            (self.Modules[CurrentMod]).Guid = tokens[2].strip()
                            
                    elif (tokens[0].strip() == 'Size:'):
                        (self.Modules[CurrentMod]).SizeString = tokens[1].strip()
                        (self.Modules[CurrentMod]).Size = int(tokens[1].strip(), 0)  
                    
                    elif (tokens[0].strip() == 'Driver'):
                        (self.Modules[CurrentMod]).Type = tokens[3].strip()                     

                    elif tokens[0].strip() == '========================================================================================================================':
                        InModSummary = False

            #increment the currentline
            CurrentLine = CurrentLine + 1
        #done processing report file.
        #sort list descending
        self.Modules.sort(key=lambda x: x.Size, reverse=True)
        


    def ParseFdRegions(self):
        InFdRegion = False
        CurrentRegion = 0
        CurrentLine = 0
        while(CurrentLine < self.TotalLines):
            line = self.ReportFile[CurrentLine]

            #look for fd region
            if  line.strip().lower() == 'fd region':
                #new fd region
                InFdRegion = True
                Region = FdRegion()
                self.FdRegions.append(Region)
                CurrentRegion = len(self.FdRegions) - 1

            elif line.strip().lower() == "nested fv":
                #nested fv region.  just track the module info
                Region = FdRegion()
                Region.Nested = "True"
                InFdRegion = True
                self.FdRegions.append(Region)
                CurrentRegion = len(self.FdRegions) -1


            #parse within fd region
            elif InFdRegion:
                tokens = line.strip().split()
                if len(tokens) > 0:
                    if tokens[0].strip() == 'Type:':
                        (self.FdRegions[CurrentRegion]).Type = tokens[1]
                        (self.FdRegions[CurrentRegion]).Desc = (self.FdRegions[CurrentRegion]).Type
                    elif tokens[0].strip() == 'Base':
                        (self.FdRegions[CurrentRegion]).Base = tokens[2]
                        bv = int(tokens[2], 0)
                        (self.FdRegions[CurrentRegion]).SystemAddress = str("0x%X" % (self.FdBaseNum + bv))
                    elif tokens[0].strip() == 'Size:':
                        (self.FdRegions[CurrentRegion]).Size = tokens[1]
                    elif tokens[0].strip() == 'Occupied':
                        (self.FdRegions[CurrentRegion]).Used = tokens[2]
                    elif tokens[0].strip() == 'Free':
                        (self.FdRegions[CurrentRegion]).Free = tokens[2]
                    elif (tokens[0].strip() == 'Fv'):
                        if(tokens[1].strip() == 'Name:'):
                            if len((self.FdRegions[CurrentRegion]).Desc) > len((self.FdRegions[CurrentRegion]).Type):
                                (self.FdRegions[CurrentRegion]).Desc =  (self.FdRegions[CurrentRegion]).Desc + " --> FV: " + tokens[2]
                            else:
                                (self.FdRegions[CurrentRegion]).Desc =  "FV: " + tokens[2]

                    elif  (tokens[0].strip() == 'Capsule'):
                        if (tokens[1].strip() == "Name:"):
                            (self.FdRegions[CurrentRegion]).Desc = "Capsule: " + tokens[2]
                        elif (tokens[1].strip() == 'Size:'):
                            (self.FdRegions[CurrentRegion]).Used = tokens[2]
                            filed = int((self.FdRegions[CurrentRegion]).Used, 0)
                            total = int((self.FdRegions[CurrentRegion]).Size, 0)
                            free = total - filed
                            (self.FdRegions[CurrentRegion]).Free = str("0x%X" % free)

                    elif  (tokens[0].strip() == 'File'):
                        if (tokens[1].strip() == "Name:"):
                            (self.FdRegions[CurrentRegion]).Desc = "File: " + tokens[2]
                        elif (tokens[1].strip() == 'Size:'):
                            (self.FdRegions[CurrentRegion]).Used = tokens[2]
                            filed = int((self.FdRegions[CurrentRegion]).Used, 0)
                            total = int((self.FdRegions[CurrentRegion]).Size, 0)
                            free = total - filed
                            (self.FdRegions[CurrentRegion]).Free = str("0x%X" % free)


                    elif tokens[0].strip() == '<---------------------------------------------------------------------------------------------------------------------->':
                        InFdRegion = False
                        if (self.FdfMiniParser != None):
                            #try to get comment for region
                            desc = self.FdfMiniParser.GetRegionDescComment(int((self.FdRegions[CurrentRegion]).Base, 0))
                            if len (desc) > 0:
                                (self.FdRegions[CurrentRegion]).Desc = (self.FdRegions[CurrentRegion]).Desc + " - " + desc
                    elif len(tokens) == 3 and tokens[0].lower().startswith("0x"):
                        #This is the module section
                        mo = FvModule()
                        mo.Name = tokens[1]
                        mo.Path = tokens[2]
                        if(mo.Path.startswith('(') and not mo.Path.endswith(')')):
                           #build report line wrapped around.  Go get next line
                           CurrentLine += 1
                           mo.Path = mo.Path.strip() + self.ReportFile[CurrentLine].strip()
                        mo.Path = mo.Path.rstrip(')').lstrip('(')
                        self.FdRegions[CurrentRegion].FvModuleList.append(mo)

            #increment the currentline
            CurrentLine = CurrentLine + 1

    #
    #loop thru all the mods and all the fv regions trying to match up
    #
    def UpdateModsWithFvs(self, ws):
        #go thru each Module Summary
        for ms in self.Modules:
            logging.debug("Looking for FVs for Mod named %s path: %s" % (ms.Name, ms.InfPath))
            #loop thru all modules in all FVs to find all matches
            for fv in self.FdRegions:
                for modinfv in fv.FvModuleList:
                    #to much output logging.debug("Comparing against mod %s" % modinfv.Name)
                    if(ms.Name.lower() == modinfv.Name.lower()):
                        #possible match
                        logging.debug("Name Match.  P1 %s p2 %s" % (ms.InfPath.lower().replace("/", "\\"), modinfv.Path.lower().replace("/", "\\")))
                        #if(os.path.join(ws, ms.InfPath).lower().replace("/", "\\") == modinfv.Path.lower().replace("/", "\\") ):
                        if((modinfv.Path.lower().replace("/", "\\")).endswith(ms.InfPath.lower().replace("/", "\\")) ):
                            #good match
                            logging.debug("Match found for ModInFV.  Mod %s is in FV %s" % (ms.InfPath, fv.Desc))
                            if(fv.Desc not in ms.FvList):
                                ms.FvList.append(fv.Desc)
                            break #break out of this FV



    def JS(self, key, value, comma=True):
        r = '"' + key.strip() + '": "' + value.strip() + '"'
        if comma:
             r = r + ","
        return r

    def JSRegion(self, region):
        return "{" + self.JS("nested", region.Nested) + self.JS("description", region.Desc) + self.JS("systemAddress", region.SystemAddress) + self.JS("baseAddress", region.Base) + self.JS("size", region.Size) + self.JS("used", region.Used) + self.JS("usedPercentage", region.GetUsedPercent()) + self.JS("free", region.Free) + self.JS("freePercentage", region.GetFreePercent(), False) + "},"

    def JSModule(self, mod):
        fvlistjson = '"FV":['
        for f in mod.FvList:
            fvlistjson += "'" + f + "',"
        fvlistjson = fvlistjson.rstrip(',') + ']'

        return "{" + self.JS("name", mod.Name) + self.JS("path", mod.InfPath) + self.JS("guid", mod.Guid) + self.JS("size", str(mod.Size)) + self.JS("sizeString", mod.SizeString) + self.JS("type", mod.Type) + fvlistjson + "},"

    def ToJsonString(self):
        js = "{" + self.JS("FdSizeReportGeneratorVersion", self.ToolVersion) + self.JS("ProductName", self.ProductName) 
        js += self.JS("ProductVersion", self.ProductVersion) + self.JS("DateCollected", datetime.datetime.strftime(datetime.datetime.now(), "%A, %B %d, %Y %I:%M%p" ))
        js += self.JS("fdName", self.FdName) + self.JS("fdBase", self.FdBase) + self.JS("fdSize", self.FdSize) + '"fdRegions": ['
        for fdr in self.FdRegions:
            js = js + self.JSRegion(fdr)

        js = js.rstrip(",")
        js = js + '], "modules": ['

        for mod in self.Modules:
            js = js + self.JSModule(mod)
        js = js.rstrip(",")
        js = js + "]}"
        return js


class FdReport(object):
    MY_FOLDER = os.path.dirname(os.path.realpath(__file__))
    VERSION = "2.02"

    def __init__(self):
        pass

    def MakeReport(self, ProductName, ProductVersion, OutputReport, InputFdf, InputBuildReport, Workspace, PackagesPathList=[]):

        if not os.path.isfile(InputFdf):
            logging.critical("Invalid path to input FDF file")
            return -2

        if not os.path.isfile(InputBuildReport):
            logging.critical("Invalid path to input Build Report file")
            return -1

        if not os.path.isdir(Workspace):
            logging.critical("Invalid path to workspace dir")
            return -3


        f = open(InputBuildReport, "r")
        lines = f.readlines()
        f.close()

        f = open(InputFdf, "r")
        fdfLines = f.readlines()
        f.close()

        rep = FlashReportParser(lines, fdfLines, FdReport.VERSION, ProductName, ProductVersion)
        rep.ParseFdInfo()
        rep.ParseFdRegions()
        rep.ParseAllModules()
        rep.UpdateModsWithFvs(Workspace)


        f = open(OutputReport, "w")
        template = open(os.path.join(FdReport.MY_FOLDER, "FdReport_Template.html"), "r")
        for line in template.readlines():
            if "%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%" in line:
                line = line.replace("%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%", rep.ToJsonString())
            f.write(line)
        template.close()
        f.close()

        return 0


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
    parser.add_option("-w", dest="Workspace", help="Absolute path to workspace", default=None)
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

    if not options.Workspace:
        logging.critical("No workspace specified")
        sys.exit(-4)

    Report = FdReport()
    ret = Report.MakeReport(options.ProductName, options.FirmwareVersion, options.OutputFile, options.FdfFile, options.InputReport, options.Workspace)
    logging.debug("Build Report generation returned %d" % ret)

    if ret != 0:
        logging.critical("Tool Failed.  Return Code: %i" % ret)
    #end logging
    logging.shutdown()
    sys.exit(ret)
