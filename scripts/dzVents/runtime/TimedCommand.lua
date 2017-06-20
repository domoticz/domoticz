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

local function TimedCommand(domoticz, name, value)
	local valueValue = value
	local afterValue, forValue, randomValue

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

		local sCommand = table.concat(command, " ")

		utils.log('Constructed timed-command: ' .. sCommand, utils.LOG_DEBUG)

		return sCommand
	end

	-- get a reference to the latest entry in the commandArray so we can
	-- keep modifying it here.
	local latest, command, sValue = domoticz.sendCommand(name, constructCommand())

	return {
		['_constructCommand'] = constructCommand, -- for testing purposes
		['_latest'] = latest, -- for testing purposes
		['afterSec'] = function(seconds)
			afterValue = seconds
			latest[command] = constructCommand()
			return {
				['forMin'] = function(minutes)
					forValue = minutes
					latest[command] = constructCommand()
				end
			}
		end,
		['afterMin'] = function(minutes)
			afterValue = minutes * 60
			latest[command] = constructCommand()
			return {
				['forMin'] = function(minutes)
					forValue = minutes
					latest[command] = constructCommand()
				end
			}
		end,
		['forMin'] = function(minutes)
			forValue = minutes
			latest[command] = constructCommand()
			return {
				['afterSec'] = function(seconds)
					afterValue = seconds
					latest[command] = constructCommand()
				end,
				['afterMin'] = function(minutes)
					afterValue = minutes * 60
					latest[command] = constructCommand()
				end
			}
		end,
		['withinMin'] = function(minutes)
			randomValue = minutes
			latest[command] = constructCommand()
			return {
				['forMin'] = function(minutes)
					forValue = minutes
					latest[command] = constructCommand()
				end
			}
		end
	}
end

return TimedCommand