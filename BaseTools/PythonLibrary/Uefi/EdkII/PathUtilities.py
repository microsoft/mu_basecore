## @file PathUtilities.py
# Code to help convert Edk2, absolute, and relative file paths
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
import os
import logging
import fnmatch

#
# Class to help convert from absolute path to EDK2 build path
# using workspace and packagepath variables
#
class Edk2Path(object):

    #
    # ws - absolute path or cwd relative to workspace
    # packagepathlist - list of packages path.  Absolute path list or workspace relative path
    #
    def __init__(self, ws, packagepathlist):
        self.WorkspacePath = ws
        if( not os.path.isabs(ws)):
            self.WorkspacePath =  os.path.abspath(os.path.join(os.getcwd(), ws))

        if(not os.path.isdir(self.WorkspacePath)):
            logging.error("Workspace path invalid.  {0}".format(ws))
            raise Exception("Workspace path invalid.  {0}".format(ws))
        
        # Set PackagePath
        self.PackagePathList = list()
        for a in packagepathlist:
            if(os.path.isabs(a)):
                self.PackagePathList.append(a)
            else:
                #see if workspace relative
                wsr = os.path.join(ws, a)
                if(os.path.isdir(wsr)):
                    self.PackagePathList.append(wsr)
                else:
                    #assume current working dir relative.  Will catch invalid dir when checking whole list
                    self.PackagePathList.append(os.path.abspath(os.path.join(os.getcwd(), a)))
                    
        error = False
        for a in self.PackagePathList:
            if(not os.path.isdir(a)):
                logging.error("Invalid package path entry {0}".format(a))
                error = True

        #report error
        if(error):
            raise Exception("Invalid package path directory(s)")


    def GetEdk2RelativePathFromAbsolutePath(self, abspath):
        relpath = None
        found = False
        if abspath is None:
            return None
        for a in self.PackagePathList:
            stripped = abspath.lower().partition(a.lower())[2]
            if stripped:  
                #found our path...now lets correct for case
                relpath = abspath[len(a):]
                found = True
                logging.debug("Successfully converted AbsPath to Edk2Relative Path using PackagePath")
                logging.debug("AbsolutePath: %s found in PackagePath: %s" % (abspath, a))
                break

        if(not found):
            #try to strip the workspace
            stripped = abspath.lower().partition(self.WorkspacePath.lower())[2]
            if stripped:  
                #found our path...now lets correct for case
                relpath = abspath[len(self.WorkspacePath):]
                found = True
                logging.debug("Successfully converted AbsPath to Edk2Relative Path using WorkspacePath")
                logging.debug("AbsolutePath: %s found in Workspace: %s" % (abspath, self.WorkspacePath))
            
        if(found):
            relpath = relpath.replace(os.sep, "/")
            return relpath.lstrip("/")

        #didn't find the path for conversion. 
        logging.error("Failed to convert AbsPath to Edk2Relative Path")
        logging.error("AbsolutePath: %s" % abspath)
        return None
    
    def GetAbsolutePathOnThisSytemFromEdk2RelativePath(self, relpath):
        relpath = relpath.replace("/", os.sep)
        abspath = os.path.join(self.WorkspacePath, relpath)
        if os.path.exists(abspath):
            return abspath

        for a in self.PackagePathList:
            abspath = os.path.join(a, relpath)
            if(os.path.exists(abspath)):
                return abspath
        logging.error("Failed to convert Edk2Relative Path to an Absolute Path on this system.")
        logging.error("Relative Path: %s" % relpath)
        
        return None

    # Find the package this path belongs to using
    # some Heuristic.  This isn't perfect but at least 
    # identifies the directory consistently
    #
    # @param InputPath:  absolute path to module
    #
    # @ret Name of Package that the module is in. 
    def GetContainingPackage(self, InputPath):
        logging.debug("GetContainingPackage: %s" % InputPath)

        dirpathprevious = os.path.dirname(InputPath)
        dirpath = os.path.dirname(InputPath)
        while( dirpath is not None):
            #
            # if at the root of a packagepath return the previous dir.  
            # this catches cases where a package has no DEC
            #
            if(dirpath in self.PackagePathList):
                a = os.path.basename(dirpathprevious)
                logging.debug("Reached Package Path.  Using previous directory: %s" % a)
                return a
            #
            # if at the root of the workspace return the previous dir.
            # this catches cases where a package has no DEC
            #
            if(dirpath == self.WorkspacePath):
                a = os.path.basename(dirpathprevious)
                logging.debug("Reached Workspace Path.  Using previous directory: %s" % a)
                return a
            #
            # Check for a DEC file in this folder
            # if here then return the directory name as the "package"
            #
            for f in os.listdir(dirpath):
                if fnmatch.fnmatch(f, '*.dec'):
                    a = os.path.basename(dirpath)
                    logging.debug("Found DEC file at %s.  Pkg is: %s", dirpath, a)
                    return a
            
            dirpathprevious = dirpath
            dirpath = os.path.dirname(dirpath)

        logging.error("Failed to find containing package for %s" % InputPath)
        logging.info("PackagePath is: %s" % os.pathsep.join(self.PackagePathList))
        logging.info("Workspace path is : %s" % self.WorkspacePath)
        return None


        
