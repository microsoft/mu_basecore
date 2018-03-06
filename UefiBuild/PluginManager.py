import sys
import os
import imp
import logging

class PluginDescriptor(object):
	def __init__(self, t):
		self.descriptor = t
		self.Obj = None
		self.Name = t["name"]

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






