local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'kWh device adapter',

	matches = function (device)
		return (device.deviceSubType == 'kWh')
	end,

	process = function (device, data, domoticz)
		local formatted = device.counterToday or ''
		local info = adapters.parseFormatted(formatted, domoticz['radixSeparator'])

		-- as this is a kWh device we assume the value is in kWh
		-- so we have to multiply it with 1000 to get it in W

		device['WhToday'] = info['value'] * 1000
		device['counterToday'] = info['value']

		formatted = device.usage or ''
		info = adapters.parseFormatted(formatted, domoticz['radixSeparator'])

		device.usage = info['value']

		function device.updateElectricity(power, energy)
			device.update(0, tostring(power) .. ';' .. tostring(energy))
		end
	end

}