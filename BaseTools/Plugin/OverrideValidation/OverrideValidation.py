##
# Tool to generate a report on Module Level Overriding status for a UEFI build.
# This tool depends on EDK2 and will parse dsc files, inf files and other standard
# EDK2 assets
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


import logging
import os
import io
import sys
from datetime import datetime
import subprocess
import argparse
import hashlib
import re
from io import StringIO

#
# for now i want to keep this file as both a command line tool and a plugin for the Uefi Build system.
# To do this the plugin class is only defined if in the build environment where the plugin classes are importable.
#
#
try:
    from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
    from edk2toollib.uefi.edk2.parsers.inf_parser import InfParser
    from edk2toollib.utility_functions import RunCmd
    from edk2toollib.uefi.edk2.parsers.dsc_parser import *
    from edk2toollib.uefi.edk2.path_utilities import Edk2Path

    #Tuple for (version, entrycount)
    FORMAT_VERSION_1 = (1, 4)   #Version 1: #OVERRIDE : VERSION | PATH_TO_MODULE | HASH | YYYY-MM-DDThh-mm-ss
    FORMAT_VERSION_2 = (2, 5)   #Version 2: #OVERRIDE : VERSION | PATH_TO_MODULE | HASH | YYYY-MM-DDThh-mm-ss | GIT_COMMIT
    FORMAT_VERSIONS = [FORMAT_VERSION_1, FORMAT_VERSION_2]


    class OverrideValidation(IUefiBuildPlugin):

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
                    str = 'INF_FILE_NOT_FOUND'
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

            self.PathTool = thebuilder.edk2path

            if (InfFileList == []):
                return result

            modulelist = []
            status = [0, 0]

            # Search through the workspace and package paths
            for file in InfFileList:
                temp_list = []
                modulenode = self.ModuleNode(file, self.OverrideResult.OR_ALL_GOOD, 0)
                fullpath = thebuilder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(file)

                m_result = self.override_detect_process(thebuilder, fullpath, temp_list, modulenode, status)
                # Do not log the module that does not have any override records
                if (len(modulenode.reflist) > 0):
                    modulelist.append(modulenode)

                if m_result != self.OverrideResult.OR_ALL_GOOD:
                    if m_result != self.OverrideResult.OR_DSC_INF_NOT_FOUND:
                        result = m_result
                    logging.error("Override processing error %s in file/dir %s" % (self.OverrideResult.GetErrStr(m_result), file))

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
            trackno = 0
            track_nf = []
            track_fc = []
            track_ag = []

            list_path = os.path.normpath(filepath).lower()

            if (list_path in filelist):
                return result

            # This processing step is only for files. If the filepath is a directory (meaning the directory is hashed), skip this step
            if os.path.isdir(filepath):
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
                    if (len(CommentLine) != 2) or\
                       ((CommentLine[0].strip().lower() != 'override') and\
                        (CommentLine[0].strip().lower() != 'track')):
                        continue

                    # Process the override content, 1. Bail on bad data; 2. Print on formatted data (matched or not)
                    tagtype = CommentLine[0].strip().lower()
                    m_result = self.override_process_line(thebuilder, CommentLine[1], filepath, filelist, modulenode, status, tagtype)

                    if CommentLine[0].strip().lower() == 'override':
                        # For override tags, the hash has to match
                        if m_result != self.OverrideResult.OR_ALL_GOOD:
                            result = m_result
                            logging.error("At Line %d: %s" %(lineno, Line))

                    elif CommentLine[0].strip().lower() == 'track':
                        # For track tags, ignore the tags of which inf modules are not found
                        trackno = trackno + 1
                        if m_result == self.OverrideResult.OR_TARGET_INF_NOT_FOUND:
                            track_nf.append ((lineno, Line))
                            logging.info("At Line %d: %s" %(lineno, Line))
                        elif m_result == self.OverrideResult.OR_FILE_CHANGE:
                            track_fc.append([lineno, Line, modulenode.reflist[-1].path, False])
                            logging.info("At Line %d: %s" %(lineno, Line))
                        elif m_result != self.OverrideResult.OR_ALL_GOOD:
                            result = m_result
                            logging.error("At Line %d: %s" %(lineno, Line))
                        else:
                            track_ag.append(modulenode.reflist[-1].path)

            if trackno != 0 and len(track_nf) == trackno:
                # All track tags in this file are not found, this will enforce a failure, if not already failed
                if result == self.OverrideResult.OR_ALL_GOOD:
                    result = self.OverrideResult.OR_TARGET_INF_NOT_FOUND
                for (lineno, Line) in track_nf:
                    logging.error("Track tag failed to locate target module at Line %d: %s" %(lineno, Line))

            if len(track_fc) != 0:
                canceled_cnt = 0
                # Some track tags failed, see if they can be canceled out by other passed track tags
                for entry in track_fc:
                    for all_good_line in track_ag:
                        if entry[2] == all_good_line:
                            canceled_cnt = canceled_cnt + 1
                            entry[3] = True
                            break
                if canceled_cnt != len(track_fc) and result == self.OverrideResult.OR_ALL_GOOD:
                    result = self.OverrideResult.OR_FILE_CHANGE

                for (lineno, Line, _, canceled) in track_fc:
                    if not canceled:
                        logging.error("Track tag failed to match module hash at Line %d: %s" %(lineno, Line))

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
        def override_process_line(self, thebuilder, overridecnt, filepath, filelist, modulenode, status, tagtype):
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
            version_match = False
            for VERSION_FORMAT in FORMAT_VERSIONS:
                if len(VERSION_FORMAT) < 2:
                    logging.warning("Invalid formatted version: " + str(VERSION_FORMAT))
                    continue
                if EntryVersion == VERSION_FORMAT[0] and len(OverrideEntry) == VERSION_FORMAT[1]:
                    version_match = VERSION_FORMAT
                    break
            if version_match == False:
                logging.error(f"Inf Override Unrecognized Version {EntryVersion} or corrupted format ({len(OverrideEntry)}) in this entry: {filepath}")
                result = self.OverrideResult.OR_VER_UNRECOG
                m_node.status = result
                return result

            if version_match[0] == 1:
                return self.override_process_line_with_version1(thebuilder, filelist, OverrideEntry, m_node, status, tagtype)
            elif version_match[0] == 2:
                return self.override_process_line_with_version2(thebuilder, filelist, OverrideEntry, m_node, status, tagtype)
            else:
                raise ValueError(f"Handler is not provided for {version_match}")

        # END: override_process_line(self, thebuilder, overridecnt, filepath, filelist, modulenode, status)

        def override_process_line_with_version1(self, thebuilder, filelist, OverrideEntry, m_node, status, tagtype):
            EntryVersion = 1
            # Step 2: Process the path to overridden module
            # Normalize the path to support different slashes, then strip the initial '\\' to make sure os.path.join will work correctly
            overriddenpath = os.path.normpath(OverrideEntry[1].strip()).strip('\\')
            fullpath = thebuilder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(overriddenpath, log_errors=False)
            # Search overridden module in workspace
            if fullpath is None:
                logging.info("Inf Overridden File/Path Not Found in Workspace or Packages_Path: %s" %(overriddenpath))
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
            overridden_rel_path = self.PathTool.GetEdk2RelativePathFromAbsolutePath(fullpath)
            date_delta = datetime.utcnow() - EntryTimestamp

            m_node.entry_hash = EntryHash
            m_node.path = overridden_rel_path
            m_node.status = result
            m_node.age = date_delta.days

            if (result == self.OverrideResult.OR_ALL_GOOD):
                status[0] = status[0] + 1
            else:
                pnt_str = "Inf Override Hash Error: %s, expecting %s, has %s" %(self.OverrideResult.GetErrStr(result), m_node.expect_hash, m_node.entry_hash)
                if tagtype == 'override':
                    logging.error(pnt_str)
                else:
                    logging.debug(pnt_str) # MU_CHANGE

            # Step 7: Do depth-first-search for cascaded modules
            m_result = self.override_detect_process(thebuilder, fullpath, filelist, m_node, status)
            if (m_result != self.OverrideResult.OR_ALL_GOOD) and \
                (result == self.OverrideResult.OR_ALL_GOOD):
                result = m_result

            # The result will be inherited from above function calls
            return result
        # END: override_process_line_version1(self, thebuilder, filelist, OverrideEntry, m_node, status)

        def override_process_line_with_version2(self, thebuilder, filelist, OverrideEntry, m_node, status, tagtype):
            ''' #Version 2: #OVERRIDE : VERSION | PATH_TO_MODULE | HASH | YYYY-MM-DDThh-mm-ss | GIT_COMMIT '''
            GitHash = OverrideEntry[4].strip()
            del OverrideEntry[4]
            result = self.override_process_line_with_version1(thebuilder, filelist, OverrideEntry, m_node, status, tagtype)
            # if we failed, do a diff of the overridden file (as long as exist) and show the output
            if result != self.OverrideResult.OR_ALL_GOOD and result != self.OverrideResult.OR_TARGET_INF_NOT_FOUND:
                overriddenpath = os.path.normpath(OverrideEntry[1].strip()).strip('\\')
                fullpath = thebuilder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(overriddenpath, log_errors=False)
                if fullpath is not None:
                    patch = ModuleGitPatch(fullpath, GitHash)
                    # TODO: figure out how to get the log file
                pnt_str = f"Override diff since last update at commit {GitHash}"
                if tagtype == 'override':
                    logging.error(pnt_str)
                else:
                    logging.info(pnt_str)

            return result
        # END: override_process_line_version2(self, thebuilder, filelist, OverrideEntry, m_node, status)

        # Check override record against parsed entries
        # version: Override record's version number, normally parsed from the override record line
        # hash: Override record's hash field, normally parsed from the override record line, calculated by the standalone ModuleHash tool
        # fullpath: the absolute path to the overridden module's inf file
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
        # log: log file object, must be readily open for file write when called
        def node_dfs(self, thebuilder, node, stack, log):
            fullpath = os.path.normpath(thebuilder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(node.path)).lower()
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
            dscp = DscParser().SetEdk2Path(thebuilder.edk2path).SetInputVars(input_vars)
            plat_dsc = thebuilder.env.GetValue("ACTIVE_PLATFORM")
            if (plat_dsc is None):
                return InfFileList

            # Parse the DSC
            pa = thebuilder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(plat_dsc)
            dscp.ParseFile(pa)
            # Add the DSC itself (including all the includes)
            InfFileList.extend(dscp.GetAllDscPaths())
            # Add the FDF
            if "FLASH_DEFINITION" in dscp.LocalVars:
                fd = thebuilder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(dscp.LocalVars["FLASH_DEFINITION"])
                InfFileList.append(fd)
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

