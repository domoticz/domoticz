local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Thermostat setpoint device adapter',

	matches = function (device)
		return (device.deviceType == 'Thermostat' and device.deviceSubType == 'SetPoint')
	end,

	process = function (device, data, domoticz)

		device['SetPoint'] = device.rawData[1] or 0

	end

}