local constMapping = {
	['noinfo'] = 0,
	['sunny'] = 1,
	['partlycloudy'] = 2,
	['cloudy'] = 3,
	['rain'] = 4
}

local humidityMapping = {
	['dry'] = 2,
	['normal'] = 0,
	['comfortable'] = 1,
	['wet'] = 3
}


return {

	baseType = 'device',

	name = 'Temperature+humidity+barometer device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Temp + Humidity + Baro')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateTempHumBaro')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: barometer, dewPoint, humidity, forecast
		-- humidityStatus, forecastString, temperature

		local humVal = humidityMapping[string.lower(device.humidityStatus or '')] or -1
		device.humidityStatusValue = humVal


		function device.updateTempHumBaro(temperature, humidity, status, pressure, forecast)

			if (status == nil) then
				-- when no status is provided, domoticz will not set the device obviously
				utils.log('No status provided. Temperature + humidity not set', utils.LOG_ERROR)
				return
			end

			forecast = forecast ~= nil and constMapping[forecast] or 5

			local value = tostring(temperature) .. ';' ..
					tostring(humidity) .. ';' ..
					tostring(status) .. ';' ..
					tostring(pressure) .. ';' ..
					tostring(forecast)
			return device.update(0, value)
		end

	end

}