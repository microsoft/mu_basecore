## @file NugetPublishing.py
# This tool allows a user to create a configuration for nuget as well as
# pack and push (publishing) a release to a feed.  
#
##
# Copyright (c) 2018, Microsoft Corporation
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

from __future__ import print_function  #support Python3 and 2 for print
import os
import sys
import argparse
import logging
import yaml
import xml.etree.ElementTree as etree
import threading
import subprocess
import shutil
import datetime
from io import StringIO
from MuPythonLibrary.UtilityFunctions import *

SCRIPT_PATH = os.path.abspath(os.path.dirname(__file__))
BinToolsPath = os.path.join(os.path.dirname(SCRIPT_PATH), "Bin", "Win32")

os.environ["PATH"] = BinToolsPath + os.pathsep + os.environ["PATH"]

OPEN_SOURCE_INITIATIVE_URL = "https://opensource.org/licenses/"
LICENSE_TYPE_SUPPORTED = {
    "BSD2": OPEN_SOURCE_INITIATIVE_URL + "BSD-2-Clause",
    "BSD3": OPEN_SOURCE_INITIATIVE_URL + "BSD-3-Clause",
    "APACHE2": OPEN_SOURCE_INITIATIVE_URL + "Apache-2.0",
    "MSPL" : OPEN_SOURCE_INITIATIVE_URL + "MS-PL", # Microsoft Public License
    "MIT" : OPEN_SOURCE_INITIATIVE_URL + "MIT"
}