# This calculates the md5 for the inf file as well as all the first order include source files
# path: the absolute path to the module's inf file
def ModuleHashCal(path):

    sourcefileList = []
    binaryfileList = []
    hash_obj = hashlib.md5()

    # Find the specific line of Sources section
    folderpath = os.path.dirname(path)

    if os.path.isdir(path):
      # Collect all files in this folder to the list
      for subdir, _, files in os.walk(path):
          for file in files:
              sourcefileList.append(os.path.join(subdir, file))
    else:
        sourcefileList.append(path)

    if path.lower().endswith(".inf") and os.path.isfile(path):

        # Use InfParser to parse sources section
        ip = InfParser()
        ip.ParseFile(path)

        # Add all referenced source files in addition to our inf file list
        for source in ip.Sources:
            sourcefileList.append(os.path.normpath(os.path.join(folderpath, source)))

        # Add all referenced binary files to our binary file list
        for binary in ip.Binaries:
            binaryfileList.append(os.path.normpath(os.path.join(folderpath, binary)))

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

def ModuleGitPatch(path, git_hash):
    ''' return a git patch of the given file since the hash '''
    GitOutput = io.StringIO()
    # TODO - let this go to console so we get colors
    path_dir = os.path.dirname(path)
    ret = RunCmd("git", f"diff {git_hash} {path}", workingdir=path_dir, outstream=GitOutput, logging_level=logging.DEBUG) # MU_CHANGE
    if ret != 0:
        return ""
    GitOutput.seek(0)
    result = []
    for line in GitOutput.readlines():
        result.append(line.strip())
    return "\n".join(result)

