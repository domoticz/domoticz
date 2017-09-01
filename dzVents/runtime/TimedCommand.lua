local scriptPath = _G.globalvariables['script_path']
package.path    = package.path .. ';' .. scriptPath .. '?.lua'

local utils = require('Utils')

-- generic 'switch' class with timed options
-- supports chainging like:
-- switch(v1).for_min(v2).after_sec/min(v3)
-- switch(v1).within_min(v2).for_min(v3)
-- switch(v1).after_sec(v2).for_min(v3)

local function deprecationWarning(msg)
	utils.log(msg, utils.LOG_ERROR)
end

local function TimedCommand(domoticz, name, value, mode)
	local valueValue = value
	local afterValue, forValue, randomValue, silentValue

	local constructCommand = function()

		local command = {} -- array of command parts

		table.insert(command, valueValue)

		if (randomValue ~= nil) then
			table.insert(command, 'RANDOM ' .. tostring(randomValue))
		end

		if (afterValue ~= nil) then
			table.insert(command, 'AFTER ' .. tostring(afterValue))
		end

		if (forValue ~= nil) then
			table.insert(command, 'FOR ' .. tostring(forValue))
		end

		if (mode == 'updatedevice' or mode == 'variable') then
			if (silentValue == false or silentValue == nil) then
				table.insert(command, 'TRIGGER')
			end
		else
			if (silentValue == true) then
				table.insert(command, 'NOTRIGGER')
			end
		end

		local sCommand = table.concat(command, " ")

		utils.log('Constructed timed-command: ' .. sCommand, utils.LOG_DEBUG)

		return sCommand

	end

	-- get a reference to the latest entry in the commandArray so we can
	-- keep modifying it here.
	local latest, command, sValue = domoticz.sendCommand(name, constructCommand())
	local forMin, afterSec, afterMin, silent, withinMin


	forMin = function(minutes)
		forValue = minutes
		latest[command] = constructCommand()
		return {
			['afterSec'] = afterSec,
			['afterMin'] = afterMin,
			['silent'] = silent,
			['withinMin'] = withinMin
		}
	end

	afterSec = function(seconds)
		afterValue = seconds
		latest[command] = constructCommand()
		if (mode == 'variable') then
			return {
				['afterMin'] = afterMin,
				['silent'] = silent,
			}
		else
			return {
				['afterMin'] = afterMin,
				['forMin'] = forMin,
				['silent'] = silent,
				['withinMin'] = withinMin
			}
		end

	end

	afterMin = function(minutes)
		afterValue = minutes * 60
		latest[command] = constructCommand()
		if (mode == 'variable') then
			return {
				['afterSec'] = afterSec,
				['silent'] = silent,
			}
		else
			return {
				['afterSec'] = afterSec,
				['forMin'] = forMin,
				['silent'] = silent,
				['withinMin'] = withinMin
			}
		end

	end

	silent = function()
		silentValue = true;
		latest[command] = constructCommand()
		if (mode == 'variable') then
			return {
				['afterMin'] = afterMin,
				['afterSec'] = afterSec,
			}
		elseif (mode == 'updatedevice') then
			return {}
		else
			return {
				['afterMin'] = afterMin,
				['afterSec'] = afterSec,
				['forMin'] = forMin,
				['withinMin'] = withinMin
			}
		end
	end

	withinMin = function(minutes)
		randomValue = minutes
		latest[command] = constructCommand()
		return {
			['afterMin'] = afterMin,
			['afterSec'] = afterSec,
			['forMin'] = forMin,
			['silent'] = silent
		}
	end

	if (mode == 'variable') then
		return {
			['_constructCommand'] = constructCommand, -- for testing purposes
			['_latest'] = latest, -- for testing purposes
			['afterSec'] = afterSec,
			['afterMin'] = afterMin,
			['silent'] = silent
		}

	elseif (mode == 'updatedevice') then
		return {
			['_constructCommand'] = constructCommand, -- for testing purposes
			['_latest'] = latest, -- for testing purposes
			['silent'] = silent
		}
	else
		return {
			['_constructCommand'] = constructCommand, -- for testing purposes
			['_latest'] = latest, -- for testing purposes
			['afterSec'] = afterSec,
			['afterMin'] = afterMin,
			['forMin'] = forMin,
			['withinMin'] = withinMin,
			['silent'] = silent
		}
	end

end

return TimedCommand