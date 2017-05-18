local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Thermostat setpoint device adapter',

	matches = function (device)
		return (device.deviceSubType == 'Text')
	end,

	process = function (device, data, domoticz)

		-- first do the generic stuff
		local generic = adapters.genericAdapter.process(device)

		generic.addAttribute('text', generic.rawData[1] or '')

		return generic

	end

}