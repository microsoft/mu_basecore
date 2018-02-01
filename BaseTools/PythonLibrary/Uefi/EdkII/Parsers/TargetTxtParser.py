from Uefi.EdkII.Parsers.BaseParser import HashFileParser
import os

class TargetTxtParser(HashFileParser):

    def __init__(self):
        HashFileParser.__init__(self, 'TargetTxtParser')
        self.Lines = []
        self.Parsed = False
        self.Dict = {}
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

        for l in self.Lines:
            l = self.StripComment(l)

            if(l == None or len(l) < 1):
                continue

            if l.count("=") == 1:
                tokens = l.split('=', 1)
                self.Dict[tokens[0].strip()] = tokens[1].strip()
                self.Logger.debug("Key,values found:  %s = %s"%(tokens[0].strip(), tokens[1].strip()))
                continue

        self.Parsed = True