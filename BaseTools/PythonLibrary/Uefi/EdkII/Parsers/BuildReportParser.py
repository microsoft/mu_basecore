## @file BuildReportParser.py
# Code to help parse an EDk2 Build Report
##
# Copyright (c) 2016, Microsoft Corporation
#
# All rights reserved.
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
### 
import os
import logging

from UtilityFunctions import *
from Uefi.EdkII.PathUtilities import *

#
# Class to represent a module within the Build Report
#
class ModuleSummary(object):
    def __init__(self, content, ws, packagepatahlist):
        self._RawContent = content
        self.Guid = ""
        self.Name = ""
        self.InfPath = ""
        self.Type = ""
        self.PCDs = {}
        self.Libraries = {}
        self.Depex = ""
        self.WorkspacePath = ws
        self.PackagePathList = packagepatahlist
        self.FvName = None


    def Parse(self):
        inPcdSection = False
        inLibSection = False
        inDepSection = False
        nextLineSection = False
        tokenspace = ""
        pathConverter = Edk2Path(self.WorkspacePath, self.PackagePathList)

        i = 0
        try:

            while i < len(self._RawContent):
                li = self._RawContent[i].strip()

                #parse start and end
                if(li == ">----------------------------------------------------------------------------------------------------------------------<"):
                    nextLineSection = True

                elif(li == "<---------------------------------------------------------------------------------------------------------------------->"):
                    inPcdSection = False
                    inLibSection = False
                    inDepSection = False
                    nextLineSection = False

                #parse section header
                elif(nextLineSection):
                    nextLineSection = False
                    if(li == "Library"):
                        inLibSection = True
                        i +=1  #add additional line to skip the dashed line
                    
                    elif(li == "PCD"):
                        inPcdSection = True
                        i +=1  #add additional line to skip the dashed line

                    elif(li == "Final Dependency Expression (DEPEX) Instructions"):
                        inDepSection = True
                        i +=1  #add additional line to skip the dashed line
                    else:
                        logging.debug("Unsupported Section: " + li)
                        inPcdSection = False
                        inLibSection = False
                        inDepSection = False

                #Normal section parsing
                else:
                    if(inLibSection):
                        logging.debug("InLibSection: %s" % li)
                        #get the whole statement library class statement
                        templine = li.strip()
                        while( '}' not in templine):
                            i += 1
                            templine += self._RawContent[i].strip()
                        
                        #have good complete line with no whitespace/newline chars                       
                        #first is the library instance INF
                        #second is the library class

                        libc = templine.partition('{')[2].partition('}')[0].partition(':')[0].strip()
                        libi = templine.partition('{')[0].strip()



                        #Take absolute path and convert to EDK build path
                        RelativePath = pathConverter.GetEdk2RelativePathFromAbsolutePath(libi)
                        if(RelativePath is not None):
                            self.Libraries[libc] = RelativePath
                        else:
                            self.Libraries[libc] = libi
                        i += 1
                        continue

                    elif(inPcdSection):
                        #this is the namespace token line
                        if(len(li.split()) == 1):
                            tokenspace = li

                        #this is the main line of the PCD value
                        elif(li.count("=") == 1 and li.count(":") ==1):
                            while(li.count("\"") % 2) != 0:
                                i +=1
                                li += " " + self._RawContent[i].rstrip()
                            while(li.count('{') != li.count('}')):
                                i+=1
                                li += " " + self._RawContent[i].rstrip()

                            token = li.partition('=')[2]
                            token2 = li.partition(':')[0].split()[-1]
                            self.PCDs[tokenspace + "." + token2] = token.strip()

                        #this is the secondary lines of PCD values showing Defaults
                        elif li.count(":") == 0 and li.count("=") ==1:
                            while(li.count("\"") % 2) != 0:
                                i +=1
                                li += self._RawContent[i].rstrip()
                        

                    elif(inDepSection):
                        pass
                        #not implemented right now
                        
                    else:
                        #not in section...Must be header section
                        l = li.partition(':')
                        if(l[2] == ""):
                            pass  #not a name: value pair
                        else:
                            key = l[0].strip().lower()
                            value = l[2].strip()
                            if(key == "module name"):
                                logging.debug("Parsing Mod: %s" % value)
                                self.Name = value
                            elif(key == "module inf path"):
                                self.InfPath = value
                            elif(key == "file guid"):
                                self.Guid = value
                            elif(key == "driver type"):
                                value = value.strip()
                                self.Type = value[value.index('(')+1:-1]

                i += 1
        except Exception:
            logging.debug("Exception in Parsing: %d" % i)
            raise

