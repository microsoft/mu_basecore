##
# Tool to generate a report on Module Level Overriding status for a UEFI build.
# This tool depends on EDK2 and will parse dsc files, inf files and other standard
# EDK2 assets
#
# Copyright (c) 2018, Microsoft Corporation

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


import logging
import os
import sys
from datetime import datetime
import subprocess
import argparse
import hashlib


#Tuple for (version, entrycount)
FORMAT_VERSION_1 = (1, 4)   #Version 1: #OVERRIDE : VERSION | PATH_TO_MODULE | HASH | YYYY-MM-DDThh-mm-ss

#
# for now i want to keep this file as both a command line tool and a plugin for the Uefi Build system.
# To do this the plugin class is only defined if in the build environment where the plugin classes are importable.
#
#
try:
    from MuEnvironment import PluginManager
    from MuPythonLibrary.Uefi.EdkII.Parsers.DscParser import *
    from MuPythonLibrary.Uefi.EdkII.Parsers.InfParser import InfParser

    class OverrideValidation(PluginManager.IUefiBuildPlugin):

        class OverrideResult(object):
            OR_ALL_GOOD             = 0
            OR_FILE_CHANGE          = 1
            OR_VER_UNRECOG          = 2
            OR_INVALID_FORMAT       = 3
            OR_DSC_INF_NOT_FOUND    = 4
            OR_TARGET_INF_NOT_FOUND = 5

            @classmethod
            def GetErrStr (cls, errcode):
                str = ''
                if (errcode == cls.OR_ALL_GOOD):
                    str = 'SUCCESS'
                elif (errcode == cls.OR_FILE_CHANGE):
                    str = 'MISMATCH'
                elif (errcode == cls.OR_VER_UNRECOG):
                    str = 'INVALID_VERSION'
                elif (errcode == cls.OR_INVALID_FORMAT):
                    str = 'INVALID_FORMAT'
                elif (errcode == cls.OR_DSC_INF_NOT_FOUND):
                    str = 'FILE_NOT_FOUND'
                elif (errcode == cls.OR_TARGET_INF_NOT_FOUND):
                    str = 'FILE_NOT_FOUND'
                else:
                    str = 'UNKNOWN'
                return str
            # END: GetErrStr (errcode)

        class ModuleNode:
            # path: the workspace/package path based path
            def __init__(self, path, status, age):
                self.path = path
                self.status = status
                self.age = age
                self.expect_hash = ''
                self.entry_hash = ''
                self.reflist = []

        # Check and see if there is any line in the inf files that follows the pattern below:
        def do_pre_build(self, thebuilder):
            # Setup timestamp to log time cost in this section
            starttime = datetime.now()
            logging.info("---------------------------------------------------------")
            logging.info("--------------Override Validation Starting---------------")
            logging.info("---------------------------------------------------------")

            rc = self.override_plat_validate(thebuilder)
            if(rc == self.OverrideResult.OR_ALL_GOOD):
                logging.debug("Override validation all in sync")
            else:
                logging.error("Override validation failed")

            endtime = datetime.now()
            delta = endtime - starttime
            logging.info("---------------------------------------------------------")
            logging.info("--------------Override Validation Finished---------------")
            logging.info("-------------- Running Time (mm:ss): {0[0]:02}:{0[1]:02} --------------".format(divmod(delta.seconds, 60)))
            logging.info("---------------------------------------------------------")

            return rc
        # END: do_pre_build(self, thebuilder)

        # Walk through the target inf files in platform dsc file for override validation
        def override_plat_validate(self, thebuilder):
            result = self.OverrideResult.OR_ALL_GOOD
            InfFileList = self.get_dsc_inf_list(thebuilder)

            if (InfFileList == []):
                return result

            modulelist = []
            status = [0, 0]

            # Search through the workspace and package paths
            for file in InfFileList:
                temp_list = []
                modulenode = self.ModuleNode(file, self.OverrideResult.OR_ALL_GOOD, 0)
                fullpath = thebuilder.mws.join(thebuilder.ws, file)

                m_result = self.override_detect_process(thebuilder, fullpath, temp_list, modulenode, status)
                # Do not log the module that does not have any override records
                if (len(modulenode.reflist) > 0):
                    modulelist.append(modulenode)

                if m_result != self.OverrideResult.OR_ALL_GOOD:
                    if m_result != self.OverrideResult.OR_DSC_INF_NOT_FOUND:
                        result = m_result
                    logging.error("Override processing error %s in file %s." % (self.OverrideResult.GetErrStr(m_result), file))

            self.override_log_print(thebuilder, modulelist, status)

            return result
        # END: override_plat_validate(self, thebuilder)

        # Check and see if the picked file has this flag
        # filepath: the absolute path to the overriding module's inf file
        # filelist: the stack of files collected during a dfs for loop detection, should be absolute path and lower case all the time
        # modulenode: Module node of this "filepath" module
        # status: tuple that contains the count for succeeded modules scanned and total scanned
        def override_detect_process(self, thebuilder, filepath, filelist, modulenode, status):
            # Find the specific line of Override flag
            result = self.OverrideResult.OR_ALL_GOOD
            lineno = 0

            list_path = os.path.normpath(filepath).lower()

            if (list_path in filelist):
                return result

            # Check for file existence, fail otherwise.
            if not os.path.isfile(filepath):
                return self.OverrideResult.OR_DSC_INF_NOT_FOUND

            # Loop detection happen here by adding the visited module into stack
            filelist.append(list_path)

            # Look for a comment line that starts with Override
            with open(filepath, "r") as my_file:
                for Line in my_file :
                    lineno = lineno + 1
                    Line = Line.strip()
                    if not Line.startswith('#'):
                        continue

                    CommentLine = Line.strip('#').split(':')
                    if (len(CommentLine) != 2) or (CommentLine[0].strip().lower() != 'override'):
                        continue

                    # Process the override content, 1. Bail on bad data; 2. Print on formatted data (matched or not)
                    m_result = self.override_process_line(thebuilder, CommentLine[1], filepath, filelist, modulenode, status)

                    if m_result != self.OverrideResult.OR_ALL_GOOD:
                        result = m_result
                        logging.error("At Line %d: %s" %(lineno, Line))

            # Revert this visitied indicator after this branch is done searching
            filelist.remove(list_path)
            return result
        # END: override_detect_process(self, thebuilder, filepath, filelist, modulenode, status)

        # Process the comment line that starts with Override
        # overridecnt: Content of Override record, should include the content after "Override:"
        # filepath: the absolute path to the overriding module's inf file
        # filelist: the stack of files collected during a dfs for loop detection, should be absolute path and lower case all the time
        # modulenode: Module node of this "filepath" module
        # status: tuple that contains the count for succeeded modules scanned and total scanned
        def override_process_line(self, thebuilder, overridecnt, filepath, filelist, modulenode, status):
            # Prepare the potential node and update total processed number here
            m_node = self.ModuleNode("", self.OverrideResult.OR_ALL_GOOD, 0)
            modulenode.reflist.append(m_node)
            status[1] = status[1] + 1

            # Handle tail comments and/or empty spaces
            EndIndex = overridecnt.find('#')
            EndIndex = EndIndex if (EndIndex != -1) else len(overridecnt)

            OverrideEntry = overridecnt[0:EndIndex].split('|')

            # Step 1: Check version and number of blocks in this entry
            EntryVersion = 0
            try:
                EntryVersion = int(OverrideEntry[0])
            except ValueError:
                logging.error("Inf Override Parse Error, override parameter has invalid version %s" %(OverrideEntry[0]))
                result = self.OverrideResult.OR_INVALID_FORMAT
                m_node.status = result
                return result

            # Verify this is a known version and has valid number of entries
            if not ((EntryVersion == FORMAT_VERSION_1[0]) and (len(OverrideEntry) == FORMAT_VERSION_1[1])):
                logging.error("Inf Override Unrecognized Version or corrupted format in this entry: %s" %(filepath))
                result = self.OverrideResult.OR_VER_UNRECOG
                m_node.status = result
                return result

            # Step 2: Process the path to overridden module
            # Normalize the path to support different slashes, then strip the initial '\\' to make sure os.path.join will work correctly
            overriddenpath = os.path.normpath(OverrideEntry[1].strip()).strip('\\')
            fullpath = os.path.normpath(thebuilder.mws.join(thebuilder.ws, overriddenpath))
            # Search overridden module in workspace
            if not os.path.isfile(fullpath):
                logging.error("Inf Overridden File Not Found in Workspace or Packages_Path: %s" %(overriddenpath))
                result = self.OverrideResult.OR_TARGET_INF_NOT_FOUND
                m_node.path = overriddenpath
                m_node.status = result
                return result

            # Step 3: Grep hash entry
            EntryHash = OverrideEntry[2].strip()

            # Step 4: Parse the time of hash generation
            try:
                EntryTimestamp = datetime.strptime(OverrideEntry[3].strip(), "%Y-%m-%dT%H-%M-%S")
            except ValueError:
                logging.error("Inf Override Parse Error, override parameter has invalid timestamp %s" %(OverrideEntry[3].strip()))
                result = self.OverrideResult.OR_INVALID_FORMAT
                m_node.status = result
                return result

            # Step 5: Calculate the hash of overridden module and compare with our record in the overriding module
            res_tuple = self.override_hash_compare(thebuilder, EntryVersion, EntryHash, fullpath)
            result = res_tuple.get('result')
            m_node.expect_hash = res_tuple.get('hash_val')

            # Step 6: House keeping
            # Process the path to workspace/package path based add it to the parent node
            overridden_rel_path = thebuilder.mws.relpath(fullpath, thebuilder.ws).replace('\\', '/')
            date_delta = datetime.utcnow() - EntryTimestamp

            m_node.entry_hash = EntryHash
            m_node.path = overridden_rel_path
            m_node.status = result
            m_node.age = date_delta.days

            if (result == self.OverrideResult.OR_ALL_GOOD):
                status[0] = status[0] + 1
            else:
                logging.error("Inf Override Hash Error: %s, expecting %s, has %s" %(self.OverrideResult.GetErrStr(result), m_node.expect_hash, m_node.entry_hash))

            # Step 7: Do depth-first-search for cascaded modules
            m_result = self.override_detect_process(thebuilder, fullpath, filelist, m_node, status)
            if (m_result != self.OverrideResult.OR_ALL_GOOD) and \
                (result == self.OverrideResult.OR_ALL_GOOD):
                result = m_result

            # The result will be inherited from above function calls
            return result
        # END: override_process_line(self, thebuilder, overridecnt, filepath, filelist, modulenode, status)

        # Check override record against parsed entries
        # version: Override record's version number, normally parsed from the override record line
        # hash: Override record's hash field, normally parsed from the override record line, calculated by the standalone ModuleHash tool
        # fullpath: the absolute path to the overriden module's inf file
        def override_hash_compare(self, thebuilder, version, hash, fullpath):
            result = self.OverrideResult.OR_ALL_GOOD
            hash_val = ''

            # Error out the unknown version
            if (version == FORMAT_VERSION_1[0]):
                hash_val = ModuleHashCal(fullpath)
                if (hash_val != hash):
                    result = self.OverrideResult.OR_FILE_CHANGE
            else:
                # Should not happen
                result = self.OverrideResult.OR_VER_UNRECOG
            return {'result':result, 'hash_val':hash_val}
        # END: override_hash_compare(self, thebuilder, version, hash, fullpath)

        # Print the log after override validation is complete
        # modulelist: list of modules collected while processing inf files
        # status: tuple that contains the count for succeeded modules scanned and total scanned
        def override_log_print(self, thebuilder, modulelist, status):
            # Check and specify log file path
            base_path = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")
            if (base_path is None):
                return

            logfile = os.path.join(base_path, "OVERRIDELOG.TXT")
            logfile = os.path.normpath(logfile)
            if not os.path.isdir(os.path.dirname(logfile)):
                os.makedirs(os.path.dirname(logfile))

            with open(logfile, 'w') as log:
                log.write("Platform:     %s\n" %(thebuilder.env.GetValue("PRODUCT_NAME")))
                log.write("Version:      %s\n" %(thebuilder.env.GetValue("BLD_*_BUILDID_STRING")))
                log.write("Date:         %s\n" %(datetime.utcnow().strftime("%Y-%m-%dT%H-%M-%S")))
                log.write("Commit:       %s\n" %(thebuilder.env.GetValue("BLD_*_BUILDSHA")))
                log.write("State:        %d/%d\n" %(status[0], status[1]))

                log.write("\n")
                log.write("Overrides\n")
                log.write("----------------------------------------------------------------\n")
                log.write("\n")

                for node in modulelist:
                    # Pass in a "stack" into the function for loop detection while doing dfs
                    stack = []
                    log.write("OVERRIDER: %s\n" %(node.path))
                    log.write("ORIGINALS:\n")
                    self.node_dfs(thebuilder, node, stack, log)
                    log.write("\n")

            logfile = logfile.replace('\\', '/')
            logging.critical("Override Log file at %s" %(logfile))
        # END: override_log_print(self, thebuilder, modulelist, status)

        # Traverse through the collect structure and print log
        # node: Module Node representing the overriding module
        # stack: the stack of paths collected during a dfs for loop detection, should be absolute path and lower case all the time
        # log: log file object, must be readliy open for file write when called
        def node_dfs(self, thebuilder, node, stack, log):
            fullpath = os.path.normpath(thebuilder.mws.join(thebuilder.ws, node.path)).lower()
            if (node.path in stack):
                return
            stack.append(fullpath)

            for m_node in node.reflist:
                list_len = len(stack)
                str = "\t"*list_len+"+ %s | %s | %d days\n" %(m_node.path, self.OverrideResult.GetErrStr(m_node.status), m_node.age)
                if (m_node.status == self.OverrideResult.OR_ALL_GOOD):
                    log.write(str)
                    self.node_dfs(thebuilder, m_node, stack, log)
                elif (m_node.status == self.OverrideResult.OR_FILE_CHANGE):
                    log.write(str)
                    log.write("\t"*list_len+"| \tCurrent State: %s | Last Fingerprint: %s\n" %(m_node.expect_hash, m_node.entry_hash))
                    self.node_dfs(thebuilder, m_node, stack, log)
                else:
                    log.write("\t"*list_len+"+ %s | %s\n" % (m_node.path, self.OverrideResult.GetErrStr(m_node.status)))

            stack.remove(fullpath)
        # END: node_dfs(self, thebuilder, node, stack, log)

        # Create a list of inf files that is included in a dsc file (including !Include entries)
        def get_dsc_inf_list(self, thebuilder):
            InfFileList = []

            # Dsc parser is used in this instance
            logging.debug("Parse Active Platform DSC file")
            input_vars = thebuilder.env.GetAllBuildKeyValues()
            input_vars["TARGET"] = thebuilder.env.GetValue("TARGET")
            dscp = DscParser().SetBaseAbsPath(thebuilder.ws).SetPackagePaths(thebuilder.pp.split(os.pathsep)).SetInputVars(input_vars)
            plat_dsc = thebuilder.env.GetValue("ACTIVE_PLATFORM")
            if (plat_dsc is None):
                return InfFileList

            pa = thebuilder.mws.join(thebuilder.ws, plat_dsc)
            dscp.ParseFile(pa)

            # Here we collect all the reference libraries, IA-32 modules, x64 modules and other modules
            if (dscp.Parsed) :
                for lib in dscp.Libs:
                    InfFileList.append(lib)
                for ThreeMod in dscp.ThreeMods:
                    InfFileList.append(ThreeMod)
                for SixMod in dscp.SixMods:
                    InfFileList.append(SixMod)
                for OtherMod in dscp.OtherMods:
                    InfFileList.append(OtherMod)
            return InfFileList
        # END: get_dsc_inf_list(self, thebuilder)
