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
			return TimedCommand(domoticz, 'Scene:' .. device.name, 'On', 'device', device.state)
			--return domoticz.setScene(device.name, 'On')
		end

		function device.switchOff()
			return TimedCommand(domoticz, 'Scene:' .. device.name, 'Off', 'device', device.state)
		end
	end

}