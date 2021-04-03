local humidityMapping = {
	['dry'] = 2,
	['normal'] = 0,
	['comfortable'] = 1,
	['wet'] = 3
}

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

		device.humidityStatusValue = humidityMapping[string.lower(device.humidityStatus or '')] or -1

		function device.updateTempHum(temperature, humidity, status)
			if status == nil or status == -1 then status = utils.humidityStatus(temperature, humidity) end -- HUM_COMPUTE or nil

			local value = temperature .. ';' .. humidity .. ';' .. status

			return device.update(0, value)
		end

	end

}