local Time = require('Time')
local TimedCommand = require('TimedCommand')

-- some states will be 'booleanized'
local function stateToBool(state, _states)
	state = string.lower(state)
	local info = _states[state]
	local b
	if (info) then
		b = _states[state]['b']
	end

	if (b == nil) then b = false end
	return b
end

local function setStateAttribute(state, device, _states)
	local level;
	if (state and string.find(state, 'Set Level')) then
		level = string.match(state, '%d+') -- extract dimming value
		state = 'On' -- consider the device to be on
	end

	if (level) then
		device['level'] = tonumber(level)
	end

	if (state ~= nil) then -- not all devices have a state like sensors
		if (type(state) == 'string') then -- just to be sure
			device['state'] = state
			device['bState'] = stateToBool(state, _states)
			device['active'] = device['bState']
			if device.active then device.inActive = false
			elseif device.active == false then device.inActive = true
			end
		else
			device['state'] = state
		end
	end

	return device
end

return {

	baseType = 'device',

	match = function (device)
		return true -- generic always matches?
	end,

	matches = function (device, adapterManager)
		adapterManager.addDummyMethod(device, 'protectionOn')
		adapterManager.addDummyMethod(device, 'protectionOff')
		adapterManager.addDummyMethod(device, 'setDescription')
		adapterManager.addDummyMethod(device, 'setIcon')
		adapterManager.addDummyMethod(device, 'setValues')
		adapterManager.addDummyMethod(device, 'rename')
		adapterManager.addDummyMethod(device, 'updateQuiet')
	end,

	process = function (device, data, domoticz, utils, adapterManager)
		local _states = adapterManager.states

		if (data.lastUpdate == '' or data.lastUpdate == nil) then
			local level
			if (data.name ~= nil and data.name ~= '') then
				level = utils.LOG_ERROR
			else
				level = utils.LOG_DEBUG
			end
			if data.baseType ~= 'camera' and data.baseType ~= 'hardware' then
				utils.log('Discarding device. No last update info found: ' .. domoticz.utils._.str(data), level)
			end
			return nil
		end

		device.isHardware = false
		device.isDevice = false
		device.isScene = false
		device.isGroup = false
		device.isTimer = false
		device.isVariable = false
		device.isHTTPResponse = false
		device.isSecurity = false

		-- All baseTypes
		device['changed'] = data.changed
		device['protected'] = data.protected
		device['description'] = data.description
		device['lastUpdate'] = Time(data.lastUpdate)

		if (data.baseType == 'device') then

			local bat
			local sig

			if (data.batteryLevel <= 100) then bat = data.batteryLevel end
			if (data.signalLevel <= 100) then sig = data.signalLevel end

			device['deviceType'] = data.deviceType
			device['hardwareName'] = data.data.hardwareName
			device['hardwareType'] = data.data.hardwareType
			device['hardwareId'] = data.data.hardwareID
			device['deviceId'] = data.deviceID
			device['unit'] = data.data.unit
			device['hardwareTypeValue'] = data.data.hardwareTypeValue
			device['switchType'] = data.switchType
			device['switchTypeValue'] = data.switchTypeValue
			device['timedOut'] = data.timedOut
			device['batteryLevel'] = bat
			device['signalLevel'] = sig
			device['deviceSubType'] = data.subType
			device['rawData'] = data.rawData
			device['nValue'] = data.data._nValue
			device['sValue'] = data.data._state or ( table.concat(device.rawData,';') ~= '' and  table.concat(device.rawData,';') ) or nil
			device['cancelQueuedCommands'] = function()
				domoticz.sendCommand('Cancel', {
					type = 'device',
					idx = data.id
				})
			end
			if customImage and tonumber(customImage) and customImage >= 100 then
				device.icon = device.Image
			end

			device.isDevice = true
		end

		if (data.baseType == 'group' or data.baseType == 'scene') then
			device['rawData'] = { [1] = data.data._state }
			device['cancelQueuedCommands'] = function()
				domoticz.sendCommand('Cancel', {
					type = 'scene',
					idx = data.id
				})
			end
		end

		setStateAttribute(data.data._state, device, _states)

		function device.setDescription(description)
			local url = domoticz.settings['Domoticz url'] ..
				'/json.htm?description=' .. utils.urlEncode(description) ..
				'&idx=' .. device.id ..
				'&name='.. utils.urlEncode(device.name) ..
				'&type=setused&used=true'
			return domoticz.openURL(url)
		end

		function device.setIcon(iconNumber)
			local url = domoticz.settings['Domoticz url'] ..
				'/json.htm?type=setused&used=true&name=' ..
				 utils.urlEncode(device.name) ..
				'&description=' .. utils.urlEncode(device.description) ..
				'&idx=' .. device.id ..
				'&switchtype=' .. device.switchTypeValue ..
				'&customimage=' .. iconNumber
			return domoticz.openURL(url)
		end

		function device.rename(newName)
			local url = domoticz.settings['Domoticz url'] ..
						'/json.htm?type=command&param=renamedevice' ..
						'&idx=' .. device.idx ..
						'&name=' .. utils.urlEncode(newName)
			return domoticz.openURL(url)
		end

		function device.protectionOn()
			local url = domoticz.settings['Domoticz url'] ..
						'/json.htm?type=setused&used=true&protected=true' ..
						'&idx=' .. device.idx
			return domoticz.openURL(url)
		end

		function device.protectionOff()
			local url = domoticz.settings['Domoticz url'] ..
						'/json.htm?type=setused&used=true&protected=false' ..
						'&idx=' .. device.idx
			return domoticz.openURL(url)
		end

		function device.setValues(nValue, ...)
			local args = {...}
			local sValue = ''
			local parsetrigger = false
			for _, value in ipairs(args) do
				if type(value) == 'string' and value:lower() == 'parsetrigger' then parsetrigger = true
				else sValue	= sValue .. tostring(value) .. ';'
				end
			end
			if #sValue > 1 then
				sValue = sValue:sub(1,-2)
			end
			local url = domoticz.settings['Domoticz url'] ..
				'/json.htm?type=command&param=udevice&idx=' .. device.id ..
				'&nvalue=' .. (nValue or device.nValue) ..
				'&svalue=' .. sValue ..
				'&parsetrigger=' .. tostring(parsetrigger)
			return domoticz.openURL(url)
		end

		function device.setState(newState)
			-- generic state update method
			return TimedCommand(domoticz, device.name, newState, 'device', device.state)
		end

		function device.updateQuiet(nValue, sValue)

			if not(nValue or sValue) then
				utils.log('nValue and sValue cannot both be nil', utils.LOG_ERROR )
				return
			end

			local nValue = nValue
			local sValue = sValue

			if sValue then
				sValue = '&svalue=' .. utils.urlEncode(tostring(sValue))
			elseif nValue and tonumber(nValue) == nil and sValue == nil then
				sValue = '&svalue=' .. utils.urlEncode(tostring(nValue))
				nValue = ''
			end

			if nValue and tonumber(nValue) ~= nil then
				nValue = '&nvalue=' .. math.floor(nValue)
			end

			sValue = sValue or ''
			nValue = nValue or ''

			local url = domoticz.settings['Domoticz url'] ..
				'/json.htm?type=command&param=udevice&idx=' .. device.id .. sValue .. nValue

			return domoticz.openURL(url)
		end

		for attribute, value in pairs(data.data) do
			if (device[attribute] == nil) then
				if type(value) == 'number' and math.floor(value) == value then
					device[attribute] = math.floor(value)
				else
					device[attribute] = value
				end
			end
		end

		return device

	end

}
