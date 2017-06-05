local TimedCommand = require('TimedCommand')

return {

	baseType = 'device',

	name = 'Switch device adapter',

	matches = function (device)
		return (device.deviceType == 'Security' and device.subType == 'Security Panel')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.disArm()

			domoticz.sendCommand(device.name, 'Arm Away')

		end



	end

}