except ImportError:
    pass

# This caluculates the md5 for the inf file as well as all the first order include source files
# path: the absolute path to the module's inf file
def ModuleHashCal(path):

    sourcefileList = []
    binaryfileList = []
    sourcefileList.append(path)

    # Find the specific line of Sources section
    folderpath = os.path.dirname(path)

    # Use InfParser to parse sources section
    ip = InfParser()
    ip.ParseFile(path)

    # Add all referenced source files in addtion to our inf file list
    for source in ip.Sources:
        sourcefileList.append(os.path.normpath(os.path.join(folderpath, source)))

    # Add all referenced binary files to our binary file list
    for binary in ip.Binaries:
        binaryfileList.append(os.path.normpath(os.path.join(folderpath, binary)))

    hash_obj = hashlib.md5()
    for sfile in sourcefileList:
        #print('Calculated: %s' %(sfile)) #Debug only
        with open(sfile, 'rb') as entry:
            # replace \r\n with \n to take care of line terminators
            hash_obj.update(entry.read().replace(b'\r\n', b'\n'))

    for bfile in binaryfileList:
        #print('Calculated: %s' %(bfile)) #Debug only
        with open(bfile, 'rb') as entry:
            hash_obj.update(entry.read())

    result = hash_obj.hexdigest()
    return result

