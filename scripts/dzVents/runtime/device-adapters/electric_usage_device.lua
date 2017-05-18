local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Electric usage device adapter',

	matches = function (device)
		return (device.deviceType == 'Usage' and device.deviceSubType == 'Electric')
	end,

	process = function (device, data, domoticz)

		-- first do the generic stuff
		local generic = adapters.genericAdapter.process(device)

		generic.addAttribute('WhActual', generic.rawData[1]  or 0)

		return generic

	end

}