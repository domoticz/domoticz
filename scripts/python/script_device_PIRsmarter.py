import domoticz

# similar to http://www.domoticz.com/wiki/Smart_Lua_Scripts
# name is PIR <options> <switch controlled>
# options can be day/night/all
# example names:
# Pir day-night Slaapkamer groot
# PIR all Slaapkamer groot
# PIR night Slaapkamer groot

if 1: # to disable set to if 0:
	parts = changed_device.name.split(" ", 2)
	if len(parts) == 3 and parts[0].lower() == "pir": # case insensitive

		options = parts[1]
		device_name_controlled = parts[2]

		if (is_daytime and "day" in options) or (is_nighttime and "night" in options) or "all" in options:
			if device_name_controlled in domoticz.devices:
				device = domoticz.devices[device_name_controlled]
				device.on() # will only trigger a command when device is off (so we don't have to check device.is_off())
				# 240 seconds is default, overrride with pir-timeout user variable
				# see https://docs.python.org/2/library/stdtypes.html#dict for understanding .get
				timeon = domoticz.user_variables.get("pir-timeout", 10)
				domoticz.log("PIR action, turned on %s, will turn off after %d seconds" % (device_name_controlled, timeon))
				device.off(after=timeon)
			else:
				domoticz.log("Device not found:", device_name_controlled)
