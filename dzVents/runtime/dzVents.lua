local TESTMODE = false
globalvariables['testmode'] = false
--globalvariables['dzVents_log_level'] = 4 --debug

if (_G.TESTMODE) then
	TESTMODE = false
	globalvariables['testmode'] = false
end

local scriptPath = globalvariables['script_path'] -- should be ${szUserDataFolder}/scripts/dzVents/
local runtimePath = globalvariables['runtime_path'] -- should be ${szStartupFolder}/dzVents/runtime/

_G.scriptsFolderPath = scriptPath .. 'scripts' -- global
_G.generatedScriptsFolderPath = scriptPath .. 'generated_scripts' -- global
_G.dataFolderPath = scriptPath .. 'data' -- global

package.path =
	scriptPath .. '?.lua;' ..
	runtimePath .. '?.lua;' ..
	runtimePath .. 'device-adapters/?.lua;' ..
	scriptPath .. 'dzVents/?.lua;' ..
	scriptPath .. 'scripts/?.lua;' ..
	scriptPath .. '../lua/?.lua;' ..
	scriptPath .. 'scripts/modules/?.lua;' ..
	scriptPath .. '?.lua;' ..
	scriptPath .. 'generated_scripts/?.lua;' ..
	scriptPath .. 'data/?.lua;' ..
	scriptPath .. 'modules/?.lua;' ..
	package.path

local EventHelpers = require('EventHelpers')
local helpers = EventHelpers()
local utils = require('Utils')


if (tonumber(globalvariables['dzVents_log_level']) == utils.LOG_DEBUG or TESTMODE) then
	print('Debug: Dumping domoticz data to ' .. scriptPath .. 'domoticzData.lua')
	local persistence = require('persistence')
	persistence.store(scriptPath .. 'domoticzData.lua', domoticzData)
	--persistence.store(scriptPath .. 'globalvariables.lua', globalvariables)
	--persistence.store(scriptPath .. 'timeofday.lua', timeofday)

	local events, length = helpers.getEventSummary()
	if (length > 0) then
		print('Debug: dzVents version: '.. globalvariables.dzVents_version)

		print('Debug: Event triggers:')
		for i, event in pairs(events) do
			print('Debug: ' .. event)
		end
	end

	if (globalvariables['isTimeEvent']) then
		print('Debug: Event triggers:')
		print('Debug: - Timer')
	end
end

commandArray = {}

local isTimeEvent = globalvariables['isTimeEvent']

if (isTimeEvent) then
	commandArray = helpers.dispatchTimerEventsToScripts()
end

helpers.dispatchDeviceEventsToScripts()
helpers.dispatchVariableEventsToScripts()
helpers.dispatchSecurityEventsToScripts()
helpers.dispatchSceneGroupEventsToScripts()
helpers.dispatchHTTPResponseEventsToScripts()
helpers.dispatchSystemEventsToScripts()
helpers.dispatchCustomEventsToScripts()

commandArray = helpers.domoticz.commandArray

return commandArray
