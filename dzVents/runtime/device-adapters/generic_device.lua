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
		level = string.match(state, "%d+") -- extract dimming value
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
		else
			device['state'] = state
		end
	end

	return device
end

return {

	baseType = 'device',

	match = function (device)
		return true -- generic always matches
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
			utils.log('Discarding device. No last update info found: ' .. domoticz.utils._.str(data), level)
			return nil
		end

		device.isDevice = false
		device.isScene = false
		device.isGroup = false
		device.isTimer = false
		device.isVariable = false
		device.isHTTPResponse = false
		device.isSecurity = false


		if (data.baseType == 'device') then

			local bat
			local sig

			if (data.batteryLevel <= 100) then bat = data.batteryLevel end
			if (data.signalLevel <= 100) then sig = data.signalLevel end

			device['changed'] = data.changed
			device['description'] = data.description
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

			device['lastUpdate'] = Time(data.lastUpdate)
			device['rawData'] = data.rawData
			device['nValue'] = data.data._nValue

			device['cancelQueuedCommands'] = function()
				domoticz.sendCommand('Cancel', {
					type = 'device',
					idx = data.id
				})
			end

			device.isDevice = true
		end

		if (data.baseType == 'group' or data.baseType == 'scene') then
			device['description'] = data.description
			device['lastUpdate'] = Time(data.lastUpdate)
			device['rawData'] = { [1] = data.data._state }
			device['changed'] = data.changed
			device['cancelQueuedCommands'] = function()
				domoticz.sendCommand('Cancel', {
					type = 'scene',
					idx = data.id
				})
			end

		end

		setStateAttribute(data.data._state, device, _states)

		function device.setState(newState)
			-- generic state update method
			return TimedCommand(domoticz, device.name, newState, 'device', device.state)
		end

		for attribute, value in pairs(data.data) do
			if (device[attribute] == nil) then
				device[attribute] = value
			end
		end

		return device

	end

}
