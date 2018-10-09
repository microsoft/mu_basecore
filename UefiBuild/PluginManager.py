## @file PluginManager.py
# This module contains code that supports Project Mu Build Plugins
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
import sys
import os
import imp
import logging

class PluginDescriptor(object):
    def __init__(self, t):
        self.descriptor = t
        self.Obj = None
        self.Name = t["name"]
    
    def __str__(self):
        return "PLUGIN DESCRIPTOR:{0}".format(self.Name)

class PluginManager(object):

    def __init__(self):
        self.Descriptors = []

    #
    # Pass tuple of Environment Descriptor dictionaries to be loaded as plugins
    #
    def SetListOfEnvironmentDescriptors(self, newlist):
        failed = []
        for a in newlist:
            b = PluginDescriptor(a)
            if(self._load(b) == 0):
                self.Descriptors.append(b)
            else:
                failed.append(a)
        return failed
    
    #
    # Return List of all plugins of a given class
    #
    def GetPluginsOfClass(self, classobj):
        temp = []
        for a in self.Descriptors:
            if(isinstance(a.Obj, classobj)):
                temp.append(a)
        return temp

    #
    # Return List of all plugins
    #
    def GetAllPlugins(self):
        return self.Descriptors

    #
    # Load and Instantiate the plugin
    #
    def _load(self, PluginDescriptor):
        PluginDescriptor.Obj = None
        PythonFileName = PluginDescriptor.descriptor["module"] + ".py"
        PyModulePath = os.path.join(os.path.dirname(os.path.abspath(PluginDescriptor.descriptor["descriptor_file"])), PythonFileName)
        PluginDescriptor.descriptor["module_file"] = PyModulePath
        logging.debug("Loading Plugin from %s", PyModulePath)
        try:
            with open(PyModulePath,"r") as plugin_file:
                _module = imp.load_module(
                    "UefiBuild_Plugin_" + PluginDescriptor.descriptor["module"],
                    plugin_file,
                    PyModulePath,
                    ("py","r",imp.PY_SOURCE))

        except Exception:
            exc_info = sys.exc_info()
            logging.error("Failed to import plugin: %s", PyModulePath, exc_info=exc_info)
            return -1

        # Instantiate the plugin
        try: 
            obj = getattr(_module, PluginDescriptor.descriptor["module"])
            PluginDescriptor.Obj = obj()
        except AttributeError:
            exc_info = sys.exc_info()
            logging.error("Failed to instantiate plugin: %s", PyModulePath, exc_info=exc_info)
            return -1

        return 0 
                
    

###############################################################################
##                           PLUGIN Base Classes                             ##
###############################################################################

###
# Plugin that supports Pre and Post Build steps
###
class IUefiBuildPlugin(object):

    ##
    # Run Post Build Operations
    #
    # @param thebuilder - UefiBuild object to get env information
    #
    # @return 0 for success NonZero for error. 
    ##
    def do_post_build(self, thebuilder):
        return 0

    ##
    # Run Pre Build Operations
    #
    # @param thebuilder - UefiBuild object to get env information
    #
    # @return 0 for success NonZero for error. 
    ##
    def do_pre_build(self, thebuilder):
        '''
        Run Pre build Operation
        '''
        return 0
###
# Plugin that supports Pre and Post Build steps
###
class IDscProcessorPlugin(object):

    ##
    # does the transform on the DSC	
    #
    # @param dsc - the in-memory model of the DSC
    # @param thebuilder - UefiBuild object to get env information
    #
    # @return 0 for success NonZero for error. 
    ##
    def do_transform(self, dsc, thebuilder):
        return 0

    ##
    # gets the level that this transform operates at
    #
    # @param thebuilder - UefiBuild object to get env information
    #
    # @return 0 for the most generic level
    ##
    def get_level(self, thebuilder):
        
        return 0

###
# Plugin that supports adding Extension or helper methods
# to the build environment
###
class IUefiHelperPlugin(object):
    
    ##
    # Function that allows plugin to register its functions with the
    # obj.  
    # @param obj[in, out]: HelperFunctions object that allows functional 
    # registration.  
    #
    def RegisterHelpers(self, obj):
        pass

