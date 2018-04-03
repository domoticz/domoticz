local TESTMODE = false
globalvariables['testmode'] = false
--globalvariables['dzVents_log_level'] = 4 --debug

if (_G.TESTMODE) then
	TESTMODE = false
	globalvariables['testmode'] = false

end

local currentPath = globalvariables['script_path'] -- should be path/to/domoticz/scripts/dzVents

_G.scriptsFolderPath = currentPath .. 'scripts' -- global
_G.generatedScriptsFolderPath = currentPath .. 'generated_scripts' -- global
_G.dataFolderPath = currentPath .. 'data' -- global

package.path = package.path .. ';' .. currentPath .. '?.lua'
package.path = package.path .. ';' .. currentPath .. '../../dzVents/runtime/?.lua'
package.path = package.path .. ';' .. currentPath .. '../../dzVents/runtime/device-adapters/?.lua'
package.path = package.path .. ';' .. currentPath .. 'dzVents/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/?.lua'
package.path = package.path .. ';' .. currentPath .. '../lua/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/modules/?.lua'
package.path = package.path .. ';' .. currentPath .. 'generated_scripts/?.lua'
package.path = package.path .. ';' .. currentPath .. 'data/?.lua'
package.path = package.path .. ';' .. currentPath .. 'modules/?.lua'

local EventHelpers = require('EventHelpers')
local helpers = EventHelpers()
local utils = require('Utils')


if (tonumber(globalvariables['dzVents_log_level']) == utils.LOG_DEBUG or TESTMODE) then
	print('Debug: Dumping domoticz data to ' .. currentPath .. 'domoticzData.lua')
	local persistence = require('persistence')
	persistence.store(currentPath .. 'domoticzData.lua', domoticzData)

	local events, length = helpers.getEventSummary()
	if (length > 0) then
		print('Debug: dzVents version: 2.4.2')

		print('Debug: Event triggers:')
		for i, event in pairs(events) do
			print('Debug: ' .. event)
		end
	end

	if (globalvariables['isTimeEvent']) then
		print('Debug: Event triggers:')
		print('Debug: • Timer')
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
