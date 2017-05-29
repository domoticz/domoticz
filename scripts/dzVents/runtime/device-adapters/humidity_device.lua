return {

	baseType = 'device',

	name = 'Humidty device adapter',

	matches = function (device)
		return (device.deviceType == 'Humidity')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

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

			device.update(humidity, status)
		end

	end


}