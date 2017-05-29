local constMapping = {
	['noinfo'] = 0,
	['sunny'] = 1,
	['partlycloudy'] = 2,
	['cloudy'] = 3,
	['rain'] = 4
}

return {

	baseType = 'device',

	name = 'Temperature+humidity+barometer device adapter',

	matches = function (device)
		return (device.deviceType == 'Temp + Humidity + Baro')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

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
			device.update(0, value)
		end

	end


}