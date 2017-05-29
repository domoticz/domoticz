return {

	baseType = 'device',

	name = 'Rain device',

	matches = function (device)
		return (device.deviceType == 'Rain')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device['updateRain'] = function (rate, counter)
			device.update(0, tostring(rate) .. ';' .. tostring(counter))
		end

	end

}