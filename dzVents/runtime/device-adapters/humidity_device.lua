local humidityMapping = {
	['dry'] = 2,
	['normal'] = 0,
	['comfortable'] = 1,
	['wet'] = 3
}

return {

	baseType = 'device',

	name = 'Humidty device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Humidity')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateHumidity')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: humidityStatus, humidity

		local humVal = humidityMapping[string.lower(device.humidityStatus or '')] or -1
		device.humidityStatusValue = humVal

		function device.updateHumidity(humidity, status)
			--[[
				status can be
				domoticz.HUM_NORMAL
				domoticz.HUM_COMFORTABLE
				domoticz.HUM_DRY
				domoticz.HUM_WET
			 ]]

			if (status == nil) then
				-- when no status is provided, domoticz will not set the device obviously
				utils.log('No status provided. Humidity not set', utils.LOG_ERROR)
				return
			end

			return device.update(humidity, status)
		end

	end


}