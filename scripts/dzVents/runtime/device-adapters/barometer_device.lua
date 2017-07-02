local constMapping = {
	['stable'] = 0,
	['sunny'] = 1,
	['cloudy'] = 2,
	['unstable'] = 3,
	['thunderstorm'] = 4
}

return {

	baseType = 'device',

	name = 'Barometer device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Barometer')

		if (not res) then
			adapterManager.addDummyMethod(device, 'updateBarometer')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)
		-- from data: barometer, forecast, forecastString

		function device.updateBarometer(pressure, forecast)
			-- pressure in hPa

			forecast = forecast ~= nil and constMapping[forecast] or 5

			device.update(0, tostring(pressure) .. ';' .. tostring(forecast))
		end

	end

}