## @file InfParser.py
# Code to help parse EDK2 INF files
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
from Uefi.EdkII.Parsers.BaseParser import HashFileParser
import os


AllPhases = ["SEC", "PEIM", "PEI_CORE", "DXE_DRIVER", "DXE_CORE", "DXE_RUNTIME_DRIVER", "UEFI_DRIVER", "SMM_CORE", "DXE_SMM_DRIVER", "UEFI_APPLICATION"]

class InfParser(HashFileParser):

    def __init__(self):
        HashFileParser.__init__(self, 'ModuleInfParser')
        self.Lines = []
        self.Parsed = False
        self.Dict = {}
        self.LibraryClass = ""
        self.SupportedPhases = []
        self.PackagesUsed = []
        self.LibrariesUsed = []
        self.ProtocolsUsed = []
        self.GuidsUsed = []
        self.PpisUsed = []
        self.PcdsUsed = []
        self.Sources = []
        self.Binaries = []
        self.Path = ""

    def ParseFile(self, filepath):
        self.Logger.debug("Parsing file: %s" % filepath)
        if(not os.path.isabs(filepath)):
            fp = self.FindPath(filepath)
        else:
            fp = filepath
        self.Path = fp
        f = open(fp, "r")
        self.Lines = f.readlines()
        f.close()
        InDefinesSection = False
        InPackagesSection = False
        InLibraryClassSection = False
        InProtocolsSection = False
        InGuidsSection = False
        InPpiSection = False
        InPcdSection = False
        InSourcesSection = False
        InBinariesSection = False

        for l in self.Lines:
            l = self.StripComment(l)

            if(l == None or len(l) < 1):
                continue

            if InDefinesSection:
                if l.strip()[0] == '[':
                    InDefinesSection = False
                else:
                    if l.count("=") == 1:
                        tokens = l.split('=', 1)
                        self.Dict[tokens[0].strip()] = tokens[1].strip()
                        #
                        # Parse Library class and phases in special manor
                        #
                        if(tokens[0].strip().lower() == "library_class"):
                            self.LibraryClass = tokens[1].partition("|")[0].strip()
                            self.Logger.debug("Library class found")
                            if(len(tokens[1].partition("|")[2].strip()) < 1):
                                self.SupportedPhases = AllPhases
                            elif(tokens[1].partition("|")[2].strip().lower() == "base"):
                                 self.SupportedPhases = AllPhases
                            else:
                                self.SupportedPhases = tokens[1].partition("|")[2].strip().split()

                        self.Logger.debug("Key,values found:  %s = %s"%(tokens[0].strip(), tokens[1].strip()))

                        continue

            elif InPackagesSection:
                if l.strip()[0] == '[':
                   InPackagesSection = False
                else:
                   self.PackagesUsed.append(l.partition("|")[0].strip())
                   continue

            elif InLibraryClassSection:
                if l.strip()[0] == '[':
                   InLibraryClassSection = False
                else:
                   self.LibrariesUsed.append(l.partition("|")[0].strip())
                   continue

            elif InProtocolsSection:
                if l.strip()[0] == '[':
                   InProtocolsSection = False
                else:
                   self.ProtocolsUsed.append(l.partition("|")[0].strip())
                   continue

            elif InGuidsSection:
                if l.strip()[0] == '[':
                   InGuidsSection = False
                else:
                   self.GuidsUsed.append(l.partition("|")[0].strip())
                   continue

            elif InPcdSection:
                if l.strip()[0] == '[':
                   InPcdSection = False
                else:
                   self.PcdsUsed.append(l.partition("|")[0].strip())
                   continue

            elif InPpiSection:
                if l.strip()[0] == '[':
                   InPpiSection = False
                else:
                   self.PpisUsed.append(l.partition("|")[0].strip())
                   continue

            elif InSourcesSection:
                if l.strip()[0] == '[':
                   InSourcesSection = False
                else:
                   self.Sources.append(l.partition("|")[0].strip())
                   continue

            elif InBinariesSection:
                if l.strip()[0] == '[':
                   InBinariesSection = False
                else:
                   self.Binaries.append(l.partition("|")[0].strip())
                   continue

            # check for different sections
            if l.strip().lower().startswith('[defines'):
                InDefinesSection = True

            elif l.strip().lower().startswith('[packages'):
                InPackagesSection = True

            elif l.strip().lower().startswith('[libraryclasses'):
                InLibraryClassSection = True

            elif l.strip().lower().startswith('[protocols'):
                InProtocolsSection = True

            elif l.strip().lower().startswith('[ppis'):
                InPpiSection = True

            elif l.strip().lower().startswith('[guids'):
                InGuidsSection = True

            elif l.strip().lower().startswith('[pcd') or l.strip().lower().startswith('[patchpcd') or l.strip().lower().startswith('[fixedpcd') or l.strip().lower().startswith('[featurepcd'):
                InPcdSection = True

            elif l.strip().lower().startswith('[sources'):
                InSourcesSection = True

            elif l.strip().lower().startswith('[binaries'):
                InBinariesSection = True

        self.Parsed = True