-- See README.md for instructions

return {
	active = false,                  -- set to true to activate this script
	on = {
		'My switch',                 -- name of the device
		'My sensor_Temperature',     -- better not use but check device.attributeIsChanged('temperature')
		'My sensor',
		258,                         -- index of the device
		['timer'] = 'every minute',  -- see readme for more options and schedules
		'PIR_*',                     -- triggers for all devices which name begins with PIR_
	},

	execute = function(domoticz, mySwitch) -- see readme for what you get
		-- see readme for the entire domoticz object tree
		-- mySwitch is a Device object with all the properties of the device that was updated
		-- unless this is a timer script, then there is not second parameter to this execute function

	end
}