class NugetSupport(object):
    # NOTE: This *should* have a namespace (http://schemas.microsoft.com/packaging/2010/07/nuspec.xsd)
    #       but ElementTree is incredibly stupid with namespaces.
    NUSPEC_TEMPLATE_XML = r'''<?xml version="1.0" encoding="utf-8"?>
<package>
    <metadata>
        <!-- Required elements-->
        <id></id>
        <version></version>
        <description></description>
        <authors></authors>

        <!-- Optional elements -->
        <licenseUrl></licenseUrl>
        <releaseNotes></releaseNotes>
        <projectUrl></projectUrl>
        <copyright></copyright>
    </metadata>
    <!-- Optional 'files' node -->
    <files>
        <file src="" target="" />
    </files>
</package>'''

    RELEASE_NOTE_SHORT_STRING_MAX_LENGTH = 500

    #
    # constructor that creates the NugetSupport object
    #   for new instances without existing config provide the Name parameter.
    #   for creating instance based on config file provide the path to the ConfigFile
    #
    def __init__(self, Name=None, ConfigFile=None):
        self.Name = Name
        self.TempFileToDelete = []  #everytime a temp is created add to list to cleanup
        self.NewVersion = None
        self.ConfigChanged = False
        
        if(ConfigFile is not None):
            self.FromConfigfile(ConfigFile)
            self.Name = self.ConfigData["name"]
        else:
            if(Name == None):
                raise Exception("Cont contruct object with both Name and ConfigFile as None")
            self.ConfigData = {"name": Name}
            self.Config = None

    def CleanUp(self):
        logging.debug("CleanUp Called.  Deleting all Temp Files")
        for a in self.TempFileToDelete:
            os.remove(a)

    #
    # Save the object config contents to file
    #
    def ToConfigFile(self, filepath=None):
        if(not self.ConfigChanged):
            logging.debug("No Config Changes.  Skip Writing config file")
            return 0
        
        if(filepath == None and self.Config == None):
            logging.error("No Config File to save to.")
            return -1

        if(filepath is not None):
            self.Config = filepath
        

        with open(filepath, "w") as c:
            yaml.dump(self.ConfigData, c, indent=4)
        logging.debug("Wrote config file to: %s" % filepath)
        self.ConfigChanged = False
        return 0

    def FromConfigfile(self, filepath):
        self.Config = filepath
        with open(self.Config, "r") as c:
            self.ConfigData = yaml.safe_load(c)

    def SetBasicData(self, authors, license, project, description, server, copyright):
        self.ConfigData["author_string"] = authors
        self.ConfigData["license_url"] = license
        self.ConfigData["project_url"] = project
        self.ConfigData["description_string"] = description
        self.ConfigData["server_url"] = server

        if not copyright:
            copyright = "Copyright %d" % datetime.date.today().year
        self.ConfigData["copyright_string"] = copyright

        self.ConfigChanged = True

    def UpdateCopyright(self, copyright):
        self.ConfigData["copyright_string"] = copyright
        self.ConfigChanged = True

    #
    # Print info about this object 
    #
    #
    def Print(self):
        print("=======================================")
        print(" Name:        " + self.Name)
        if(self.Config):
            print(" ConfigFile:  " + self.Config)
        else:
            print(" ConfigFile:  NOT SET")

        for k,v in self.ConfigData.items():
            print(" %s:   %s" %(k,v))

        print("----------------------------------------")
        print(" Temp Files List: ")
        for a in self.TempFileToDelete:
            print("    " + a)
        print("-----------------------------------------")
        print("=======================================")

    def LogObject(self):
        logging.debug("=======================================")
        logging.debug(" Name:        " + self.Name)
        if(self.Config):
            logging.debug(" ConfigFile:  " + self.Config)
        else:
            logging.debug(" ConfigFile:  NOT SET")

        for k,v in self.ConfigData.items():
            logging.debug(" %s:   %s" %(k,v))

        logging.debug("----------------------------------------")
        logging.debug(" Temp Files List: ")
        for a in self.TempFileToDelete:
            logging.debug("    " + a)
        logging.debug("-----------------------------------------")
        logging.debug("=======================================")
 

    #
    # given NugetSupport object 
    # create a nuspec file for packing
    #
    def _MakeNuspecXml(self, ContentDir, ReleaseNotesText=None):
        package = etree.fromstring(NugetSupport.NUSPEC_TEMPLATE_XML)
        meta = package.find("./metadata")
        meta.find("id").text = self.Name
        meta.find("version").text = self.NewVersion
        meta.find("authors").text = self.ConfigData["author_string"]
        meta.find("licenseUrl").text = self.ConfigData["license_url"]
        meta.find("projectUrl").text = self.ConfigData["project_url"]
        meta.find("description").text = self.ConfigData["description_string"] 
        meta.find("copyright").text = self.ConfigData["copyright_string"] 
        files = package.find("files")
        f = files.find("file")
        f.set("target", self.Name)
        f.set("src", ContentDir + "\\**\\*.*")

        if(ReleaseNotesText is not None):
            logging.debug("Make Nuspec Xml - ReleaseNotesText is not none.")
            #
            # Make sure it doesn't exceed reasonable length of string
            #
            if(len(ReleaseNotesText) > NugetSupport.RELEASE_NOTE_SHORT_STRING_MAX_LENGTH):
                logging.info("Make Nuspec Xml - ReleaseNotesText too long.  Length is (%d)" % len(ReleaseNotesText) )
                logging.debug("Original ReleaseNotesText is: %s" % ReleaseNotesText)
                #cut it off at max length
                ReleaseNotesText = ReleaseNotesText[:NugetSupport.RELEASE_NOTE_SHORT_STRING_MAX_LENGTH]
                #walk back to trim at last end of sentence 
                ReleaseNotesText = ReleaseNotesText.rpartition(".")[0].strip()
                logging.debug("New ReleaseNotesText is: %s" % ReleaseNotesText)

            meta.find("releaseNotes").text = ReleaseNotesText
        else:
            logging.debug("Make Nuspec Xml - ReleaseNotesText None. Removing element from nuspec.")
            meta.remove(meta.find("releaseNotes"))
        
        return etree.tostring(package)

    def _GetNuPkgFileName(self, version):
        #Nuget removes leading zeros so to match we must do the same
        s = self.Name + "."
        parts = version.split(".")
        for a in parts:
            s += str(int(a)) + "."
        
        #nuget must have at least x.y.z and will make zero any element undefined
        for a in range(len(parts), 3):
            s += "0."

        s += "nupkg"
        return s

    ##
    # Pack the current contents into 
    # Nupkg
    #
    def Pack(self, version, OutputDirectory, ContentDir, RelNotesText=None):
        self.NewVersion = version

        #content must be absolute path in nuspec otherwise it is assumed
        #relative to nuspec file.
        cdir = os.path.abspath(ContentDir)
        
        #make nuspec file
        xmlstring = self._MakeNuspecXml(cdir, RelNotesText)
        nuspec = os.path.join(OutputDirectory, self.Name + ".nuspec")
        self.TempFileToDelete.append(nuspec)
        f = open(nuspec, "wb")
        f.write(xmlstring)
        f.close() 

        # run nuget
        cmd = GetNugetCmd()
        cmd += ["pack", nuspec]
        cmd += ["-OutputDirectory", '"'+OutputDirectory+'"']
        cmd += ["-Verbosity", "detailed"]
        # cmd += ["-NonInteractive"]
        ret = RunCmd(cmd[0], " ".join(cmd[1:]))

        if(ret != 0):
            logging.error("Failed on nuget commend.  RC = 0x%x" % ret)
            return ret

        self.NuPackageFile = os.path.join(OutputDirectory, self._GetNuPkgFileName(self.NewVersion))
        self.TempFileToDelete.append(self.NuPackageFile)
        return ret

    def Push(self, nuPackage, apikey):
        if(not os.path.isfile(nuPackage)):
            raise Exception("Invalid file path for NuPkg file")
        logging.debug("Pushing %s file to server %s" % (nuPackage, self.ConfigData["server_url"]))

        cmd = GetNugetCmd()
        cmd += ["push", nuPackage]
        cmd += ["-Verbosity", "detailed"]
        # cmd += ["-NonInteractive"]
        cmd += ["-Source", self.ConfigData["server_url"]]
        cmd += ["-ApiKey", apikey]
        cmd_string = " ".join(cmd)
        output_buffer = StringIO()
        ret = RunCmd(cmd[0], " ".join(cmd[1:]), outstream=output_buffer)

        if(ret != 0):
            # Rewind the buffer and capture the contents.
            output_buffer.seek(0)
            output_contents = output_buffer.read()

            # Check for the API message.
            if "API key is invalid".lower() in output_contents.lower():
                logging.critical("API key is invalid. Please use --ApiKey to provide a valid key.")

            # Generic error.
            logging.error("Failed on nuget commend.  RC = 0x%x" % ret)
        
        return ret


