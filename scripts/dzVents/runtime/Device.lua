local TimedCommand = require('TimedCommand')
local utils = require('Utils')

local function Device(domoticz, name, state, wasChanged)

	local changedAttributes = {} -- storage for changed attributes

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

	local self = {
		['name'] = name,
		['changed'] = wasChanged,
	}

	if (_G.TESTMODE) then
		function self._getUtilsInstance()
			return utils
		end
	end

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

	function self._setStateAttribute(state)
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
	self._setStateAttribute(state)

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
	function self.updateTemperature(temperature)
		self.update(0, temperature)
	end

	function self.updateHumidity(humidity, status)
		--[[
			status can be
			 domoticz.HUM_NORMAL
			domoticz.HUM_COMFORTABLE
			domoticz.HUM_DRY
			domoticz.HUM_WET
		 ]]
		self.update(humidity, status)
	end

	function self.updateBarometer(pressure, forecast)
		--[[
			forecast:
			 domoticz.BARO_STABLE
			domoticz.BARO_SUNNY
			domoticz.BARO_CLOUDY
			domoticz.BARO_UNSTABLE
			domoticz.BARO_THUNDERSTORM
			domoticz.BARO_UNKNOWN
			domoticz.BARO_CLOUDY_RAIN
		 ]]
		self.update(0, tostring(pressure) .. ';' .. tostring(forecast))
	end

	function self.updateTempHum(temperature, humidity, status)
		local value = tostring(temperature) .. ';' .. tostring(humidity) .. ';' .. tostring(status)
		self.update(0, value)
	end

	function self.updateTempHumBaro(temperature, humidity, status, pressure, forecast)
		local value = tostring(temperature) .. ';' ..
				tostring(humidity) .. ';' ..
				tostring(status) .. ';' ..
				tostring(pressure) .. ';' ..
				tostring(forecast)
		self.update(0, value)
	end

	function self.updateRain(rate, counter)
		self.update(0, tostring(rate) .. ';' .. tostring(counter))
	end

	function self.updateWind(bearing, direction, speed, gust, temperature, chill)
		local value = tostring(bearing) .. ';' ..
				tostring(direction) .. ';' ..
				tostring(speed) .. ';' ..
				tostring(gust) .. ';' ..
				tostring(temperature) .. ';' ..
				tostring(chill)
		self.update(0, value)
	end

	function self.updateUV(uv)
		local value = tostring(uv) .. ';0'
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

	function self.updateAirQuality(quality)
		self.update(quality)
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

	function self.updateLux(lux)
		self.update(0, lux)
	end

	function self.updateVoltage(voltage)
		self.update(0, voltage)
	end

	function self.updateText(text)
		self.update(0, text)
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


	function self.updateSetPoint(setPoint, mode, untilDate)
		if ((self.hardwareTypeVal == 15 or self.hardwareTypeVal == 20) and self.deviceSubType == 'SetPoint') then
			-- send the command using openURL otherwise, due to a bug in Domoticz, you will get a timeout on the script
			local url = 'http://' .. domoticz.settings['Domoticz ip'] .. ':' .. domoticz.settings['Domoticz port'] ..
					'/json.htm?type=command&param=udevice&idx=' .. self.id .. '&nvalue=0&svalue=' .. setPoint
			utils.log('Setting setpoint using openURL ' .. url, utils.LOG_DEBUG)
			domoticz.openURL(url)

		elseif (self.hardwareTypeVal == 39 and self.deviceSubType == 'Zone') then --evohome
			local url = 'http://' .. domoticz.settings['Domoticz ip'] .. ':' .. domoticz.settings['Domoticz port'] ..
					'/json.htm?type=setused&idx=' .. self.id .. '&setpoint=' .. setPoint .. '&mode=' .. tostring(mode) .. '&used=true'

			if (untilDate) then
				url = url .. '&until=' .. tostring(untilDate)
			end

			utils.log('Setting setpoint using openURL ' .. url, utils.LOG_DEBUG)
			domoticz.openURL(url)
		else
			utils.log('Setting setpoint not supported for this device.', utils.LOG_ERROR)
		end
	end

	function self.kodiSwitchOff()
		--return TimedCommand(domoticz, self.name, 'Play')
		domoticz.sendCommand(self.name, 'Off')
	end

	function self.kodiStop()
		--return TimedCommand(domoticz, self.name, 'Stop')
		domoticz.sendCommand(self.name, 'Stop')
	end

	function self.kodiPlay()
		--return TimedCommand(domoticz, self.name, 'Play')
		domoticz.sendCommand(self.name, 'Play')
	end

	function self.kodiPause()
		--return TimedCommand(domoticz, self.name, 'Pause')
		domoticz.sendCommand(self.name, 'Pause')
	end

	function self.kodiSetVolume(value)

		if (value<0 or value > 100) then

			utils.log('Volume must be between 0 and 100. Value = ' .. tostring(value) , utils.LOG_ERROR)

		else
			--return TimedCommand(domoticz, self.name, 'Pause')
			domoticz.sendCommand(self.name, 'Set Volume ' .. tostring(value))
		end

	end

	function self.kodiPlayPlaylist(name, position)
		if (position == nil) then
			position = 0
		end
		domoticz.sendCommand(self.name, 'Play Playlist ' .. tostring(name) .. ' ' .. tostring(position))
	end

	function self.kodiPlayFavorites(position)
		if (position == nil) then
			position = 0
		end
		domoticz.sendCommand(self.name, 'Play Favorites ' .. tostring(position))
	end

	function self.kodiExecuteAddOn(addonId)
		domoticz.sendCommand(self.name, 'Execute ' .. tostring(addonId))
	end


	function self.attributeChanged(attribute)
		-- returns true if an attribute is marked as changed
		return (changedAttributes[attribute] == true)
	end

	function self.setAttributeChanged(attribute)
		-- mark an attribute as being changed
		changedAttributes[attribute] = true
	end

	function self.addAttribute(attribute, value)
		-- add attribute to this device
		self[attribute] = value
	end

	return self
end

return Device