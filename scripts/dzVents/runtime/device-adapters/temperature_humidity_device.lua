return {

	baseType = 'device',

	name = 'Temperature+humidity device adapter',

	matches = function (device)
		return (device.deviceType == 'Temp + Humidity')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

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