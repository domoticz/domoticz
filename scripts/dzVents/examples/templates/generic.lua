return {
	active = false, -- set to true to activate this script

	-- the on-sections holds the rules for when the script should be executed
	-- this can be a combination of device, variable or time triggers
	-- e.g.:
	on = {
		'devicename', -- name of the device
		258, -- id of the device (see devices in Domoticz)
		['timer'] = 'every minute', -- see readme for more options and schedules
		'abc*', -- triggers for all devices which name begins with abc
	},

	-- execute is the function that is executed when active == true and a trigger was met
	execute = function(domoticz, device, info)
		--[[

			The domoticz object holds all information about your Domoticz system. E.g.:

			local myDevice = domoticz.devices['myDevice']
			local myVariable = domoticz.variables['myUserVariable']
			local myGroup = domoticz.groups['myGroup']
			local myScene = domoticz.sceneds['myScene']

		    The device object is the device that was triggered due to the device in the 'on' section above.
		    The device object == nil when the script was triggered due to a timer rule.

            The info object holds information about what triggered the script:
            - info.type: domoticz.EVENT_TYPE_DEVICE, domoticz.EVENT_TYPE_TIMER, domoticz.EVENT_TYPE_VARIABLE
            - info.trigger: which timer rule triggered the script in case the script was called due to a timer event

		]] --

		-- example
		if (device.state == 'On') then
			device.switchOff().after_min(2)
			domoticz.nofity('Light info', 'The light ' .. device.name .. ' will be switched off soon')
		end
	end
}