local adapters = require('Adapters')()

return {

	baseType = 'device',

	name = 'Thermostat setpoint device adapter',

	matches = function (device)
		return (device.deviceType == 'Thermostat' and device.deviceSubType == 'SetPoint')
	end,

	process = function (device, data, domoticz)

		-- first do the generic stuff
		local generic = adapters.genericAdapter.process(device)

		generic.addAttribute('SetPoint', generic.rawData[1] or 0)

		return generic

	end

}