from yapsy.IPlugin import IPlugin

class IUefiBuildPlugin(IPlugin):
	"""
	The most simple interface for a UEFI build plugin  Pre and Post build hooks
	"""

	def __init__(self):
		self.is_activated = True

####
# Functional interface for IUefiBuildPlugin
####

	def do_post_build(self, thebuilder):
		'''
		Run Post build Operation
		'''
		return 0

	def do_pre_build(self, thebuilder):
		'''
		Run Pre build Operation
		'''
		return 0


# For some reason can't seem to make IPlugin work
# but subclasses do.  This might have to do with
# how categories are supported.  See the tips and tricks of yapsy
# for more information about the odd behavior.  

#this class should be used for plugins that provide
#functions to the build environment.
class IUefiHelperPlugin(IPlugin):

	def __init__(self):
		self.is_activated = True

	####
	# Functional interface for iUefiHelperPlugin
	####

	#
	# Function that allows plugin to register its functions with the
	# obj.  
	# @param obj[in, out]: HelperFunctions object that allows functional 
	# registration.  
	#
	def RegisterHelpers(self, obj):
		pass




