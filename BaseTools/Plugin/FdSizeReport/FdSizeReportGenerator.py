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
            FdfF = thebuilder.mws.join(thebuilder.ws, thebuilder.env.GetValue("FLASH_DEFINITION"))
            #3 - Get the product name
            Product = thebuilder.env.GetValue("PRODUCT_NAME")
            #4 - Get the Fw version
            FwVersion = thebuilder.env.GetBuildValue("BUILDID_STRING")
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


class FlashReportParser(object):

    def __init__(self, ReportFileLines, FdfFileLines = None, ToolVersion="1.00", ProductName="Unset", ProductVersion="Unset"):
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

    def ParseFdInfo(self):
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
