local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Gas device adapter',

	matches = function (device)
		return (device.deviceSubType == 'Gas')
	end,

	process = function (device, data, domoticz)


		local formatted = device.counterToday or ''
		local info = adapters.parseFormatted(formatted, domoticz['radixSeparator'])
		device['counterToday'] = info['value']

		formatted = tostring(device.counter) or ''
		info = adapters.parseFormatted(formatted, domoticz['radixSeparator'])
		device['counter'] = info['value']

		function device.updateGas(usage)
			--[[
				USAGE= Gas usage in liter (1000 liter = 1 m³)
				So if your gas meter shows f.i. 145,332 m³ you should send 145332.
				The USAGE is the total usage in liters from start, not f.i. the daily usage.
			 ]]
			device.update(0, usage)
		end

	end

}