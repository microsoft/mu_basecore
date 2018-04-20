import os
import logging
import fnmatch

#
# Class to help convert from absolute path to EDK2 build path
# using workspace and packagepath variables
#
class Edk2Path(object):
    def __init__(self, ws, packagepathlist):
        self.WorkspacePath = ws
        self.PackagePathList = packagepathlist


    def GetEdk2RelativePathFromAbsolutePath(self, abspath):
        relpath = None
        found = False
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
            return relpath.lstrip(os.sep)

        #didn't find the path for conversion. 
        logging.error("Failed to convert AbsPath to Edk2Relative Path")
        logging.error("AbsolutePath: %s" % abspath)
        return None
    
    def GetAbsolutePathOnThisSytemFromEdk2RelativePath(self, relpath):
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
        logging.info("PackagePath is: %s" % ";".join(self.PackagePathList))
        logging.info("Workspace path is : %s" % self.WorkspacePath)
        return None


        
