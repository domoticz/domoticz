"""
Helper module for python+domoticz
Every script_device_*.py script has the following variables
 * changed_device: the current device that changed (object of Device)
 * changed_device_name: name of current device (same as changed_device.name)
 * is_daytime: boolean, true when it is is daytime
 * is_nighttime: same for the night
 * sunrise_in_minutes: integer
 * sunset_in_minutes: integer
 * user_variables: dictionary from string to value

For more advanced project, you may want to use modules.
say you have a file heating.py
You can do
import heating
heating.check_heating()

But heating has a different scope, and won't have access to these variables.
By importing this module, you can have access to them in a standard way
import domoticz
domoticz.changed_device

And you also have access to more:
 * same as in the device scripts (changed_device ... user_variables)
 * devices: dictionary from string to Device


"""
try:
	import domoticz_ #
except:
	pass
import reloader
import datetime
import re

#def _log(type, text): # 'virtual' function, will be modified to call
#	pass

def log(*args):
	domoticz_.log(0, " ".join([str(k) for k in args]))

def error(*args):
	domoticz_.log(1, " ".join([str(k) for k in args]))

reloader.auto_reload(__name__)

# this will be filled in by the c++ part
devices = {}
device = None


testing = False
commands = []

event_sytem = None # will be filled in by domoticz
def command(name, action, file):
	if testing:
		commands.append((name, action))
	else:
		event_system.command(name, action, file)

def process_commands():
	for name, action in commands:
		device = devices[name]
		if action == "On":
			device.n_value = 1
			print "\tsetting %s On" % name
		elif action == "Off":
			device.n_value = 0
			print "\tsetting %s On" % name
		else:
			print "unknown command", name, action
	del commands[:]


class Device(object):
	def __init__(self, id, name, type, sub_type, switch_type, n_value, n_value_string, s_value, last_update):
		devices[name] = self
		self.id = id
		self.name = name
		self.type = type
		self.sub_type = sub_type
		self.switch_type = switch_type
		self.n_value = n_value
		self.n_value_string = n_value_string
		self.s_value = s_value
		self.last_update_string = last_update
		# from http://stackoverflow.com/questions/127803/how-to-parse-iso-formatted-date-in-python
		if self.last_update_string:
			# from http://stackoverflow.com/questions/127803/how-to-parse-iso-formatted-date-in-python
			try:
				self.last_update = datetime.datetime(*map(int, re.split('[^\d]', self.last_update_string)[:]))
				if str(self.last_update_string) != str(self.last_update):
					log("Error parsing date:", self.last_update_string, " parsed as",  str(self.last_update))
			except:
				log("Exception while parsing date:", self.last_update_string)
		else:
			self.last_update = None

        def __str__(self):
            return "Device(%s, %s, %s, %s)" % (self.id, self.name, self.n_value, self.s_value)
        def __repr__(self):
            return "Device(%s, %s, %s, %s)" % (self.id, self.name, self.n_value, self.s_value)

	def last_update_was_ago(self, **kwargs):
		"""Arguments can be: days[, seconds[, microseconds[, milliseconds[, minutes[, hours[, weeks]"""
		return self.last_update + datetime.deltatime(**kwargs) < datetime.datetime.now()

	def is_on(self):
		return self.n_value == 1

	def is_off(self):
		return self.n_value == 0

	def on(self, after=None, reflect=False):
		if self.is_off() or after is not None:
			self._command("On" , after=after)
		if reflect:
			self.n_value = 1

	def off(self, after=None, reflect=False):
		if self.is_on() or after is not None:
			self._command("Off" , after=after)
		if reflect:
			self.n_value = 0

	def _command(self, action, after=None):
		if after is not None:
			command(self.name, "%s AFTER %d" % (action, after), __file__)
		else:
			command(self.name, action, __file__)
