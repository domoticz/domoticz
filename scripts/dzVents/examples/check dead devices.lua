
local devicesToCheck = {
	{ ['name'] = 'Sensor1', ['threshold'] = 30 },
	{ ['name'] = 'Sensor2', ['threshold'] = 30 },
	{ ['name'] = 'Bathroom temperature', ['threshold'] = 20 }
}

return {
	active = true,
	on = {
		['timer'] = {
			'every 5 minutes'
		}
	},
	execute = function(domoticz)

		local message = ""

		for i, deviceToCheck in pairs(devicesToCheck) do
			local name = deviceToCheck['name']
			local threshold = deviceToCheck['threshold']
			local minutes = domoticz.devices(name).lastUpdate.minutesAgo

			if ( minutes > threshold) then
				message = message .. 'Device ' ..
						name .. ' seems to be dead. No heartbeat for at least ' ..
						minutes .. ' minutes.\r'
			end
		end

		if (message ~= "") then
			domoticz.email('Dead devices', message, 'me@address.nl')
			domoticz.log('Dead devices found: ' .. message, domoticz.LOG_ERROR)
		end
	end
}

