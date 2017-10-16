return {

	baseType = 'device',

	name = 'Gas device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Gas')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateGas')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: counterToday, counter

		local formatted = device.counterToday or ''
		local info = adapterManager.parseFormatted(formatted, domoticz['radixSeparator'])
		device['counterToday'] = info['value']

		formatted = tostring(device.counter) or ''
		info = adapterManager.parseFormatted(formatted, domoticz['radixSeparator'])
		device['counter'] = info['value']

		function device.updateGas(usage)
			--[[
				USAGE= Gas usage in liter (1000 liter = 1 m³)
				So if your gas meter shows f.i. 145,332 m³ you should send 145332.
				The USAGE is the total usage in liters from start, not f.i. the daily usage.
			 ]]
			return device.update(0, usage)
		end

	end

}