local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'kWh device adapter',

	matches = function (device)
		return false
		--return (device.deviceType == 'General' and device.deviceSubType == 'kWh')
	end,

	process = function (device)

		-- first do the generic stuff
		local generic = adapters.genericAdapter.process(device)

		device.addAttribute('WhTotal', tonumber(device.rawData[2]))
		device.addAttribute('WActual', tonumber(device.rawData[1]))

		local todayFormatted = device.CounterToday or ''
		-- risky business, we assume no decimals, just thousands separators
		-- there is no raw value available for today
		local s = string.gsub(todayFormatted, '%,', '')
		--s = string.gsub(s, '%,', '')
		s = string.gsub(s, ' kWh', '')
		device.addAttribute('WhToday', tonumber(s))

		return device

	end

}