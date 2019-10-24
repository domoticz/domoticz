return {

	baseType = 'device',

	name = 'Wind device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Wind')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateWind')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.speedMs = utils.round(tonumber(device.rawData[3]) / 10, 1)
		device.gustMs = utils.round(tonumber(device.rawData[4]) / 10, 1)

		device.gust = tonumber(device.rawData[4]) / 10     -- Until gust is in data ( like speed already is ) we take it from sValues
		device.temperature = tonumber(device.rawData[5])
		device.chill = tonumber(device.rawData[6])

		function device.updateWind(bearing, direction, speed, gust, temperature, chill)
			local value = tostring(bearing) .. ';' ..
				tostring(direction) .. ';' ..
				tostring(speed * 10) .. ';' ..
				tostring(gust * 10) .. ';' ..
				tostring(temperature) .. ';' ..
				tostring(chill)
			return device.update(0, value)
		end
	end

}
