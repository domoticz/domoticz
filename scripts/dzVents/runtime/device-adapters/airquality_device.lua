return {

	baseType = 'device',

	name = 'Air quality device',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Air Quality')

		if (not res) then
			adapterManager.addDummyMethod(device, 'updateAirQuality')
		end

		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: quality

		-- todo this doesn't work just yet
		device['co2'] = tonumber(device.state) -- co2 (ppm)

		-- quality is automatically added (named)

		device['updateAirQuality'] = function(quality)
			device.update(quality)
		end

	end

}