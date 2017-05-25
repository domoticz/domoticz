local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Wind device adapter',

	matches = function (device)
		return (device.deviceSubType == 'Barometer')
	end,

	process = function (device, data, domoticz, utils)

		--[[
			forecast:
			 domoticz.BARO_STABLE
			domoticz.BARO_SUNNY
			domoticz.BARO_CLOUDY
			domoticz.BARO_UNSTABLE
			domoticz.BARO_THUNDERSTORM
			domoticz.BARO_UNKNOWN
			domoticz.BARO_CLOUDY_RAIN
		 ]]
		function device.updateBarometer(pressure, forecast)
			device.update(0, tostring(pressure) .. ';' .. tostring(forecast))
		end

	end


}