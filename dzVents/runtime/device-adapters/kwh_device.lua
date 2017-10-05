return {

	baseType = 'device',

	name = 'kWh device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'kWh')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateElectricity')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: whAtual, whTotal

		local formatted = device.counterToday or ''
		local info = adapterManager.parseFormatted(formatted, domoticz['radixSeparator'])

		-- as this is a kWh device we assume the value is in kWh
		-- so we have to multiply it with 1000 to get it in W

		device['WhToday'] = info['value'] * 1000

		-- fix casing
		device['whTotal'] = nil
		device['WhTotal'] = data.data.whTotal
		device['whActual'] = nil
		device['WhActual'] = data.data.whActual

		device['counterToday'] = info['value']

		formatted = device.usage or ''
		info = adapterManager.parseFormatted(formatted, domoticz['radixSeparator'])

		device.usage = info['value']

		function device.updateElectricity(power, energy)
			return device.update(0, tostring(power) .. ';' .. tostring(energy))
		end
	end

}