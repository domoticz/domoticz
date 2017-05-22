local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'P1 smart meter energy device adapter',

	matches = function (device)
		return (device.deviceType == 'P1 Smart Meter' and device.deviceSubType == 'Energy')
	end,

	process = function (device, data, domoticz)

		device['WhActual'] = device.rawData[5] or 0

	end

}