###
# Plugin that supports adding Extension or helper methods
# to the build environment
###
class IMuBuildPlugin(object):
    
    ##
    # External function of plugin.  This function is used to perform the task of the MuBuild Plugin
    # 
    #   - package is the edk2 path to package.  This means workspace/packagepath relative.  
    #   - absolute path to workspace 
    #   - packagespath csv
    #   - any additional command line args
    #   - RepoConfig Object (dict) for the build
    #   - PkgConfig Object (dict)
    #   - EnvConfig Object 
    #   - Plugin Manager Instance
    #   - Plugin Helper Obj Instance
    #   - Junit Logger
    def RunBuildPlugin(self, packagename, Edk2pathObj, args, repoconfig, pkgconfig, environment, PLM, PLMHelper, JUnitLogger):
        pass

    ##
    # Return tuple (string, string) that is the (test case name, test case base class name)
    #
    #
    def GetTestName(self, packagename, environment):
        pass

    ##
    # Returns true or false if plugin would like to be called for each target
    ##
    def IsTargetDependent(self):
        return False

    #
    # Walks a directory for all items ending in certain extension
    # Default is to walk all of workspace
    #
    def WalkDirectoryForExtension(self, extensionlist, directory, ignorelist=None):
        if not isinstance(extensionlist, list):
            logging.critical("Expected list but got " + str(type(extensionlist)))
            return -1

        if directory is None:
            logging.critical("No directory given")
            return -2

        if not os.path.isabs(directory):
            logging.critical("Directory not abs path")
            return -3

        if not os.path.isdir(directory):
            logging.critical("Invalid find directory to walk")
            return -4

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
    
    # Gets the DSC for a particular folder
    def get_dsc_name_in_dir(self, folderpath):
        try:
            directory = folderpath
            allEntries = os.listdir(directory)
            dscFile = None
            jsonFile = None
            for entry in allEntries:
                if entry.endswith(".dsc"):
                    dscFile = entry
                if entry.endswith(".mu.dsc.json"):
                    jsonFile = entry

            if jsonFile:
                # create the dsc file on the fly
                logging.info("We should create a DSC from the JSON file on the fly: {0}".format(jsonFile))
            if dscFile:
                return os.path.join(directory, dscFile)

            if dscFile is None and jsonFile is None:
                raise Exception()
        except:
            logging.error("UNABLE TO FIND PACKAGE {0}".format(pkg))
            return None


###############################################################################
##                           PLUGIN HELPER SUPPORT                           ##
## Supports IUefiHelperPlugin type
###############################################################################
class HelperFunctions(object):
    def __init__(self):
        self.RegisteredFunctions = {}

    #
    # Function to logging.debug all registered functions and their source path
    #
    def DebugLogRegisteredFunctions(self):
        logging.debug("Logging all Registered Helper Functions:")
        for name, file in self.RegisteredFunctions.items():
            logging.debug("  Function %s registered from file %s", name, file)
        logging.debug("Finished logging %d functions", len(self.RegisteredFunctions))

    #
    # Plugins that want to register a helper function should call
    # this routine for each function
    #
    # @param name[in]: name of function
    # @param function[in] function being registered
    # @param filepath[in] filepath registering function.  used for tracking and debug purposes
    #
    def Register(self, name, function, filepath):
        if(name in self.RegisteredFunctions.keys()):
            raise Exception("Function %s already registered from plugin file %s.  Can't register again from %s" % (name, self.RegisteredFunctions[name], filepath))
        setattr(self, name, function)
        self.RegisteredFunctions[name] = filepath

    def HasFunction(self, name):
        if(name in self.RegisteredFunctions.keys()):
            return True
        else:
            return False


    def LoadFromPluginManager(self,pluginManager):
        for Descriptor in pluginManager.GetPluginsOfClass(IUefiHelperPlugin):
            
            logging.info(Descriptor)
            logging.debug("Helper Plugin Register: %s", Descriptor.Name)
            try:
                Descriptor.Obj.RegisterHelpers(self)
            except:
                logging.warning("Unable to register {0}".format(Descriptor.Name))
                pass






