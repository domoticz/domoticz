local forecastMapping = {
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

	name = 'Thermostat 6 device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Thermostat 6')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateThermostat')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: temperature, setPoint
		-- optional: humidity, humidityStatus, barometer, forecast

		device.setPoint = tonumber(device.rawData[2] or 0)

		if device.humidity then
			device.humidityStatusValue = humidityMapping[string.lower(device.humidityStatus or '')] or -1
		end

		function device.updateSetPoint(setPoint)
			return device.update(0, ';' .. setPoint)
		end

		function device.updateThermostat(temperature, setPoint, humidity, humidityStatus, barometer, forecast)
			local value = (temperature or '') .. ';' .. (setPoint or '')

			if device.deviceSubType == 'Temp/Hum/Setpoint' or device.deviceSubType == 'Temp/Hum/Baro/Setpoint' then
				if humidity and (humidityStatus == nil or humidityStatus == -1) then
					humidityStatus = utils.humidityStatus(temperature, humidity)
				end
				value = value .. ';' .. (humidity or '') .. ';' .. (humidityStatus or '')
			end

			if device.deviceSubType == 'Temp/Baro/Setpoint' or device.deviceSubType == 'Temp/Hum/Baro/Setpoint' then
				forecast = forecast and forecastMapping[forecast] or forecast or ''
				value = value .. ';' .. (barometer or '') .. ';' .. forecast
			end

			return device.update(0, value)
		end

	end

}