#
# Class to parse and objectify the Build report so that 
# tools can interact with the Build Report.  
# This should simplify the Build Report based interactions
# but should not contain tool specific logic or tests. 
#
class BuildReport(object):
    RegionTypes = Enum(['PCD', 'FD', 'MODULE', 'UNKNOWN']) 
    
    def __init__(self, filepath, ws, packagepathcsv, protectedWordsDict):
        self.PlatformName = ""
        self.DscPath = ""
        self.FdfPath = ""
        self.BuildOutputDir = ""
        self.ReportFile = filepath
        self.Modules = { }  # fill this in with objects of ModuleSummary type
        self._ReportContents = ""
        self._Regions = []  # fill this in with tuple (type, start, end)
        self.Workspace = ws  #needs to contain the trailing slash
        self.PackagePathList = []
        for a in packagepathcsv.split(","):
            a = a.strip()
            if(len(a) > 0):
                self.PackagePathList.append(a)
        self.ProtectedWords = protectedWordsDict
        self.PathConverter = Edk2Path(self.Workspace, self.PackagePathList)
  
    #
    # do region level parsing
    # to get the layout, lists, and dictionaries setup. 
    #
    def BasicParse(self):
        if(not os.path.isfile(self.ReportFile)):
            raise Exception("Report File path invalid!")

        #read report
        f = open(self.ReportFile, "r")
        self._ReportContents = [x.strip() for x in f.readlines()]
        f.close()
        #
        #replace protected words
        #
        for (k,v) in self.ProtectedWords.items():
            self._ReportContents = [x.replace(k, v) for x in self._ReportContents]

        logging.debug("Report File is: %s" % self.ReportFile)
        logging.debug("Input report had %d lines of content" % len(self._ReportContents))
 
        #
        #parse thru and find the regions and basic info at top
        # this is a little hacky in that internal operations could
        # fail but it doesn't seem critical
        #
        linenum = self._GetNextRegionStart(0)
        while(linenum != None):
            start = linenum
            end = self._GetEndOfRegion(start)
            type = self._GetRegionType(start)
            self._Regions.append( (type, start, end))
            linenum = self._GetNextRegionStart(linenum)
            logging.debug("Found a region of type: %s start: %d end: %d" % (BuildReport.RegionTypes[type], start, end))

        #
        # Parse the basic header of the report.  
        # we do it after parsing region because we
        # can limit scope to 0 - first start
        #
        for n in range(0, self._Regions[0][1] ) :  #loop thru from 0 to start of first region
            l = self._ReportContents[n].strip()
            li = l.partition(':')
            if(li[2] == ""):
                continue

            key = li[0].strip().lower()
            value = li[2].strip()

            if(key == "platform name"):
                self.PlatformName = value
            elif(key == "platform dsc path"):
                self.DscPath = value
            elif(key == "output path"):
                self.BuildOutputDir = value

        #
        # now for each module summary
        # parse it
        for r in self._Regions:
            if(r[0] == BuildReport.RegionTypes.MODULE):
                mod = ModuleSummary(self._ReportContents[r[1]:r[2]], self.Workspace, self.PackagePathList)
                mod.Parse()
                self.Modules[mod.Guid] = mod

        #now that all modules are parsed lets parse the FD region so we can get the FV name for each module
        for r in self._Regions:
            #if FD region parse out all INFs in the all of the flash
            if(r[0] == BuildReport.RegionTypes.FD):
                self._ParseFdRegionForModules(self._ReportContents[r[1]:r[2]])
  
    def FindComponentByInfPath(self, InfPath):
        for (k,v) in self.Modules.items():
            if(v.InfPath.lower() == InfPath.lower()):
                logging.debug("Found Module by InfPath: %s" % InfPath)
                return v
        
        logging.error("Failed to find Module by InfPath %s" % InfPath)
        return None



    def _ParseFdRegionForModules(self, rawcontents):
        FvName = None
        index = 0
        WorkspaceAndPPList = [self.Workspace]
        WorkspaceAndPPList.extend(self.PackagePathList)

        while index < len(rawcontents):
            a = rawcontents[index]
            tokens = a.split()
            if a.startswith("0x") and (len(tokens) == 3) and (a.count('(') == 1):
                if ".inf" not in a.lower() or (a.count('(') != a.count(")")):
                    a = a + rawcontents[index+1].strip()
                    index += 1
                    tokens = a.split()

                i = a.split()[2].strip().strip('()')


                logging.debug("Found INF in FV Region: " + i)

                #Take absolute path and convert to EDK build path
                RelativePath = self.PathConverter.GetEdk2RelativePathFromAbsolutePath(i)
                if(RelativePath is not None):
                    comp = self.FindComponentByInfPath(RelativePath)
                    if comp is not None:
                        comp.FvName = FvName
                    else:
                        logging.error("Failed to find component for INF path %a" % RelativePath)


            elif a.startswith("Fv Name:"):
                #Fv Name:            FVDXE (99.5% Full)
                FvName = a.partition(":")[2].strip().split()[0]
                logging.debug("Found FvName. RAW: %s  Name: %s" % (a, FvName))
            else:
                logging.debug("ignored line in FD parsing: %s" % a)
            index += 1

        return

    #
    # Get the start of region
    #
    def _GetNextRegionStart(self, number):
        lineNumber = number
        while(lineNumber < len(self._ReportContents)):
            if(self._ReportContents[lineNumber] == ">======================================================================================================================<"):
                return lineNumber+1
            lineNumber += 1
        logging.debug("Failed to find a Start Next Region after lineNumber: %d" % number)
        #didn't find new region
        return None

    # 
    # Get the end of region
    #
    def _GetEndOfRegion(self, number):
        lineNumber = number
        while(lineNumber < len(self._ReportContents)):
            if(self._ReportContents[lineNumber] == "<======================================================================================================================>"):
                return lineNumber-1
            lineNumber += 1
        
        logging.debug("Failed to find a End Region after lineNumber: %d" % number)
        #didn't find new region
        return None

    def _GetRegionType(self, lineNumber):
        l = self._ReportContents[lineNumber].strip()
        if(l == "Firmware Device (FD)"):
            return BuildReport.RegionTypes.FD
        elif(l == "Platform Configuration Database Report"):
            return BuildReport.RegionTypes.PCD
        elif(l == "Module Summary"):
            return BuildReport.RegionTypes.MODULE
        else:
            return BuildReport.RegionTypes.UNKNOWN