def ModuleGitHash(path):
    ''' gets the current git hash of the given directory that path is '''
    abspath_dir = os.path.dirname(os.path.abspath(path))
    git_stream = StringIO()
    ret = RunCmd("git", "rev-parse --verify HEAD", workingdir=abspath_dir, outstream=git_stream, logging_level=logging.DEBUG) # MU_CHANGE
    if ret != 0:
        return None
    git_stream.seek(0)
    git_hash = git_stream.readline().strip()
    if git_hash.count(" ") != 0:
        raise RuntimeError("Unable to get GIT HASH for: " + abspath_dir)
    return git_hash

# Setup import and argument parser
def path_parse():

    parser = argparse.ArgumentParser()

    parser.add_argument (
        '-w', '--workspace', dest = 'WorkSpace', required = True, type=str,
        help = '''Specify the absolute path to your workspace by passing -w WORKSPACE or --workspace WORKSPACE.'''
        )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument (
        '-m', '--modulepath', dest = 'ModulePath', type=str,
        help = '''Specify the absolute path to your module by passing -m Path/To/Module.inf or --modulepath Path/To/Module.inf.'''
        )
    group.add_argument (
        '-t', '--targetpath', dest = 'TargetPath', type=str,
        help = '''Specify the absolute path to your target module/file/folder by passing t Path/To/Target or --targetpath Path/To/Target.'''
        )
    group.add_argument (
        '-r', '--regenpath', dest = 'RegenPath', type=str,
        help = '''Specify the absolute path to an inf with existing overrides to regen by passing r Path/To/Target or --regenpath Path/To/Target.'''
        )
    parser.add_argument (
        '-p', '--packagepath', dest = 'RegenPackagePath', nargs="*", default=[],
        help = '''Specify the packages path to be used to resolve relative paths when using --regenpath. ignored otherwise. Workspace is always included.'''
        )
    parser.add_argument (
        '-v', '--version', dest = 'Version', default= 2, type=int,
        help = '''This is the version of the override hash to produce (currently only 1 and 2 are valid)'''
        )
    parser.add_argument (
        '--track', action="store_true", dest = 'Track', default= False,
        help = '''Indicate whether to create a track tag or override tag. Track tags will be treated as ignorable if the
        overridden modules are not found. However, for each module that contains track tags, at least one tracked modules
        has to be found, otherwise build will fail. By default, all tags will be generated as override tags.'''
        )

    Paths = parser.parse_args()
    # pre-process the parsed paths to abspath
    Paths.WorkSpace = os.path.abspath(Paths.WorkSpace)
    if Paths.TargetPath is not None:
        Paths.TargetPath = os.path.abspath(Paths.TargetPath)

    if Paths.ModulePath is not None:
        Paths.TargetPath = os.path.abspath(Paths.ModulePath)
        if not os.path.isfile(Paths.TargetPath):
            raise RuntimeError("Module path is invalid.")

    if Paths.Version < 1 or Paths.Version > len(FORMAT_VERSIONS):
        raise RuntimeError("Version is invalid")

    if not os.path.isdir(Paths.WorkSpace):
        raise RuntimeError("Workspace path is invalid.")
    if Paths.TargetPath is not None:
        if not os.path.isfile(Paths.TargetPath) and not os.path.isdir(Paths.TargetPath):
            raise RuntimeError("Module path is invalid.")
        # Needs to strip os.sep is to take care of the root path case
        # For a folder, this will do nothing on a formatted abspath
        # For a drive root, this will rip off the os.sep
        if not os.path.normcase(Paths.TargetPath).startswith(os.path.normcase(Paths.WorkSpace.rstrip(os.sep)) + os.sep):
            raise RuntimeError("Module is not within specified Workspace.")

    if Paths.RegenPath is not None:
        if not os.path.isfile(Paths.RegenPath):
            raise RuntimeError("Regen path is invalid.")
        # Needs to strip os.sep is to take care of the root path case
        # For a folder, this will do nothing on a formatted abspath
        # For a drive root, this will rip off the os.sep
        if not os.path.normcase(Paths.RegenPath).startswith(os.path.normcase(Paths.WorkSpace.rstrip(os.sep)) + os.sep):
            raise RuntimeError("Module is not within specified Workspace.")

    return Paths

