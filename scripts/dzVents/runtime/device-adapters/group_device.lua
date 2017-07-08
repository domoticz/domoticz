local TimedCommand = require('TimedCommand')

return {

	baseType = 'group',

	name = 'Group device adapter',

	matches = function (device, adapterManager)
		local res = (device.baseType == 'group')
		if (not res) then
			adapterManager.addDummyMethod(device, 'switchOn')
			adapterManager.addDummyMethod(device, 'switchOff')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.toggleGroup()
			local current, inv
			if (device.state ~= nil) then
				current = adapterManager.states[string.lower(device.state)]
				if (current ~= nil) then
					inv = current.inv
					if (inv ~= nil) then
						return domoticz.switchGroup(device.name, inv)
					end
				end
			end
			return nil
		end

		function device.setState(newState)
			-- generic state update method
			return domoticz.switchGroup(device.name, newState)
		end

		function device.switchOn()
			return domoticz.switchGroup(device.name, 'On')
		end

		function device.switchOff()
			return domoticz.switchGroup(device.name, 'Off')
		end

	end

}