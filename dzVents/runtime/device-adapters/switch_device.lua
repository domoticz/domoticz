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
			device.deviceType == 'Color Switch' or
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
			adapterManager.addDummyMethod(device, 'setLevel')
			adapterManager.addDummyMethod(device, 'switchSelector')
			adapterManager.addDummyMethod(device, 'toggleSwitch')
			adapterManager.addDummyMethod(device, 'quietOn')
			adapterManager.addDummyMethod(device, 'quietOff')
		else
			blindsOff2Close = { "Venetian Blinds US", "Venetian Blinds EU", "Blinds Percentage", "Blinds" }
		end

		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)
		-- from data: levelName, levelOffHidden, levelActions, maxDimLevel

		if (data.lastLevel ~= nil) then
			-- dimmers that are switched off have a last level
			device.lastLevel = data.lastLevel
		end

		function device.quietOn()
			local url = domoticz.settings['Domoticz url'] .. '/json.htm?type=command&param=udevice&nvalue=1&svalue=1&idx=' .. device.id
			return domoticz.openURL(url)
		end

		function device.quietOff()
			local url = domoticz.settings['Domoticz url'] .. '/json.htm?type=command&param=udevice&nvalue=0&svalue=0&idx=' .. device.id
			return domoticz.openURL(url)
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
			return TimedCommand(domoticz, device.name, ( utils.inTable(blindsOff2Close, device.switchType ) and 'On') or 'Off', 'device', device.state)
		end

		function device.open()
			return TimedCommand(domoticz, device.name, ( utils.inTable(blindsOff2Close, device.switchType ) and 'Off') or 'On', 'device', device.state)
		end

		function device.stop() -- blinds
			return TimedCommand(domoticz, device.name, 'Stop', 'device', device.state)
		end

		function device.dimTo(percentage)
			return TimedCommand(domoticz, device.name, 'Set Level ' .. tostring(percentage), 'device')
		end

		function device.setLevel(percentage)
			return TimedCommand(domoticz, device.name, 'Set Level ' .. tostring(percentage), 'device')
		end

		function device.switchSelector(level)

			local function guardLevel(val)
				local maxLevel = #device.levelNames * 10 - 10
				val = ( val % 10 ~= 0 ) and ( utils.round(val / 10) * 10 ) or val
				return math.floor(( math.min(math.max(val,0),maxLevel) ))
			end

			local sLevel
			if type(level) == 'string' then
				sLevel = level
				local levels = {}
				for i=1, #device.levelNames do
					levels[device.levelNames[(i)]] = i * 10 - 10
				end
				level = levels[level]
			end

			if device.switchType ~= "Selector" then
				utils.log('method switchSelector not available for type ' .. device.switchType, domoticz.LOG_ERROR )
			elseif not level and sLevel then
				utils.log('levelname ' .. sLevel .. ' does not exist', domoticz.LOG_ERROR )
			elseif not level then
				utils.log('level cannot be nil', domoticz.LOG_ERROR )
			else
				return TimedCommand(domoticz, device.name, 'Set Level ' .. guardLevel(level), 'device', level, device.level)
			end

		end

		if (device.switchType == 'Selector') then
			device.levelNames = device.levelNames and utils.stringSplit(device.levelNames, '|') or {}
			device.level = tonumber(device.rawData[1])
			device.levelName = device.state
		end

	end
}
