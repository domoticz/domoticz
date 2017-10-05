local currentPath = globalvariables['script_path']
local triggerReason = globalvariables['script_reason']

_G.scriptsFolderPath = currentPath .. 'scripts' -- global
_G.generatedScriptsFolderPath = currentPath .. 'generated_scripts' -- global
_G.dataFolderPath = currentPath .. 'data' -- global

package.path = package.path .. ';' .. currentPath .. '?.lua'
package.path = package.path .. ';' .. currentPath .. 'runtime/?.lua'
package.path = package.path .. ';' .. currentPath .. 'runtime/device-adapters/?.lua'
package.path = package.path .. ';' .. currentPath .. 'dzVents/?.lua'
package.path = package.path .. ';' .. currentPath .. 'scripts/?.lua'
package.path = package.path .. ';' .. currentPath .. 'generated_scripts/?.lua'
package.path = package.path .. ';' .. currentPath .. 'data/?.lua'

local EventHelpers = require('EventHelpers')
local helpers = EventHelpers()
local utils = require('Utils')

if (tonumber(globalvariables['dzVents_log_level']) == utils.LOG_DEBUG) then
	utils.log('Dumping domoticz data to ' .. currentPath .. '/domoticzData.lua', utils.LOG_DEBUG)
	local persistence = require('persistence')
	persistence.store(currentPath .. '/domoticzData.lua', domoticzData)
end
--

commandArray = {}

utils.log('Event trigger type: ' .. triggerReason, utils.LOG_DEBUG)

if triggerReason == "time" then
	commandArray = helpers.dispatchTimerEventsToScripts()
elseif triggerReason == "device" then
	commandArray = helpers.dispatchDeviceEventsToScripts()
elseif triggerReason == "uservariable" then
	commandArray = helpers.dispatchVariableEventsToScripts()
elseif triggerReason == 'security' then
	commandArray = helpers.dispatchSecurityEventsToScripts()
else
	utils.log("Unknown trigger: ", triggerReason, utils.LOG_DEBUG)
end

return commandArray

