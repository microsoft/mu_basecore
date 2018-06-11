import os
import logging

class BaseParser(object):
    
    def __init__(self, log):
        self.Logger = logging.getLogger(log)
        self.Lines = []
        self.LocalVars = {}
        self.InputVars = {}
        self.CurrentSection = ""
        self.CurrentFullSection = ""
        self.Parsed = False
        self.ConditionalStack = []
        self.RootPath = ""
        self.PPs = []
        self.TargetFile = None
        self.TargetFilePath = None
        

    #
    # For include files set the base root path
    #
    def SetBaseAbsPath(self, path):
        self.RootPath = path
        return self

    def SetPackagePaths(self, pps = []):
        self.PPs = pps
        return self

    def SetInputVars(self, inputdict):
        self.InputVars = inputdict
        return self

    def FindPath(self, *p):
        # NOTE: Some of this logic should be replaced
        #       with the path resolution from Edk2Module code.

        # If the absolute path exists, return it.
        Path = os.path.join(self.RootPath, *p)
        if os.path.exists(Path):
            return Path

        # If that fails, check a path relative to the target file.
        if self.TargetFilePath is not None:
            Path = os.path.join(self.TargetFilePath, *p)
            if os.path.exists(Path):
                return Path

        # If that fails, check in every possible Pkg path.
        for Pkg in self.PPs:
            Path = os.path.join(self.RootPath,Pkg, *p)
            if os.path.exists(Path):
                return Path

        # log invalid file path
        Path = os.path.join(self.RootPath, *p)
        self.Logger.error("Invalid file path %s" % Path)
        return Path


    def WriteLinesToFile(self, filepath):
        self.Logger.debug("Writing all lines to file: %s" % filepath)
        f = open(filepath, "w")
        for l in self.Lines:
            f.write(l + "\n")
        f.close()
    #
    # do logical comparisons
    #
    def ComputeResult(self, value, cond, value2):
        if(cond == "=="):
            #equal
            return (value.upper() == value2.upper())

        elif (cond == "!="):
            #not equal
            return (value.upper() != value2.upper())

        elif (cond == "<"):
            return (self.ConvertToInt(value) < (self.ConvertToInt(value2)))

        elif (cond == "<="):
            return (self.ConvertToInt(value) <= (self.ConvertToInt(value2)))

        elif (cond == ">"):
            return (self.ConvertToInt(value) > (self.ConvertToInt(value2)))

        elif (cond == ">="):
            return (self.ConvertToInt(value) >= (self.ConvertToInt(value2)))
        

    #
    # convert to int based on prefix
    #
    def ConvertToInt(self, value):
        if(value.upper().startswith("0X")):
            return int(value, 16)
        else:
            return int(value, 10)
                                              
    #
    # Push new value on stack
    #
    def PushConditional(self, v):
        self.ConditionalStack.append(v)
            

    #
    # Pop conditional and return the value
    #
    def PopConditional(self):
        if(len(self.ConditionalStack) > 0):
            return self.ConditionalStack.pop()
        else:
            self.Logger.critical("Tried to pop an empty conditional stack.  Line Number %d" % self.CurrentLine)
            return self.ConditionalStack.pop()  #this should cause a crash but will give trace.

    #
    #Method to replace variables
    # in a line with their value from input dict or local dict
    #
    def ReplaceVariables(self, line):
        rep = line.count("$")
        result = line
        index = 0
        while(rep > 0):
            start = line.find("$(", index)
            end = line.find(")", start)
           
            token = line[start+2:end]
            retoken = line[start:end+1]
            self.Logger.debug("Token is %s" % token)
            v = self.LocalVars.get(token)
            self.Logger.debug("Trying to replace %s" % retoken)
            if(v != None):
                #
                # fixme: This should just be a workaround!!!!!
                #
                if (v.upper() == "TRUE" or v.upper() == "FALSE"):
                    v = v.upper()
                self.Logger.debug("with %s  [From Local Vars]"% v)
                result = result.replace(retoken, v, 1)
            else:
                #use the passed in Env
                v = self.InputVars.get(token)

                if(v == None):
                    self.Logger.error("Unknown variable %s in  %s" % (token, line))
                    #raise Exception("Invalid Variable Replacement", token)
                    #just skip it because we need to support ifdef
                else:
                    #found in the Env
                    #
                    # fixme: This should just be a workaround!!!!!
                    #
                    if (v.upper() == "TRUE" or v.upper() == "FALSE"):
                        v = v.upper()
                    self.Logger.debug("with %s [From Input Vars]" % v)
                    result = result.replace(retoken, v, 1)

            index = end+1
            rep = rep -1

        return result
            

    #
    #Process Conditional
    # return true if line is a conditional otherwise false
    #
    def ProcessConditional(self, text):
        tokens = text.split()
        if(tokens[0].lower() == "!if"):
            #need to add support for OR/AND
            if(len(tokens) < 4):
                self.Logger.error("!if conditionals need to be formatted correctly (spaces between each token)")
                raise Exception("Invalid conditional", text)
            con = self.ComputeResult(tokens[1].strip(), tokens[2].strip(), tokens[3].strip())
            self.PushConditional(con)
            return True

        elif(tokens[0].lower() == "!ifdef"):
            self.PushConditional((tokens[1].count("$") == 0))
            return True

        elif(tokens[0].lower() == "!ifndef"):
            self.PushConditional((tokens[1].count("$") > 0))
            return True
            
        elif(tokens[0].lower() == "!else"):
            v = self.PopConditional()
            self.PushConditional(not v)
            return True

        elif(tokens[0].lower() == "!endif"):
            self.PopConditional()
            return True

        return False

    #
    #returns true or false depending on what state of conditional you are currently in
    #
    def InActiveCode(self):
        ret = True
        for a in self.ConditionalStack:
            if(a == False):
                ret = False
                break
            
        return ret
            
            

    #
    #will return true if the the line has
    # { 0xD3B36F2C, 0xD551, 0x11D4, { 0x9A, 0x46, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D }}
    #
    def IsGuidString(self, l):
        if(l.count("{") == 2 and l.count("}") == 2 and l.count(",") == 10 and l.count("=") == 1):
            return True
        return False


    def ParseGuid(self, l):
        #parse a guid in format
        #{ 0xD3B36F2C, 0xD551, 0x11D4, { 0x9A, 0x46, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D }}
        #into F7FDE4A6-294C-493c-B50F-9734553BB757  (NOTE these are not same guid this is just example of format)
        entries = l.lstrip(' {').rstrip(' }').split(',')
        gu = entries[0].lstrip(' 0').lstrip('x').strip()
        #pad front until 8 chars
        while(len(gu) < 8):
            gu = "0" + gu

        gut = entries[1].lstrip(' 0').lstrip('x').strip()
        while(len(gut) < 4):
            gut = "0" + gut
        gu = gu + "-" + gut

        gut = entries[2].lstrip(' 0').lstrip('x').strip()
        while(len(gut) < 4):
            gut = "0" + gut
        gu = gu + "-" + gut

        #strip off extra {
        gut = entries[3].lstrip(' { 0').lstrip('x').strip()
        while(len(gut) < 2):
            gut = "0" + gut
        gu = gu + "-" + gut

        gut = entries[4].lstrip(' 0').lstrip('x').strip()
        while(len(gut) < 2):
            gut = "0" + gut
        gu = gu  + gut

        gut = entries[5].lstrip(' 0').lstrip('x').strip()
        while(len(gut) < 2):
            gut = "0" + gut
        gu = gu + "-" + gut

        gut = entries[6].lstrip(' 0').lstrip('x').strip()
        while(len(gut) < 2):
            gut = "0" + gut
        gu = gu + gut

        gut = entries[7].lstrip(' 0').lstrip('x').strip()
        while(len(gut) < 2):
            gut = "0" + gut
        gu = gu + gut

        gut = entries[8].lstrip(' 0').lstrip('x').strip()
        while(len(gut) < 2):
            gut = "0" + gut
        gu = gu + gut

        gut = entries[9].lstrip(' 0').lstrip('x').strip()
        while(len(gut) < 2):
            gut = "0" + gut
        gu = gu + gut

        gut = entries[10].split()[0].lstrip(' 0').lstrip('x').rstrip(' } ').strip()
        while(len(gut) < 2):
            gut = "0" + gut
        gu = gu + gut
        
        return gu.upper()

    def ResetParserState(self):
        self.ConditionalStack = []
        self.CurrentSection = ''
        self.CurrentFullSection = ''
        self.Parsed = False

#
# Base Class for Edk2 build files that use # for comments
#
class HashFileParser(BaseParser):

    def __init__(self, log):
        BaseParser.__init__(self, log)
        
    def StripComment(self, l):
        return l.split('#')[0].strip()

    def ParseNewSection(self, l):
        if(l.count("[") == 1 and l.count("]") ==1):  #new section
            section = l.strip().lstrip("[").split(".")[0].split(",")[0].rstrip("]").strip()
            self.CurrentFullSection = l.strip().lstrip("[").split(",")[0].rstrip("]").strip()
            return (True, section)
        return (False, "")