return {

	baseType = 'device',

	name = 'Lux device adapter',

	matches = function (device)
		return (device.deviceType == 'Lux' and device.deviceSubType == 'Lux')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- set the lux value
		device['lux'] = tonumber(device.rawData[1])

		device['updateLux'] = function (lux)
			device.update(0, lux)
		end

	end

}