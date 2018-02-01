from Uefi.EdkII.Parsers.BaseParser import HashFileParser
import os

class DecParser(HashFileParser):
    def __init__(self):
        HashFileParser.__init__(self, 'DecParser')
        self.Lines = []
        self.Parsed = False
        self.Dict = {}
        self.LibrariesUsed = []
        self.PPIsUsed = []
        self.ProtocolsUsed = []
        self.GuidsUsed = []
        self.PcdsUsed = []
        self.IncludesUsed = []
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
        InLibraryClassSection = False
        InProtocolsSection = False
        InGuidsSection = False
        InPPISection = False
        InPcdSection = False
        InIncludesSection = False

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
                        continue


            elif InLibraryClassSection:
                if l.strip()[0] == '[':
                   InLibraryClassSection = False
                else:
                   t = l.partition("|")
                   self.LibrariesUsed.append(t[0].strip())
                   continue

            elif InProtocolsSection:
                if l.strip()[0] == '[':
                   InProtocolsSection = False
                else:
                   t = l.partition("=")
                   self.ProtocolsUsed.append(t[0].strip())
                   continue

            elif InGuidsSection:
                if l.strip()[0] == '[':
                   InGuidsSection = False
                else:
                   t = l.partition("=")
                   self.GuidsUsed.append(t[0].strip())
                   continue

            elif InPcdSection:
                if l.strip()[0] == '[':
                   InPcdSection = False
                else:
                   t = l.partition("|")
                   self.PcdsUsed.append(t[0].strip())
                   continue

            elif InIncludesSection:
                if l.strip()[0] == '[':
                   InIncludesSection = False
                else:
                   self.IncludesUsed.append(l.strip())
                   continue

            elif InPPISection:
                if (l.strip()[0] == '['):
                    InPPISection = False
                else:
                    t = l.partition("=")
                    self.PPIsUsed.append(t[0].strip())
                    continue

            # check for different sections
            if l.strip().lower().startswith('[defines'):
                InDefinesSection = True

            elif l.strip().lower().startswith('[libraryclasses'):
                InLibraryClassSection = True

            elif l.strip().lower().startswith('[protocols'):
                InProtocolsSection = True

            elif l.strip().lower().startswith('[guids'):
                InGuidsSection = True

            elif l.strip().lower().startswith('[ppis'):
                InPPISection = True

            elif l.strip().lower().startswith('[pcd'):
                InPcdSection = True

            elif l.strip().lower().startswith('[includes'):
                InIncludesSection = True

        self.Parsed = True