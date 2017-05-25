local TimedCommand = require('TimedCommand')
local utils = require('Utils')
local Time = require('Time')
local Adapters = require('Adapters')

local function Device(domoticz, data)

	local changedAttributes = {} -- storage for changed attributes

	local self = {}

	local _states = {
		on = { b = true, inv = 'Off' },
		open = { b = true, inv = 'Closed' },
		['group on'] = { b = true },
		panic = { b = true, inv = 'Off' },
		normal = { b = true, inv = 'Alarm' },
		alarm = { b = true, inv = 'Normal' },
		chime = { b = true },
		video = { b = true },
		audio = { b = true },
		photo = { b = true },
		playing = { b = true, inv = 'Pause' },
		motion = { b = true },
		off = { b = false, inv = 'On' },
		closed = { b = false, inv = 'Open' },
		['group off'] = { b = false },
		['panic end'] = { b = false },
		['no motion'] = { b = false, inv = 'Off' },
		stop = { b = false, inv = 'Open' },
		stopped = { b = false },
		paused = { b = false, inv = 'Play' },
		['all on'] = { b = true, inv = 'All Off' },
		['all off'] = { b = false, inv = 'All On' },
	}

	-- some states will be 'booleanized'
	local function stateToBool(state)
		state = string.lower(state)
		local info = _states[state]
		local b
		if (info) then
			b = _states[state]['b']
		end

		if (b == nil) then b = false end
		return b
	end

	function setStateAttribute(state)
		local level;
		if (state and string.find(state, 'Set Level')) then
			level = string.match(state, "%d+") -- extract dimming value
			state = 'On' -- consider the device to be on
		end

		if (level) then
			self['level'] = tonumber(level)
		end


		if (state ~= nil) then -- not all devices have a state like sensors
			if (type(state) == 'string') then -- just to be sure
				self['state'] = state
				self['bState'] = stateToBool(self['state'])
			else
				self['state'] = state
			end
		end
	end

	-- extract dimming levels for dimming devices
	local level

	function self.toggleSwitch()
		local current, inv
		if (self.state ~= nil) then
			current = _states[string.lower(self.state)]
			if (current ~= nil) then
				inv = current.inv
				if (inv ~= nil) then
					return TimedCommand(domoticz, self.name, inv)
				end
			end
		end
		return nil
	end

	function self.setState(newState)
		-- generic state update method
		return TimedCommand(domoticz, self.name, newState)
	end

	function self.switchOn()
		return TimedCommand(domoticz, self.name, 'On')
	end

	function self.switchOff()
		return TimedCommand(domoticz, self.name, 'Off')
	end

	function self.close()
		return TimedCommand(domoticz, self.name, 'Closed')
	end

	function self.open()
		return TimedCommand(domoticz, self.name, 'Open')
	end

	function self.stop() -- blinds
		return TimedCommand(domoticz, self.name, 'Stop')
	end

	function self.dimTo(percentage)
		return TimedCommand(domoticz, self.name, 'Set Level ' .. tostring(percentage))
	end

	function self.switchSelector(level)
		return TimedCommand(domoticz, self.name, 'Set Level ' .. tostring(level))
	end

	function self.update(...)
		-- generic update method for non-switching devices
		-- each part of the update data can be passed as a separate argument e.g.
		-- device.update(12,34,54) will result in a command like
		-- ['UpdateDevice'] = '<id>|12|34|54'
		local command = self.id
		for i, v in ipairs({ ... }) do
			command = command .. '|' .. tostring(v)
		end

		domoticz.sendCommand('UpdateDevice', command)
	end

	-- update specials
	-- see http://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s

	function self.updateTempHumBaro(temperature, humidity, status, pressure, forecast)
		local value = tostring(temperature) .. ';' ..
				tostring(humidity) .. ';' ..
				tostring(status) .. ';' ..
				tostring(pressure) .. ';' ..
				tostring(forecast)
		self.update(0, value)
	end

	function self.updateCounter(value)
		self.update(0, value)
	end

	function self.updateElectricity(power, energy)
		self.update(0, tostring(power) .. ';' .. tostring(energy))
	end

	function self.updateP1(usage1, usage2, return1, return2, cons, prod)
		--[[
			USAGE1= energy usage meter tariff 1
			USAGE2= energy usage meter tariff 2
			RETURN1= energy return meter tariff 1
			RETURN2= energy return meter tariff 2
			CONS= actual usage power (Watt)
			PROD= actual return power (Watt)
			USAGE and RETURN are counters (they should only count up).
			For USAGE and RETURN supply the data in total Wh with no decimal point.
			(So if your meter displays f.i. USAGE1= 523,66 KWh you need to send 523660)
		 ]]
		local value = tostring(usage1) .. ';' ..
				tostring(usage2) .. ';' ..
				tostring(return1) .. ';' ..
				tostring(return2) .. ';' ..
				tostring(cons) .. ';' ..
				tostring(prod)
		self.update(0, value)
	end

	function self.updatePressure(pressure)
		self.update(0, pressure)
	end

	function self.updatePercentage(percentage)
		self.update(0, percentage)
	end

	function self.updateGas(usage)
		--[[
			USAGE= Gas usage in liter (1000 liter = 1 m³)
			So if your gas meter shows f.i. 145,332 m³ you should send 145332.
			The USAGE is the total usage in liters from start, not f.i. the daily usage.
		 ]]
		self.update(0, usage)
	end

	function self.updateVoltage(voltage)
		self.update(0, voltage)
	end

	function self.updateAlertSensor(level, text)
		--[[ level can be
			 domoticz.ALERTLEVEL_GREY
			 domoticz.ALERTLEVEL_GREEN
			domoticz.ALERTLEVEL_YELLOW
			domoticz.ALERTLEVEL_ORANGE
			domoticz.ALERTLEVEL_RED
		]]
		self.update(level, text)
	end

	function self.updateDistance(distance)
		--[[
		 distance in cm or inches, can be in decimals. For example 12.6
		 ]]
		self.update(0, distance)
	end

	function self.updateCustomSensor(value)
		self.update(0, value)
	end

	function self.attributeChanged(attribute)
		-- returns true if an attribute is marked as changed
		return (changedAttributes[attribute] == true)
	end

	function self.addAttribute(attribute, value)
		-- add attribute to this device
		self[attribute] = value
	end

	local state

	self['name'] = data.name
	self['id'] = data.id
	self['_data'] = data

	local adapters = Adapters()

	if (data.baseType == 'device') then


		--state = data.data._state
		self['changed'] = data.changed
		self['description'] = data.description
		self['description'] = data.description
		self['deviceType'] = data.deviceType
		self['hardwareName'] = data.hardwareName
		self['hardwareType'] = data.hardwareType
		self['hardwareId'] = data.hardwareID
		self['hardwareTypeVal'] = data.hardwareTypeValue
		self['switchType'] = data.switchType
		self['switchTypeValue'] = data.switchTypeValue
		self['timedOut'] = data.timedOut
		self['batteryLevel'] = data.batteryLevel
		self['signalLevel'] = data.signalLevel
		self['deviceSubType'] = data.subType
		self['lastUpdate'] = Time(data.lastUpdate)
		self['rawData'] = data.rawData

		-- process generic first
		adapters.genericAdapter.process(self)

		-- get a specific adapter for the current device
		local adapters = adapters.getDeviceAdapters(self)
		for i, adapter in pairs(adapters) do
			adapter.process(self, data, domoticz, utils)
		end

	elseif (data.baseType == 'group' or data.baseType == 'scene') then
		state = data.data._state
		self['lastUpdate'] = Time(data.lastUpdate)
		self['rawData'] = { [1]=data.state }
		setStateAttribute(state)
	end

	--setStateAttribute(state)

	-- tbd
	if (data.changedAttribute) then
		changedAttributes[data.changedAttribute] = true
	end

	if (_G.TESTMODE) then
		function self._getUtilsInstance()
			return utils
		end
	end




	return self
end

return Device