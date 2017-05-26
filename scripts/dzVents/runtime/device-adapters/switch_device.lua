local TimedCommand = require('TimedCommand')

return {

	baseType = 'device',

	name = 'Switch device adapter',

	matches = function (device)
		return (device.deviceType == 'Light/Switch')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.toggleSwitch()
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

		function device.switchOn()
			return TimedCommand(domoticz, device.name, 'On')
		end

		function device.switchOff()
			return TimedCommand(domoticz, device.name, 'Off')
		end

		function device.close()
			return TimedCommand(domoticz, device.name, 'Off')
		end

		function device.open()
			return TimedCommand(domoticz, device.name, 'On')
		end

		function device.stop() -- blinds
			return TimedCommand(domoticz, device.name, 'Stop')
		end

		function device.dimTo(percentage)
			return TimedCommand(domoticz, device.name, 'Set Level ' .. tostring(percentage))
		end

		function device.switchSelector(level)
			return TimedCommand(domoticz, device.name, 'Set Level ' .. tostring(level))
		end

		if (device.switchType == 'Selector') then
			device.levelNames = device.levelName and string.split(device.levelName, '|') or {}
		end


	end

}