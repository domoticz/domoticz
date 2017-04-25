local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Lux device adapter',

	matches = function (device)
		return (device.deviceType == 'Lux' and device.deviceSubType == 'Lux')
	end,

	process = function (device)

		-- first do the generic stuff
		local generic = adapters.genericAdapter.process(device)

		-- set the lux value
		device.addAttribute('lux', tonumber(device.rawData[1]))

		return device

	end

}