# Setup import and argument parser
def path_parse():

    parser = argparse.ArgumentParser()

    parser.add_argument (
        '-w', '--workspace', dest = 'WorkSpace', required = True, type=str,
        help = '''Specify the absolute path to your workspace by passing -w WORKSPACE or --workspace WORKSPACE.'''
        )
    parser.add_argument (
        '-m', '--modulepath', dest = 'ModulePath', required = True, type=str,
        help = '''Specify the absolute path to your module by passing -m Path/To/Module.inf or --modulepath Path/To/Module.inf.'''
        )

    Paths = parser.parse_args()
    # pre-process the parsed paths to abspath
    Paths.WorkSpace = os.path.abspath(Paths.WorkSpace)
    Paths.ModulePath = os.path.abspath(Paths.ModulePath)

    if not os.path.isdir(Paths.WorkSpace):
        raise RuntimeError("Workspace path is invalid.")
    if not os.path.isfile(Paths.ModulePath):
        raise RuntimeError("Module path is invalid.")
    # Needs to strip os.sep is to take care of the root path case
    # For a folder, this will do nothing on a formatted abspath
    # For a drive root, this will rip off the os.sep
    if not os.path.normcase(Paths.ModulePath).startswith(os.path.normcase(Paths.WorkSpace.strip(os.sep)) + os.sep):
        raise RuntimeError("Module is not within specified Workspace.")

    return Paths

