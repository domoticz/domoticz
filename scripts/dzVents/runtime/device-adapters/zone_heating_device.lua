local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Zone heating device adapter',

	matches = function (device)
		return (device.deviceType == 'Heating' and device.deviceSubType == 'Zone')
	end,

	process = function (device)

		device['setPoint'] =  tonumber(device.rawData[2])
		device['heatingMode'] = device.rawData[3]

	end

}