def GatherArguments():
  tempparser = argparse.ArgumentParser(description='Nuget Helper Script for creating, packing, and pushing packages', add_help=False)
  tempparser.add_argument('--Operation', dest="op", choices=["New", "Pack", "Push", "PackAndPush"], required=True)

  #Get the operation the user wants to do
  (args, rest) = tempparser.parse_known_args()

  #now build up the real parser with required parameters
  parser = argparse.ArgumentParser(description='Nuget Helper Script for creating, packing, and pushing packages')
  parser.add_argument("--Dirty", dest="Dirty", action="store_true", help="Keep all temp files", default=False)
  parser.add_argument('--Operation', dest="Operation", choices=["New", "Pack", "Push", "PackAndPush"], required=True)
  parser.add_argument("--OutputLog", dest="OutputLog", help="Create an output log file")


  if(args.op.lower() == "new"):
    parser.add_argument("--ConfigFileFolderPath", dest="ConfigFileFolderPath", help="<Required>Path to folder to save new config file to", required=True)
    parser.add_argument('--Name', dest='Name', help='<Required> The unique id/name of the package.  This is a string naming the package', required=True)
    parser.add_argument('--Author', dest="Author", help="<Required> Author string for publishing", required=True)
    parser.add_argument("--ProjectUrl", dest="Project", help="<Required> Project Url", required=True)
    g = parser.add_mutually_exclusive_group(required=True)
    g.add_argument('--CustomLicenseUrl', dest="LicenseUrl", help="<Optional> http url for custom license file.  Can use LicenseType for standard licenses")
    g.add_argument('--LicenseType', dest="LicenseType", choices=LICENSE_TYPE_SUPPORTED.keys(),  help="Standard Licenses")
    parser.add_argument('--Description', dest="Description", help="<Required> Description of package.", required=True)
    parser.add_argument("--FeedUrl", dest="FeedUrl", help="<Required>Feed Url of the nuget server feed", required=True)
    parser.add_argument('--Copyright', dest="Copyright", help="Copyright string", required=False)
  
  elif(args.op.lower() == "pack" or args.op.lower() == "packandpush"):
    parser.add_argument("--ConfigFilePath", dest="ConfigFilePath", help="<Required>Path to config file", required=True)
    parser.add_argument('--Version', dest="Version", help="<Required> Version to publish", required=True)
    parser.add_argument('--ReleaseNotesText', dest="ReleaseNotes", help="<Optional>Release Notes String", required=False)
    parser.add_argument('--InputFolderPath', dest="InputFolderPath", help="<Required>Relative or Absolute Path to folder containing content to pack.  All content will be added but the folder will not", required=True)
    parser.add_argument('--Copyright', dest="Copyright", help="<Optional>Change the Copyright string")
    parser.add_argument('--ApiKey', dest="ApiKey", help="<Optional>Api key to use. Default is 'VSTS' which will invoke interactive login", default="VSTS")

  elif(args.op.lower() == "push"):
      parser.add_argument("--ConfigFilePath", dest="ConfigFilePath", help="<Required>Path to config file", required=True)
      parser.add_argument('--PackageFile', dest="PackageFile", help="<Required>Path To Package File", required=True)
      parser.add_argument('--ApiKey', dest="ApiKey", help="<Optional>Api key to use. Default is 'VSTS' which will invoke interactive login", default="VSTS")

  if(args.op.lower() == "pack"):
    parser.add_argument('--OutputFolderPath', dest="OutputFolderPath", help="<Optional>Output folder where nupkg will be saved.  Default is cwd", default=os.getcwd())


  return parser.parse_args()


