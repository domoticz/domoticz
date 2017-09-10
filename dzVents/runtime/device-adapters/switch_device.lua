local TimedCommand = require('TimedCommand')

return {

	baseType = 'device',

	name = 'Switch device adapter',

	matches = function (device, adapterManager)
		local res = (
			device.deviceType == 'Light/Switch' or
			device.deviceType == 'Lighting 1' or
			device.deviceType == 'Lighting 2' or
			device.deviceType == 'Lighting 3' or
			device.deviceType == 'Lighting 4' or
			device.deviceType == 'Lighting 5' or
			device.deviceType == 'Lighting 6' or
			device.deviceType == 'Lighting Limitless/Applamp' or
			device.deviceType == 'Fan' or
			device.deviceType == 'Curtain' or
			device.deviceType == 'Blinds' or
			device.deviceType == 'RFY' or
			device.deviceType == 'Chime' or
			device.deviceType == 'Thermostat 2' or
			device.deviceType == 'Thermostat 3' or
			device.deviceType == 'Thermostat 4' or
			device.deviceType == 'Remote & IR' or
			device.deviceType == 'Home Confort' or -- typo should be there
			(device.deviceType == 'Radiator 1' and device.deviceSubType == 'Smartwares Mode') or
			(device.deviceType == 'Value' and device.deviceSubType == 'Rego 6XX')
		)
		if (not res) then
			adapterManager.addDummyMethod(device, 'switchOn')
			adapterManager.addDummyMethod(device, 'switchOff')
			adapterManager.addDummyMethod(device, 'close')
			adapterManager.addDummyMethod(device, 'open')
			adapterManager.addDummyMethod(device, 'stop')
			adapterManager.addDummyMethod(device, 'dimTo')
			adapterManager.addDummyMethod(device, 'switchSelector')
			adapterManager.addDummyMethod(device, 'toggleSwitch')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: levelName, levelOffHidden, levelActions, maxDimLevel

		if (data.lastLevel ~= nil) then
			-- dimmers that are switched off have a last level
			device.lastLevel = data.lastLevel
		end

		if (device.level == nil) then
			if (device.rawData[1] ~= nil) then
				device.level = tonumber(device.rawData[1]) -- for rgb(w)
			else
				device.level = data.data.levelVal
			end
		end


		function device.toggleSwitch()
			local current, inv
			if (device.state ~= nil) then
				current = adapterManager.states[string.lower(device.state)]
				if (current ~= nil) then
					inv = current.inv
					if (inv ~= nil) then
						return TimedCommand(domoticz, device.name, inv, 'device')
					end
				end
			end
			return nil
		end

		function device.switchOn()
			return TimedCommand(domoticz, device.name, 'On', 'device', device.state)
		end

		function device.switchOff()
			return TimedCommand(domoticz, device.name, 'Off', 'device', device.state)
		end

		function device.close()
			return TimedCommand(domoticz, device.name, 'Off', 'device', device.state)
		end

		function device.open()
			return TimedCommand(domoticz, device.name, 'On', 'device', device.state)
		end

		function device.stop() -- blinds
			return TimedCommand(domoticz, device.name, 'Stop', 'device', device.state)
		end

		function device.dimTo(percentage)
			return TimedCommand(domoticz, device.name, 'Set Level ' .. tostring(percentage), 'device')
		end

		function device.switchSelector(level)
			return TimedCommand(domoticz, device.name, 'Set Level ' .. tostring(level), 'device')
		end

		if (device.switchType == 'Selector') then
			device.levelNames = device.levelNames and string.split(device.levelNames, '|') or {}
			device.level = tonumber(device.rawData[1])
			device.levelName = device.state

		end

	end

}