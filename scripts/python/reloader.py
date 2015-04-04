"""
Python only loads modules once, so domoticz will only load a module the first time it is needed. Code changes will not be reflected, only after a restart of domoticz. Note that doesn't hold for scripts, they always get reloaded.
To have your module reloaded, you can enter the following code in it.
import reloader
reloader.auto_reload(__name__)
"""

import sys
import os


# mark module as a module to reload
def auto_reload(module_name):
	import domoticz as dz
	path = _py_source(sys.modules[module_name])
	_module_mtimes[module_name] = os.path.getmtime(path)
	print "added module %s to auto reload list" % module_name


		
# below this is internal stuff

# maps from module name to modification time (mtime)
_module_mtimes = {}

# convert mpdule to python source filename
def _py_source(module):
	path = module.__file__
	if path[:-1].endswith("py"):
		path = path[:-1]
	return path

def _check_reload():
	print "modules", _module_mtimes
	for module_name, loaded_mtime in _module_mtimes.items():
		path = _py_source(sys.modules[module_name])
		# if file is changed, the current mtime is greater
		if loaded_mtime < os.path.getmtime(path):
			#print "reloading %s module" % module_name
			reload(sys.modules[module_name]) # and reload
		else:
			#print "no reloading for %s" % module_name
			pass
