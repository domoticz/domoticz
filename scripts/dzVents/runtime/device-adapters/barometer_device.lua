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

	matches = function (device)
		return (device.deviceSubType == 'Barometer')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.updateBarometer(pressure, forecast)
			-- pressure in hPa

			forecast = forecast ~= nil and constMapping[forecast] or 5

			device.update(0, tostring(pressure) .. ';' .. tostring(forecast))
		end

	end

}