################################################
# This plugin python file is also
# a command line tool
#
################################################
if __name__ == '__main__':

    SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
    MUBUILD_PATH = os.path.dirname(os.path.dirname(os.path.dirname(SCRIPT_PATH)))
    PY_LIB_PATH = os.path.join(MUBUILD_PATH, "PythonLibrary")
    sys.path.append(PY_LIB_PATH)
    from MuPythonLibrary.Uefi.EdkII.Parsers.InfParser import InfParser
    from MuPythonLibrary.Uefi.EdkII.PathUtilities import Edk2Path

    # Parse required paths passed from cmd line arguments
    Paths = path_parse()

    dummy_list = []
    pathtool = Edk2Path(Paths.WorkSpace, dummy_list)

    # Use absolute module path to find package path
    pkg_path = pathtool.GetContainingPackage(Paths.ModulePath)
    rel_path = Paths.ModulePath[Paths.ModulePath.find(pkg_path):]

    rel_path = rel_path.replace('\\', '/')
    mod_hash = ModuleHashCal(Paths.ModulePath)
    print("Copy and paste the following line(s) to your overrider inf file(s):\n")
    print('#Override : %08d | %s | %s | %s' % (FORMAT_VERSION_1[0], rel_path, mod_hash, datetime.utcnow().strftime("%Y-%m-%dT%H-%M-%S")))