################################################
# This plugin python file is also
# a command line tool
#
################################################
if __name__ == '__main__':

    from edk2toollib.uefi.edk2.parsers.inf_parser import InfParser
    from edk2toollib.uefi.edk2.path_utilities import Edk2Path

    # Parse required paths passed from cmd line arguments
    Paths = path_parse()

    # check if we are asked to update an .inf file "in-place"
    if (Paths.RegenPath is not None):
        pathtool = Edk2Path(Paths.WorkSpace, Paths.RegenPackagePath)

        v1_regex = re.compile(r"#(Override|Track) : (.*?) \| (.*?) \| (.*?) \| (.*?)")
        v2_regex = re.compile(r"#(Override|Track) : (.*?) \| (.*?) \| (.*?) \| (.*?) \| (.*?)")
        with open (Paths.RegenPath) as fd:
            RegenInfData = fd.read()

        RegenInfOutData = ""
        for line in RegenInfData.splitlines (True):
            match = v1_regex.match(line)
            if match is None:
                match = v2_regex.match(line)

            if match is not None:
                rel_path = match.group(3)
                abs_path = pathtool.GetAbsolutePathOnThisSystemFromEdk2RelativePath(rel_path)
                if abs_path is not None:
                    mod_hash = ModuleHashCal(abs_path)
                    # only update the line if the hash has changed - this ensures the timestamp tracks actual changes rather than last time it was run.
                    if (mod_hash != match.group(4)):
                        VERSION_INDEX = Paths.Version - 1

                        if VERSION_INDEX == 0:
                            line = '#%s : %08d | %s | %s | %s\n' % (match.group(1), FORMAT_VERSION_1[0], rel_path, mod_hash, datetime.utcnow().strftime("%Y-%m-%dT%H-%M-%S"))
                        elif VERSION_INDEX == 1:
                            git_hash = ModuleGitHash(abs_path)
                            line = '#%s : %08d | %s | %s | %s | %s\n' % (match.group(1), FORMAT_VERSION_2[0], rel_path, mod_hash, datetime.utcnow().strftime("%Y-%m-%dT%H-%M-%S"), git_hash)
                        print("Updating:\n" + line)
                else:
                    print(f"Warning: Could not resolve relative path {rel_path}. Override line not updated.\n")

            RegenInfOutData += line

        with open (Paths.RegenPath, "w") as fd:
            fd.write(RegenInfOutData)

    else:
        dummy_list = []
        pathtool = Edk2Path(Paths.WorkSpace, dummy_list)

        # Generate and print the override for pasting into the file.
        # Use absolute module path to find package path
        pkg_path = pathtool.GetContainingPackage(Paths.TargetPath)
        if pkg_path is not None:
            rel_path = Paths.TargetPath[Paths.TargetPath.find(pkg_path):]
        else:
            rel_path = pathtool.GetEdk2RelativePathFromAbsolutePath(Paths.TargetPath)
            if not rel_path:
                print(f"{Paths.TargetPath} is invalid for this workspace.")
                sys.exit(1)

        rel_path = rel_path.replace('\\', '/')
        mod_hash = ModuleHashCal(Paths.TargetPath)

        VERSION_INDEX = Paths.Version - 1

        if VERSION_INDEX == 0:
            print("Copy and paste the following line(s) to your overrider inf file(s):\n")
            print('#%s : %08d | %s | %s | %s' % ("Override" if not Paths.Track else "Track", FORMAT_VERSION_1[0], rel_path, mod_hash, datetime.utcnow().strftime("%Y-%m-%dT%H-%M-%S")))

        elif VERSION_INDEX == 1:
            git_hash = ModuleGitHash(Paths.TargetPath)
            print("Copy and paste the following line(s) to your overrider inf file(s):\n")
            print('#%s : %08d | %s | %s | %s | %s' % ("Override" if not Paths.Track else "Track", FORMAT_VERSION_2[0], rel_path, mod_hash, datetime.utcnow().strftime("%Y-%m-%dT%H-%M-%S"), git_hash))
