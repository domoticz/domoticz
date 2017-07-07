return {

	baseType = 'device',

	name = 'Temperature+humidity device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Temp + Humidity')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateTempHum')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: dewPoint, humidity, humidityStatus, temperature

		function device.updateTempHum(temperature, humidity, status)
			if (status == nil) then
				-- when no status is provided, domoticz will not set the device obviously
				utils.log('No status provided. Temperature + humidity not set', utils.LOG_ERROR)
				return
			end

			local value = tostring(temperature) .. ';' .. tostring(humidity) .. ';' .. tostring(status)
			device.update(0, value)
		end

	end

}