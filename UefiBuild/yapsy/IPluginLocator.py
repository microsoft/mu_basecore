# -*- coding: utf-8; tab-width: 4; indent-tabs-mode: t; python-indent: 4 -*-



"""
Role
====

``IPluginLocator`` defines the basic interface expected by a
``PluginManager`` to be able to locate plugins and get basic info
about each discovered plugin (name, version etc).

API
===

"""


from yapsy import log

class IPluginLocator(object):
	"""
	Plugin Locator interface with some methods already implemented to
	manage the awkward backward compatible stuff.
	"""
	
	def locatePlugins(self):
		"""
		Walk through the plugins' places and look for plugins.

		Return the discovered plugins as a list of
		``(candidate_infofile_path, candidate_file_path,plugin_info_instance)``
		and their number.
		"""
		raise NotImplementedError("locatePlugins must be reimplemented by %s" % self)
	
	def gatherCorePluginInfo(self, directory, filename):
		"""
		Return a ``PluginInfo`` as well as the ``ConfigParser`` used to build it.
		
		If filename is a valid plugin discovered by any of the known
		strategy in use. Returns None,None otherwise.
		"""
		raise NotImplementedError("gatherPluginInfo must be reimplemented by %s" % self)
	