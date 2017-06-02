return {

	active = false, -- set to true to make it execute

	--	active = function(domoticz)
	--		return domoticz.time.isNightTime -- only run script at nighttime
	--	end,

	-- custom logging, overriding the settings
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
		'device name', -- device name
		['another device'] = { 'at *:30' }, --device with time limitation
		devices = { -- collection of devices
			['device1'] = { 'at sunset on sat, sun, mon' }, -- with time limitation
			'device2',
			'device3'
		},
		258, -- id
		['timer'] = { 'at 13:45', 'at 18:37', 'every 3 minutes' },
		'abc*', -- triggers for all devices which name begins with abc

		variable = 'x', -- uservariable x changes
		variables = {
			'y', 'z'  -- collections of uservariable changes
		}
	},

	-- actual event code
	-- in case of an uservariable change event, device is the variable
	-- in case of a timer event, device == nil
	execute = function(domoticz, device, info)
		-- code
	end
}