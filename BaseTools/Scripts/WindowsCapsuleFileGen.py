##
# Tool to create a Windows Capsule files that complies with 
# the Windows Firmware Update Platform specification.  
#
# Gen INF, CAT, and then dev sign the CAT if PFX supplied.
#
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import sys
import logging
import argparse
import datetime

from edk2toollib.windows.capsule.cat_generator import *
from edk2toollib.windows.capsule.inf_generator import *
from edk2toollib.utility_functions import CatalogSignWithSignTool

def main():
    parser = argparse.ArgumentParser(description='Generate Windows Firmware Update Platform Files for Capsules')
    parser.add_argument("name", help="Firmware Name. No spaces")
    parser.add_argument("provider", help="Firmware provider listed in INF")
    parser.add_argument("description", help="Firmware description listed in INF")
    parser.add_argument("version_string", help="Version String in form of XX.XX.XX[.XX]")
    parser.add_argument("version_hex", help="Version String in Hex 0xAABBCCDD must be representable within 32bit")
    parser.add_argument("esrt_guid", help="guid string in registry format (########-####-####-####-############) for this ESRT entry")
    parser.add_argument("firmware_bin_file_path", help="full path to firmware bin / capsule file")
    parser.add_argument('arch', choices=InfGenerator.SUPPORTED_ARCH,  help="Architecture targeted by INF and CAT")
    parser.add_argument('operating_sytem', choices=CatGenerator.SUPPORTED_OS,  help="operating system targeted by INF and CAT")
    parser.add_argument("--mfgname", help="Manufacturer name listed in INF")
    parser.add_argument("--rollback", action="store_true", dest="rollback", help="build a rollback capsule",  default=False)
    parser.add_argument("--pfx_file", help="Full Path to PFX file.  If not set then signing will not be performed.")
    parser.add_argument("--pfx_pass", help="Password for PFX file.  Optional based on PFX file")


    #Turn on dubug level logging
    parser.add_argument("--debug", action="store_true", dest="debug", help="turn on debug logging level for file log",  default=False)
    #Output debug log
    parser.add_argument("-l", dest="OutputLog", help="Create an output debug log file: ie -l out.txt", default=None)

    args = parser.parse_args()

    #setup file based logging if outputReport specified
    if(args.OutputLog):
        if(len(args.OutputLog) < 2):
            logging.critical("the output log file parameter is invalid")
            return -2
        else:
            #setup file based logging
            filelogger = logging.FileHandler(filename=args.OutputLog, mode='w')
            if(args.debug):
                filelogger.setLevel(logging.DEBUG)
            else:
                filelogger.setLevel(logging.INFO)

            filelogger.setFormatter(formatter)
            logging.getLogger('').addHandler(filelogger)

    logging.info("Log Started: " + datetime.datetime.strftime(datetime.datetime.now(), "%A, %B %d, %Y %I:%M%p" ))
    OutputFolder = os.path.dirname(args.firmware_bin_file_path)
    FirmwareFile = os.path.basename(args.firmware_bin_file_path)

    logging.debug("Make INF")
    #Make INF
    InfFilePath = os.path.join(OutputFolder, args.name + ".inf")
    InfTool = InfGenerator(args.name, args.provider, args.esrt_guid, args.arch, args.description, args.version_string, args.version_hex)
    if(args.mfgname is not None):
      InfTool.Manufacturer = args.mfgname  #optional
    ret = InfTool.MakeInf(InfFilePath, FirmwareFile, args.rollback)
    if(ret != 0):
        logging.critical("CreateWindowsInf Failed with errorcode %d" % ret)
        return ret
      
    #Make CAT
    CatFilePath = os.path.realpath(os.path.join(OutputFolder, args.name + ".cat"))
    CatTool = CatGenerator(args.arch, args.operating_sytem)
    ret = CatTool.MakeCat(CatFilePath)

    if(ret != 0):
        logging.critical("Creating Cat file Failed with errorcode %d" % ret)
        return ret

    if(args.pfx_file is not None):
        logging.debug("PFX file set.  Going to do signing")
        #Find Signtool 
        SignToolPath = os.path.join(os.getenv("ProgramFiles(x86)"), "Windows Kits", "8.1", "bin", "x64", "signtool.exe")
        if not os.path.exists(SignToolPath):
            logging.debug("Failed to find 8.1 version of signtool. Trying 10")
            SignToolPath = SignToolPath.replace('8.1', '10')

        if not os.path.exists(SignToolPath):
            logging.critical("Can't find signtool on this machine.")
            return -3
        #dev sign the cat file
        ret = CatalogSignWithSignTool(SignToolPath, CatFilePath, args.pfx_file, args.pfx_pass)
        if(ret != 0):
            logging.critical("Signing Cat file Failed with errorcode %d" % ret)
            return ret
    else:
        logging.info("No PFX. Not signing")
    
    return ret 


#--------------------------------
# Control starts here
#
#--------------------------------
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
        logging.critical("Failed.  Return Code: %i" % retcode)
    #end logging
    logging.shutdown()
    sys.exit(retcode)
