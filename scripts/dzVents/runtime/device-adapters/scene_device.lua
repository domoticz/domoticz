local TimedCommand = require('TimedCommand')

return {

	baseType = 'scene',

	name = 'Scene device adapter',

	matches = function (device, adapterManager)
		local res = (device.baseType == 'scene')
		if (not res) then
			adapterManager.addDummyMethod(device, 'switchOn')
			adapterManager.addDummyMethod(device, 'switchOff')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.setState(newState)
			-- generic state update method
			return domoticz.setScene(device.name, newState)
		end

		function device.switchOn()
			return domoticz.setScene(device.name, 'On')
		end

		function device.switchOff()
			return domoticz.setScene(device.name, 'Off')
		end
	end

}