#
# Test code to validate the parser and classes
#
''' if __name__ == '__main__':
    brp = r"C:\src\Devices\Build\MsftIvanhoePkg\DEBUG_VS2015x86\BUILD_REPORT.TXT"
    #brp = r"C:\src\Devices\Build\MsftKingsburgPkg\DEBUG_VS2015x86\BUILD_REPORT.TXT"
    wsp = r"C:\src\Devices"
    ppcsv = os.path.join(wsp, "SM_UDK") + "," + os.path.join(wsp, "SURF_KBL") + "," + os.path.join(wsp, "SM_INTEL_KBL")
    plats = {"MsftKingsburgPkg": "ThePlatformPkg", "MsftIvanhoePkg": "ThePlatformPkg"}
    br = BuildReport(brp, wsp, ppcsv, plats)
    br.BasicParse()

    #output as text in a stable and comparable manner
    #1 - sort modules
    #2 - print modules
    
    for k in sorted(br.Modules.iterkeys()):
        mod = br.Modules[k]
        #print mod
        print("--Module--Start: %s" % mod.Guid)
        print("  Guid:   %s" % mod.Guid)
        print("  Name:   %s" % mod.Name)
        print("  Inf:    %s" % mod.InfPath)
        print("  Type:   %s" % mod.Type)
        print("  FvName: %s" % mod.FvName)
        print("  Depex:  %s" % mod.Depex)
        print("  ==PCDs==")
        for a in sorted(mod.PCDs.iterkeys()):
            pcd = mod.PCDs[a]
            print("    Name: %s  Value: %s" % (a, str(pcd)))
        print("  ==LIBs==")
        for a in sorted(mod.Libraries.iterkeys()):
            lib = mod.Libraries[a]
            print("    Class: %s  Instance: %s" % (a, lib))
        print("--Module--End: %s" % mod.Guid)
        print("")
        print("")
    a = 1 '''
