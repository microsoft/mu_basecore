from Uefi.EdkII.Parsers.BaseParser import HashFileParser
import os

class DscParser(HashFileParser):

    def __init__(self):
        super(DscParser, self).__init__('DscParser')
        self.SixMods = []
        self.ThreeMods = []
        self.OtherMods = []
        self.Libs = []
        self.ParsingInBuildOption = 0
        self.LibraryClassToInstanceDict = {}
        self.Pcds = []

    def __ParseLine(self, Line):
        l = self.StripComment(Line).strip()
        if(len(l) < 1):
            return ("", [])

        li = self.ReplaceVariables(l)
        if(self.ProcessConditional(li)):
            #was a conditional
            ## Other parser returns li, [].  Need to figure out which is right
            return ("", [])
        
        #not conditional keep procesing

        #check if conditional is active
        if(not self.InActiveCode()):
            return ("", [])

        #check for include file and import lines from file
        if(li.strip().lower().startswith("!include")):
            #include line.
            toks= li.split()
            self.Logger.debug("Opening Include File %s" % os.path.join(self.RootPath, toks[1]))
            sp = self.FindPath(toks[1])
            lf = open(sp, "r")
            loc = lf.readlines()
            lf.close()
            return ("", loc)

        #check for new section
        (IsNew, Section) = self.ParseNewSection(li)
        if(IsNew):
            self.CurrentSection = Section.upper()
            self.Logger.debug("New Section: %s" % self.CurrentSection)
            self.Logger.debug("FullSection: %s" % self.CurrentFullSection)
            return (li, [])

        #process line based on section we are in
        if(self.CurrentSection == "DEFINES") or (self.CurrentSection == "BUILDOPTIONS"):
            if li.count("=") >= 1:
                tokens = li.split("=", 1)
                leftside = tokens[0].split()
                if(len(leftside) == 2):
                    left = leftside[1]
                else:
                    left = leftside[0]
                right = tokens[1].strip()
                    
                self.LocalVars[left] = right
                self.Logger.debug("Key,values found:  %s = %s"%(left, right))
                return (li, [])

        #process line in x64 components    
        elif(self.CurrentFullSection.upper() == "COMPONENTS.X64"):
            if(self.ParsingInBuildOption > 0):
                if(".inf" in li.lower()):
                    p = self.ParseInfPathLib(li)
                    self.Libs.append(p)
                    self.Logger.debug("Found Library in a 64bit BuildOptions Section: %s" % p)
                elif("tokenspaceguid" in li.lower() and (li.count('|') > 0) and (li.count('.') > 0) ):
                    #should be a pcd statement
                    p = li.partition('|')
                    self.Pcds.append(p[0].strip())
                    self.Logger.debug("Found a Pcd in a 64bit Module Override section: %s" % p[0].strip())
            else:
                if(".inf" in li.lower()):
                    p = self.ParseInfPathMod(li)
                    self.SixMods.append(p)
                    self.Logger.debug("Found 64bit Module: %s" % p)
                    
            self.ParsingInBuildOption = self.ParsingInBuildOption + li.count("{")
            self.ParsingInBuildOption = self.ParsingInBuildOption - li.count("}")
            return (li, [])

        #process line in ia32 components
        elif(self.CurrentFullSection.upper() == "COMPONENTS.IA32"):
            if(self.ParsingInBuildOption > 0):               
                if(".inf" in li.lower()):
                    p = self.ParseInfPathLib(li)
                    self.Libs.append(p)
                    self.Logger.debug("Found Library in a 32bit BuildOptions Section: %s" % p)
                elif("tokenspaceguid" in li.lower() and (li.count('|') > 0) and (li.count('.') > 0) ):
                    #should be a pcd statement
                    p = li.partition('|')
                    self.Pcds.append(p[0].strip())
                    self.Logger.debug("Found a Pcd in a 32bit Module Override section: %s" % p[0].strip())

            else:
                if(".inf" in li.lower()):
                    p = self.ParseInfPathMod(li)
                    self.ThreeMods.append(p)
                    self.Logger.debug("Found 32bit Module: %s" % p)
                    
            self.ParsingInBuildOption = self.ParsingInBuildOption + li.count("{")
            self.ParsingInBuildOption = self.ParsingInBuildOption - li.count("}")
            return (li, [])

         #process line in other components
        elif("COMPONENTS" in self.CurrentFullSection.upper()):
            if(self.ParsingInBuildOption > 0):               
                if(".inf" in li.lower()):
                    p = self.ParseInfPathLib(li)
                    self.Libs.append(p)
                    self.Logger.debug("Found Library in a BuildOptions Section: %s" % p)
                elif("tokenspaceguid" in li.lower() and (li.count('|') > 0) and (li.count('.') > 0) ):
                    #should be a pcd statement
                    p = li.partition('|')
                    self.Pcds.append(p[0].strip())
                    self.Logger.debug("Found a Pcd in a Module Override section: %s" % p[0].strip())

            else:
                if(".inf" in li.lower()):
                    p = self.ParseInfPathMod(li)
                    self.OtherMods.append(p)
                    self.Logger.debug("Found Module: %s" % p)
                    
            self.ParsingInBuildOption = self.ParsingInBuildOption + li.count("{")
            self.ParsingInBuildOption = self.ParsingInBuildOption - li.count("}")
            return (li, [])

        #process line in library class section (don't use full name)
        elif(self.CurrentSection.upper() == "LIBRARYCLASSES"):
            if(".inf" in li.lower()):
                p = self.ParseInfPathLib(li)
                self.Libs.append(p)
                self.Logger.debug("Found Library in Library Class Section: %s" % p)
            return (li, [])
        #process line in PCD section
        elif(self.CurrentSection.upper().startswith("PCDS")):
            if("tokenspaceguid" in li.lower() and (li.count('|') > 0) and (li.count('.') > 0) ):
                #should be a pcd statement
                p = li.partition('|')
                self.Pcds.append(p[0].strip())
                self.Logger.debug("Found a Pcd in a PCD section: %s" % p[0].strip())
            return (li, [])                      
        else:
            return (li, [])

    def ParseInfPathLib(self, line):
        if(line.count("|") > 0):
            l = []
            c = line.split("|")[0].strip()
            i = line.split("|")[1].strip()
            if(c in self.LibraryClassToInstanceDict):
                l = self.LibraryClassToInstanceDict.get(c)
            sp = self.FindPath(i)
            l.append(sp)
            self.LibraryClassToInstanceDict[c] = l
            return line.split("|")[1].strip()
        else:
            return line.strip().split()[0]
        

    def ParseInfPathMod(self, line):
        return line.strip().split()[0].rstrip("{")

    def __ProcessMore(self, lines):
        if(len(lines) > 0):
            for l in lines:
                (line, add) = self.__ParseLine(l)
                if(len(line) > 0):
                    self.Lines.append(line)
                self.__ProcessMore(add)
            
    def ParseFile(self, filepath):
        self.Logger.debug("Parsing file: %s" % filepath)
        f = open(os.path.join(filepath), "r")
        #expand all the lines and include other files
        self.__ProcessMore(f.readlines())
        f.close()
        self.Parsed = True        

    def GetMods(self):
        return self.ThreeMods + self.SixMods

    def GetLibs(self):
        return self.Libs