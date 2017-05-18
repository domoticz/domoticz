local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Zone heating device adapter',

	matches = function (device)
		return (device.deviceType == 'Heating' and device.deviceSubType == 'Zone')
	end,

	process = function (device)

		-- first do the generic stuff
		local generic = adapters.genericAdapter.process(device)

		generic.addAttribute('setPoint', tonumber(device.rawData[2]))
		generic.addAttribute('heatingMode', generic.rawData[3])

		return generic

	end

}