import logging
import os, sys
import shutil
import subprocess
import time
from Tests.XmlArtifact import XmlOutput

#assume PythonLibrary already on path....required by consumer of this
from Uefi.EdkII.Parsers.DecParser import *
from Uefi.EdkII.Parsers.InfParser import *
from Uefi.EdkII.Parsers.DscParser import *



class BaseTestLibClass(object):
    def __init__(self, workspace, packagespath, args, ignorelist = None, environment = None, summary = None, xmlartifact = None):
        self.ws = workspace
        self.pp = packagespath
        self.Args = args
        self.ignorelist = ignorelist
        self.env = environment
        self.summary = summary

        if xmlartifact is None:
            self.xmlartifact = XmlOutput()
        else:
            self.xmlartifact = xmlartifact

        #INF Parser
        self.ip = InfParser()
        self.ip.SetBaseAbsPath(self.ws)
        self.ip.SetPackagePaths(self.pp)

        #DSC Parser
        self.dp = DscParser()
        self.dp.SetBaseAbsPath(self.ws)
        self.dp.SetPackagePaths(self.pp)

        #DEC Parser
        self.decp = DecParser()
        self.decp.SetBaseAbsPath(self.ws)
        self.decp.SetPackagePaths(self.pp)

    #
    # Function to be overwritten by child class
    #
    @classmethod
    def RunTest(self):
        logging.error("BASE TEST CLASS NOT OVERWRITTEN")
        return -1

    #
    # Walks a directory for all itmes ending in certain extension
    # Default is to walk all of workspace
    #
    def WalkDirectoryForExtension(self, extensionlist, directory=None, ignorelist=None):
        if not isinstance(extensionlist, list):
            logging.critical("Expected list but got " + str(type(extensionlist)))
            return -1

        if directory is None:
            directory = self.ws
        elif not os.path.isdir(directory):
            if os.path.isdir(os.path.join(self.ws, directory)):
                directory = os.path.join(self.ws, directory)
            else:
                logging.critical("Cannot find directory to walk")
                return -1

        if ignorelist is not None:
            ignorelist_lower = list()
            for item in ignorelist:
                ignorelist_lower.append(item.lower())


        extensionlist_lower = list()
        for item in extensionlist:
                extensionlist_lower.append(item.lower())

        returnlist = list()
        for Root, Dirs, Files in os.walk(directory):
            for File in Files:
                for Extension in extensionlist_lower:
                    if File.lower().endswith(Extension):
                        ignoreIt = False
                        if(ignorelist is not None):
                            for c in ignorelist_lower:
                                if(File.lower().startswith(c)):
                                    ignoreIt = True
                                    break
                        if not ignoreIt:
                            logging.debug(os.path.join(Root, File))
                            returnlist.append(os.path.join(Root, File))

        return returnlist

    #
    # Returns the active platform if the envdict is inherited
    #
    def GetActivePlatform(self):
        if self.env is not None:
            return self.env.GetValue("ACTIVE_PLATFORM") 
        else:
            return -1

    #
    # Returns the active platform if the envdict is inherited
    #
    def GetTarget(self):
        if self.env is not None:
            return self.env.GetValue("TARGET")
        else:
            return -1

    #
    # Run a shell commmand and print the output to the log file
    #
    def RunCmd(self, cmd, capture=True, workingdir=None, outfile=None, outstream=None):
        starttime = datetime.now()
        logging.debug("Cmd to run is: " + cmd)
        logging.info("------------------------------------------------")
        logging.info("--------------Cmd Output Starting---------------")
        logging.info("------------------------------------------------")
        c = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=workingdir)
        if(capture):
            outr = PropagatingThread(target=self.reader, args=(outfile, outstream, c.stdout,))
            outr.start()
            c.wait()
            outr.join()
        else:
            c.wait()
                
        endtime = datetime.now()
        delta = endtime - starttime
        logging.info("------------------------------------------------")
        logging.info("--------------Cmd Output Finished---------------")
        logging.info("--------- Running Time (mm:ss): {0[0]:02}:{0[1]:02} ----------".format(divmod(delta.seconds, 60)))
        logging.info("------------------------------------------------")
        return c.returncode