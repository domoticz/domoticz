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
						return TimedCommand(domoticz, 'Group:' .. device.name, inv, 'device')
					end
				end
			end
			return nil
		end

		function device.setState(newState)
			-- generic state update method
			return TimedCommand(domoticz, 'Group:' .. device.name, newState, 'device', device.state)
		end

		function device.switchOn()
			return TimedCommand(domoticz, 'Group:' .. device.name, 'On', 'device', device.state)
		end

		function device.switchOff()
			return TimedCommand(domoticz, 'Group:' .. device.name, 'Off', 'device', device.state)
		end

	end

}