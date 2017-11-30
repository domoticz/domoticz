local BATTERY_THRESHOLD = 10

return {
	active = true,
	on = {
		['timer'] = {
			'every hour'
		}
	},
	execute = function(domoticz)

		local message = ''

		-- first filter on low battery level
		local lowOnBat = domoticz.devices().filter(function(device)

			local level = device.batteryLevel -- level is 0-100
			return (level ~= nil and -- not all devices have this attribute
					level <= BATTERY_THRESHOLD)

		end)

		-- then loop over the results
		lowOnBat.forEach(function(lowDevice)

			message = message .. 'Device ' ..
				lowDevice.name .. ' is low on batteries (' .. tostring(lowDevice.batteryLevel) .. '), '

		end)

		if (message ~= '') then
			domoticz.notify('Low battery warning', message, domoticz.PRIORITY_NORMAL)
			domoticz.log('Low battery warning: ' .. message, domoticz.LOG_ERROR)
		end
	end
}

