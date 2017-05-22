local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'kWh device adapter',

	matches = function (device)
		return (device.deviceType == 'General' and device.deviceSubType == 'kWh')
	end,

	process = function (device, data, domoticz)

		local todayFormatted = device.counterToday or ''
		local todayInfo = adapters.parseFormatted(todayFormatted, domoticz['radixSeparator'])

		-- as this is a kWh device we assume the value is in kWh
		-- so we have to multiply it with 1000 to get it in W

		device['WhToday'] = todayInfo['value'] * 1000

	end

}