local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Temperature+humidity+barometer device adapter',

	matches = function (device)
		return (device.deviceType == 'Temp + Humidity + Baro')
	end,

	process = function (device, data, domoticz, utils)

		function device.updateTempHumBaro(temperature, humidity, status, pressure, forecast)

			if (status == nil) then
				-- when no status is provided, domoticz will not set the device obviously
				utils.log('No status provided. Temperature + humidity not set', utils.LOG_ERROR)
				return
			end


			local value = tostring(temperature) .. ';' ..
					tostring(humidity) .. ';' ..
					tostring(status) .. ';' ..
					tostring(pressure) .. ';' ..
					tostring(forecast)
			device.update(0, value)
		end

	end


}