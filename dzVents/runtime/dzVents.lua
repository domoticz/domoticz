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

package.path = package.path .. ';' .. scriptPath .. '?.lua'
package.path = package.path .. ';' .. runtimePath .. '?.lua'
package.path = package.path .. ';' .. runtimePath .. 'device-adapters/?.lua'
package.path = package.path .. ';' .. scriptPath .. 'dzVents/?.lua'
package.path = package.path .. ';' .. scriptPath .. 'scripts/?.lua'
package.path = package.path .. ';' .. scriptPath .. '../lua/?.lua'
package.path = package.path .. ';' .. scriptPath .. 'scripts/modules/?.lua'
package.path = package.path .. ';' .. scriptPath .. '?.lua'
package.path = package.path .. ';' .. scriptPath .. 'generated_scripts/?.lua'
package.path = package.path .. ';' .. scriptPath .. 'data/?.lua'
package.path = package.path .. ';' .. scriptPath .. 'modules/?.lua'

local EventHelpers = require('EventHelpers')
local helpers = EventHelpers()
local utils = require('Utils')


if (tonumber(globalvariables['dzVents_log_level']) == utils.LOG_DEBUG or TESTMODE) then
	print('Debug: Dumping domoticz data to ' .. scriptPath .. 'domoticzData.lua')
	local persistence = require('persistence')
	persistence.store(scriptPath .. 'domoticzData.lua', domoticzData)

	local events, length = helpers.getEventSummary()
	if (length > 0) then
		print('Debug: dzVents version: 2.4.7')

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
commandArray = helpers.domoticz.commandArray


return commandArray
