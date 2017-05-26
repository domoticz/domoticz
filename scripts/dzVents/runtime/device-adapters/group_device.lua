local TimedCommand = require('TimedCommand')

return {

	baseType = 'group',

	name = 'Group device adapter',

	matches = function (device)
		return (device.baseType == 'group')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.toggleGroup()
			local current, inv
			if (device.state ~= nil) then
				current = adapterManager.states[string.lower(device.state)]
				if (current ~= nil) then
					inv = current.inv
					if (inv ~= nil) then
						return TimedCommand(domoticz, device.name, inv)
					end
				end
			end
			return nil
		end

		function device.setState(newState)
			-- generic state update method
			return TimedCommand(domoticz, device.name, newState)
		end

		function device.switchOn()
			return TimedCommand(domoticz, device.name, 'On')
		end

		function device.switchOff()
			return TimedCommand(domoticz, device.name, 'Off')
		end

	end

}