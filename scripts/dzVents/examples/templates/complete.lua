return {

	-- active controls if this entire script is considered or not
	-- pick one of these three modes:
	active = true,
	active = false, -- set to true to make it execute
	active = function(domoticz)
		-- function must return a true or false value
		return domoticz.time.isNightTime -- only run script at nighttime
	end,

	-- optional custom logging, overriding the settings
	logging = {
		level = domoticz.LOG_DEBUG,
		marker = "My marker" -- prefix added to every log message
	},

	-- optional persistent data
	data = {
		myPersistentVar = { initial = 10 },
		anotherVar = { initial = { x = 1, y = 2 } }, -- anotherVar is an object
		historyVar = { history = true, maxItems = 15 },
		anotherHistoryVar = {
			history = true,
			maxHours = 1,
			getValue = function(item) -- custom getter function to get a value from the items
				return item.data.num
			end
		},
	},

	-- trigger
	-- can be a combination
	on = {
		devices = {
			-- scripts is executed if the device that was updated matches with one of these triggers
			'device name', -- device name
			['another device'] = { 'at *:30' }, --device with time limitation
			['device1'] = { 'at sunset on sat, sun, mon' }, -- with time limitation
			'device2',
			'device3',
			'abc*', -- triggers for all devices which name begins with abc
			258, -- id
		},

		timer = {
			-- timer triggers.. if one matches with the current time then the script is executed
			'at 13:45',
			'at 18:37',
			'every 3 minutes'
		},

		variables = {
			-- script is executed if the variable that was changed is lister here
			'x',
			'y',
			'z'  -- collections of uservariable changes
		},

		security = {
			-- security events, if one of them matches with the current security state, the script is executed
			domoticz.SECURITY_ARMEDAWAY,
			domoticz.SECURITY_ARMEDHOME
		}
	},

	-- actual event code
	-- in case of an uservariable change event, device is the variable
	-- in case of a timer event or security event, device == nil
	execute = function(domoticz, device, info)
		--[[

		The domoticz object holds all information about your Domoticz system. E.g.:

		local myDevice = domoticz.devices['myDevice']
		local myVariable = domoticz.variables['myUserVariable']
		local myGroup = domoticz.groups['myGroup']
		local myScene = domoticz.scenes['myScene']

		The device object is the device that was triggered due to the device in the 'on' section above.
		The device object == nil when the script was triggered due to a timer rule or a security event rule.

		The info object holds information about what triggered the script:
		- info.type: domoticz.EVENT_TYPE_DEVICE, domoticz.EVENT_TYPE_TIMER, domoticz.EVENT_TYPE_VARIABLE
		- info.trigger: which timer rule triggered the script in case the script was called due to a timer event or
		                which security state triggered the script in case of a security trigger

		]] --

		-- example
		if (device.state == 'On') then
			device.switchOff().after_min(2)
			domoticz.nofity('Light info', 'The light ' .. device.name .. ' will be switched off soon')
		end
	end
}
