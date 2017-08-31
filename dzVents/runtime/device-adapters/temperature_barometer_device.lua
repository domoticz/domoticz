local constMapping = {
	['stable'] = 0,
	['sunny'] = 1,
	['cloudy'] = 2,
	['unstable'] = 3,
	['thunderstorm'] = 4
}

return {

	baseType = 'device',

	name = 'Temperature+barometer device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Temp + Baro')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateTempBaro')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: barometer, forecast
		-- forecastString, temperature

		function device.updateTempBaro(temperature, pressure, forecast)

			forecast = forecast ~= nil and constMapping[forecast] or 5

			local value = tostring(temperature) .. ';' ..
					tostring(pressure) .. ';' ..
					tostring(forecast)
			return device.update(0, value)
		end

	end

}