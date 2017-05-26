return {

	baseType = 'device',

	name = 'Air quality device',

	matches = function (device)
		return (device.deviceType == 'Air Quality')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- todo this doesn't work just yet
		device['co2'] = tonumber(device.state) -- co2 (ppm)

		-- quality is automatically added (named)

		device['updateAirQuality'] = function(quality)
			device.update(quality)
		end

	end

}