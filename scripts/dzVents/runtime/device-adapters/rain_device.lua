local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Rain device',

	matches = function (device)
		return (device.deviceType == 'Rain')
	end,

	process = function (device, data, domoticz)

		device['updateRain'] = function (rate, counter)
			device.update(0, tostring(rate) .. ';' .. tostring(counter))
		end

	end

}