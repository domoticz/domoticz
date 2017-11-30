local scriptPath = _G.globalvariables['script_path']
package.path    = package.path .. ';' .. scriptPath .. '?.lua'
local TIMED_OPTIONS = require('TimedCommandOptions')

local utils = require('Utils')


local function TimedCommand(domoticz, commandName, value, mode, currentState)

	if (type(mode) == 'string') then
		mode = TIMED_OPTIONS[mode]
	end

	math.randomseed(os.time())

	-- mode can be 'device', 'updatedevice' or 'variable'

	local valueValue = value
	local afterValue, forValue, randomValue, silentValue, repeatValue, repeatIntervalValue, checkValue
	local _for, _after, _within, _rpt, _silent, _repeatAfter

	local constructCommand = function()

		local command = {} -- array of command parts

		if (checkValue == true and currentState == valueValue) then
			-- do nothing
			return nil
		end

		table.insert(command, valueValue)

		if (randomValue ~= nil) then
			table.insert(command, 'RANDOM ' .. tostring(randomValue) .. ' SECONDS')
		end

		if (afterValue ~= nil) then
			table.insert(command, 'AFTER ' .. tostring(afterValue) .. ' SECONDS')
		end

		if (forValue ~= nil) then
			table.insert(command, 'FOR ' .. tostring(forValue) .. ' SECONDS')
		end

		if (mode._triggerMode == 'TRIGGER') then
			if (not silentValue == true) then
				table.insert(command, 'TRIGGER')
			end
		else
			if (silentValue == true) then
				table.insert(command, 'NOTRIGGER')
			end
		end

		if (repeatValue ~= nil) then
			table.insert(command, 'REPEAT ' .. tostring(repeatValue))
		end

		if (repeatIntervalValue ~= nil) then
			table.insert(command, 'INTERVAL ' .. tostring(repeatIntervalValue) .. ' SECONDS')
		end

		local sCommand = table.concat(command, " ")

		utils.log('Constructed timed-command: ' .. sCommand, utils.LOG_DEBUG)

		return sCommand
	end

	local constructCommandForTable = function()

		if (randomValue ~= nil) then
			-- value._after = tostring(math.random(randomValue))
			value._random = randomValue
		end

		if (afterValue ~= nil) then
			value._after = afterValue
		end

		if (forValue ~= nil) then
			value._for = forValue
		end

		if (silentValue == true) then
			value._trigger = nil
		-- else
		-- 	value._trigger = true
		end

		if (repeatValue ~= nil) then
			value._repeat = repeatValue
		end

		if (repeatIntervalValue ~= nil) then
			value._repeatInterval = repeatIntervalValue
		end

		return value
	end

	local latest, command, sValue

	-- get a reference to the latest entry in the commandArray so we can
	-- keep modifying it here.
	if (type(value) == 'table') then
		latest, command, sValue = domoticz.sendCommand(commandName, value)
	else
		latest, command, sValue = domoticz.sendCommand(commandName, constructCommand())
	end

	local function factory()

		local res = {}

		if (mode._for == true and forValue == nil) then
			res.forSec = _for(1)
			res.forMin = _for(60)
			res.forHour = _for(3600)
		end

		if (mode._repeat == true and repeatIntervalValue == nil) then
			res.repeatAfterSec = _repeatAfter(1)
			res.repeatAfterMin = _repeatAfter(60)
			res.repeatAfterHour = _repeatAfter(3600)
		end

		if (mode._checkState and checkValue == nil and currentState ~= nil) then
			res.checkFirst = _checkState
		end

		if (afterValue == nil and randomValue == nil) then
			if (mode._after == true) then
				res.afterSec = _after(1)
				res.afterMin = _after(60)
				res.afterHour = _after(3600)
			end

			if (mode._within == true) then
				res.withinSec = _within(1)
				res.withinMin = _within(60)
				res.withinHour = _within(3600)
			end
		end

		if (mode._silent == true and silentValue == nil) then
			res.silent = _silent
		end

		res._latest = latest

		return res
	end

	local function updateCommand()
		if (type(value) == 'table') then
			latest[command] = constructCommandForTable()
		else
			latest[command] = constructCommand()
		end
	end

	_checkValue = function(value, msg)
		if (value == nil) then utils.log(msg, utils.LOG_ERROR) end
	end

	_after = function(factor)
		return function(value)
			_checkValue(value, "No value given for 'afterXXX' command")
			afterValue = value * factor
			updateCommand()
			return factory()
		end
	end

	_within = function(factor)
		return function(value)
			_checkValue(value, "No value given for 'withinXXX' command")
			randomValue = value * factor

			updateCommand()

			return factory()
		end
	end

	_for = function(factor)
		return function(value)
			_checkValue(value, "No value given for 'forXXX' command")
			forValue = value * factor
			updateCommand()
			return factory()
		end
	end

	_silent = function()
		silentValue = true;
		updateCommand()
		return factory()
	end

	_repeatAfter = function(factor)
		_checkValue(value, "No value given for 'repeatAfterXXX' command")
		return function(value, amount)

			if (amount == nil) then
				amount = 1
			end

			repeatIntervalValue = value * factor
			repeatValue = amount + 1 -- add one due to a bug in domoticz

			updateCommand()
			return factory()
		end
	end

	_checkState = function()
		checkValue = true;
		updateCommand()
		return factory()
	end

	return factory()

end

return TimedCommand
