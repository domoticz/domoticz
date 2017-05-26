local TimedCommand = require('TimedCommand')

return {

	baseType = 'scene',

	name = 'Scene device adapter',

	matches = function (device)
		return (device.baseType == 'scene')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.setState(newState)
			-- generic state update method
			return TimedCommand(domoticz, device.name, newState)
		end

		function device.switchOn()
			return TimedCommand(domoticz, device.name, 'On')
		end


	end

}