local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'P1 smart meter energy device adapter',

	matches = function (device)
		return (device.deviceType == 'P1 Smart Meter' and device.deviceSubType == 'Energy')
	end,

	process = function (device, data, domoticz)

		-- first do the generic stuff
		local generic = adapters.genericAdapter.process(device)

		generic.addAttribute('WhActual', generic.rawData[5] or 0)

		return generic

	end

}