def main():
    args = GatherArguments()
    ret = 0

    #setup file based logging if outputReport specified
    if(args.OutputLog):
        if(len(args.OutputLog) < 2):
            logging.critical("the output log file parameter is invalid")
            return -2

        #setup file based logging
        filelogger = logging.FileHandler(filename=args.OutputLog, mode='w')
        filelogger.setLevel(logging.DEBUG)
        filelogger.setFormatter(formatter)
        logging.getLogger('').addHandler(filelogger)

    logging.info("Log Started: " + datetime.datetime.strftime(datetime.datetime.now(), "%A, %B %d, %Y %I:%M%p" ))

    logging.debug("Script Path is %s" % SCRIPT_PATH)

    TempOutDir = None
    NuPkgFilePath = None

    if(args.Operation.lower() == "new"):
        logging.critical("Generating new nuget configuration...")
        logging.debug("Checking input parameters for new")
        ConfigFilePath = os.path.join(args.ConfigFileFolderPath, args.Name.strip() + ".config.yaml")

        if(not os.path.isdir(args.ConfigFileFolderPath)):
            logging.critical("Config File Folder Path doesn't exist.  %s" % args.ConfigFileFolderPath)
            raise Exception("Invalid Config File Folder.  Doesn't exist")

        if(os.path.isfile(ConfigFilePath)):
            logging.critical("Config File already exists at that path.  %s" % ConfigFilePath)
            raise Exception("Can't Create New Config file when file already exists")

        nu = NugetSupport(Name=args.Name)

        #license
        lurl = args.LicenseUrl
        if(args.LicenseType is not None):
            lurl = LICENSE_TYPE_SUPPORTED[args.LicenseType]
        nu.SetBasicData(args.Author, lurl, args.Project, args.Description, args.FeedUrl, args.Copyright)
        nu.LogObject()
        ret = nu.ToConfigFile(ConfigFilePath)
        return ret

    
    elif(args.Operation.lower() == "pack" or args.Operation.lower() == "packandpush"):
        logging.critical("Creating nuget package")
        logging.debug("Checking input parameters for packing")
        #check args
        if(not os.path.isfile(args.ConfigFilePath)):
            logging.critical("Invalid Config File (%s).  File doesn't exist" % args.ConfigFilePath)
            raise Exception("Invalid Config File.  File doesn't exist")
        if(not os.path.isdir(args.InputFolderPath)):
            logging.critical("Invalid Input folder (%s).  Folder doesn't exist" % args.InputFolderPath)
            raise Exception("Invalid Input folder.  folder doesn't exist")
        contents = os.listdir(args.InputFolderPath)
        logging.debug("Input Folder contains %d files" % len(contents))
        if(len(contents) == 0):
            logging.critical("No binary contents to pack in %s" % args.InputFolderPath)
            raise Exception("No binary contents to package")
        
        #make a temp dir for the pack operation which actually creates files
        TempOutDir = os.path.join(os.getcwd(), "_TEMP_" + str(datetime.datetime.now().time()).replace(":", "_"))
        os.mkdir(TempOutDir) 
        
        nu = NugetSupport(ConfigFile=args.ConfigFilePath)
        if(args.Copyright != None):
            nu.UpdateCopyright(args.Copyright)
        ret = nu.ToConfigFile()
        if (ret != 0):
            logging.error("Failed to save config file.  Return Code 0x%x" % ret)
            return ret

        ret = nu.Pack(args.Version, TempOutDir, args.InputFolderPath, args.ReleaseNotes)
        if (ret != 0):
            logging.error("Failed to pack.  Return Code 0x%x" % ret)
            return ret  

        NuPkgFilePath = nu.NuPackageFile

    if(args.Operation.lower() == "pack"):
        if(not os.path.isdir(args.OutputFolderPath)):
            logging.critical("Invalid Pack Output Folder (%s).  Folder doesn't exist" % args.OutputFolderPath)
            raise Exception("Invalid Output folder.  folder doesn't exist")
        #since it is pack only lets copy nupkg file to output
        shutil.copyfile(NuPkgFilePath, os.path.join(args.OutputFolderPath, os.path.basename(NuPkgFilePath)))
        NuPkgFilePath = os.path.join(args.OutputFolderPath, os.path.basename(NuPkgFilePath))

    if(args.Operation.lower() == "push"):
        #set the parameters for push
        logging.debug("Checking input parameters for push")
        #check args
        if(not os.path.isfile(args.ConfigFilePath)):
            logging.critical("Invalid Config File (%s).  File doesn't exist" % args.ConfigFilePath)
            raise Exception("Invalid Config File.  File doesn't exist")
        NuPkgFilePath = args.PackageFile
        nu = NugetSupport(ConfigFile=args.ConfigFilePath)
        

    if(args.Operation.lower() == "push" or args.Operation.lower() == "packandpush"):
        #do the pushing
        logging.critical("Pushing the package")
        logging.debug("NuPkgFilePath is %s" % NuPkgFilePath)
        #check args
        if(not os.path.isfile(NuPkgFilePath)):
            logging.critical("NuPkgFilePath is not valid file.  %s" % NuPkgFilePath)
            raise Exception("Invalid Pkg File.  File doesn't exist")
        ret = nu.Push(NuPkgFilePath, args.ApiKey)


    nu.LogObject()
    nu.ToConfigFile(args.ConfigFilePath)  #save any changes
    if(not args.Dirty):
        nu.CleanUp()
        if(TempOutDir is not None):
            os.removedirs(TempOutDir)
    return ret

if __name__ == '__main__':
    #setup main console as logger
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    console.setFormatter(formatter)
    logger.addHandler(console)

    #call main worker function
    retcode = main()

    if retcode != 0:
        logging.critical("Failed.  Return Code: %d" % retcode)
    else:
        logging.critical("Success!")
    #end logging
    logging.shutdown()
    sys.exit(retcode)
    